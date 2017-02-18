#include "mbed.h"
#include "rtos.h"

#include "config.h"

#include "xbee.hpp"
#include "ssWiSocket.hpp"

#ifndef DEBUG
#define     printf(fmt,...)     
#endif

// global configuration
global_confg_t global_config;

// ssWi sockets
ssWiSocket* socket_command;
ssWiSocket* socket_moisture[MAX_NUM_NODES];
ssWiSocket* socket_temperature[MAX_NUM_NODES];
ssWiSocket* socket_humidity[MAX_NUM_NODES];
ssWiSocket* socket_response[MAX_NUM_NODES];
ssWiSocket* socket_water[MAX_NUM_NODES];

LocalFileSystem local("local");

// read counter value from file
bool read_counter();
// read board configuration from file
bool read_configuration();

// return true if a message has been received, false otherwise
bool read_from_port(ssWiSocket *socket, int *value);
// blocking read - timeout after 10 seconds
bool read_timeout(ssWiSocket *socket, int *value);

void do_sampling();
void do_watering();

int main() {
    DigitalOut power_device(POWER_EN_PIN);
    power_device = POWER_OFF;

    // supervisor configuration
    printf("SUPERVISOR - config\r\n");

    // read configuration
    if (!read_configuration())
        error("Impossible to read configuration");

    // network configuration
    XBeeModule xbee(XBEE_PIN_TX, XBEE_PIN_RX, PAN_ID, CHANNEL_ID);
    xbee.init(XBEE_TX_PER_SECOND, XBEE_RX_PER_SECOND);
    socket_command = ssWiSocket::createSocket(PORT_COMMANDS);
    for (int i = 0; i < global_config.num_units; i++) {
        socket_moisture[i] = ssWiSocket::createSocket(
                     global_config.nodes[i].address * 5 + PORT_MOISTURE_OFFSET);
        socket_temperature[i] = ssWiSocket::createSocket(
                  global_config.nodes[i].address * 5 + PORT_TEMPERATURE_OFFSET);
        socket_humidity[i] = ssWiSocket::createSocket(
                     global_config.nodes[i].address * 5 + PORT_HUMIDITY_OFFSET);
        socket_response[i] = ssWiSocket::createSocket(
                     global_config.nodes[i].address * 5 + PORT_RESPONSE_OFFSET);
        socket_water[i] = ssWiSocket::createSocket(
                        global_config.nodes[i].address * 5 + PORT_WATER_OFFSET);
    }

    // start
    printf("SUPERVISOR - start\r\n");

    while(1) {
        int minute_counters = 0;
        printf("SUPERVISOR - waiting\r\n");
        do {
            // wait 1 minute
            Thread::wait(INTERVAL_60_SECONDS * 1000);
            minute_counters++;
        } while (minute_counters < global_config.wait_minutes);

        printf("SUPERVISOR - active\r\n");

        // mark as busy
        DigitalOut led_busy(LED4);
        led_busy = 1;
        FILE* fp_busy = fopen(FILE_BSY, "w");

        // power watering units
        power_device = POWER_ON;
        wait(INTERVAL_POWER_START);
        
        read_counter();

        // sample and water
        printf("SUPERVISOR - sampling\r\n");
        do_sampling();
        printf("SUPERVISOR - watering\r\n");
        do_watering();
        
        // increment counter
        global_config.count++;
        FILE* fp = fopen(FILE_CNT, "w");
        fprintf(fp, "%d\n", global_config.count);
        fclose(fp);

        // send shutdown
        printf("SUPERVISOR - shutdown\r\n");

        wait(INTERVAL_SYNC);
        socket_command->write(COMM_SHUTDOWN);
        wait(INTERVAL_SYNC * 2);

        // power off devices
        power_device = POWER_OFF;
        
        // mark as not busy
        led_busy = 0;
        fclose(fp_busy);
    }
}

void do_sampling () {
    FILE* fp = fopen(FILE_SNS, "a");

    socket_command->write(COMM_START_SAMPLING);
    wait(INTERVAL_SAMPLING);
    
    socket_command->write(COMM_STOP_SAMPLING);
    wait(INTERVAL_SYNC);

    for (int i = 0; i < global_config.num_units; i++) {
        int temp = 0, humi = 0, mois = 0, sampling_feedback = COMM_NO_VALUE;
        int code;
        if (!read_timeout(socket_response[i], &sampling_feedback))
            code = 1;
        else {
            switch (sampling_feedback) {
                case COMM_SAMPLING_OK:
                    code = (read_timeout(socket_temperature[i], &temp) &&
                            read_timeout(socket_humidity[i], &humi) &&
                            read_timeout(socket_moisture[i], &mois)) ? 0 : 2;
                    break;
                case COMM_SAMPLING_KO: code = 3; break;
                case COMM_SAMPLING_OUT: code = 4; break;
                default: code = 5;
            }
        }
        fprintf(fp, "%d %d %d %4.2f %4.2f %4.2f\n", global_config.count,
                                                 global_config.nodes[i].address,
                                                 code,
                                                 (double)humi/10.0,
                                                 (double)temp/10.0,
                                                 (double)mois/10.0);
    }
    fclose(fp);
}

void do_watering () {
    FILE* fp = fopen(FILE_WTR, "a");
    for (int i = 0; i < global_config.num_units; i++) {
        if (global_config.count % global_config.nodes[i].watering_wait)
            continue;
        // write watering time in seconds
        socket_water[i]->write(global_config.nodes[i].watering_seconds);
        wait(INTERVAL_SYNC);
        // send watering command
        socket_command->write(COMM_START_WATERING_OFFSET + 
                                                global_config.nodes[i].address);
        wait(global_config.nodes[i].watering_seconds + INTERVAL_SYNC + 
                                                                 INTERVAL_SYNC);
        
        int watering_response = 0;
        int code = 4;
        if (read_timeout(socket_response[i], &watering_response)) {
            switch(watering_response) {
                case COMM_WATERING_KO: code = 1; break;
                case COMM_WATERING_OK: code = 0; break;
                case COMM_LOW_WATER_LEVEL: code = 2; break;
                default: code = 3;
            }
        }
        fprintf(fp, "%d %d %d %d\n", global_config.count,
                                     global_config.nodes[i].address,
                                     code,
                                     global_config.nodes[i].watering_seconds);
    }
    fclose(fp);
}

bool read_from_port(ssWiSocket *socket, int *value) {
    int prev = *value;
    *value = socket->read();
    return (*value) != prev;
}

bool read_timeout(ssWiSocket *socket, int *value) {
    Timer t;
    t.start();
    bool ret;
    int v = COMM_NO_VALUE;
    double start = t.read();
    do {
        if ((t.read() - start) > TIMEOUT_READ)
            return false;
        ret = read_from_port(socket, &v);
    } while(!ret);
    t.stop();
    *value = v;
    return true;
}

bool read_configuration() {
    int state = 0;
    int n_unit = 0;
    
    FILE *fp = fopen(FILE_CFG, "r");
    if(fp == NULL)
        return false;
    char line[250];
    
    while(fgets(line, sizeof(line), fp)) {
        if (line[0] == '#')
            continue;
        switch(state) {
            case 0: //read interval length
                sscanf(line, "%d\r", &global_config.wait_minutes);
                state = 1;
                break;
            case 1: //read number of watering units
                sscanf(line, "%d\r", &global_config.num_units);
                state = 2;
                break;
            case 2: //read number of watering units
                sscanf(line, "%d %d %d\r",
                                &global_config.nodes[n_unit].address,
                                &global_config.nodes[n_unit].watering_wait,
                                &global_config.nodes[n_unit].watering_seconds);
                n_unit++;
                if (n_unit >= global_config.num_units || n_unit >=MAX_NUM_NODES)
                    state = 3;
                break;
        }
    }
    fclose(fp);

    return true;
}

bool read_counter () {
    FILE *fp = fopen(FILE_CNT, "r");
    if(fp == NULL) {
        fp = fopen(FILE_CNT, "w");
        if(fp == NULL)
            return false;
        global_config.count = 0;
        fprintf(fp, "0\n");
    } else
        fscanf(fp, "%d\n", &global_config.count);
    fclose(fp);
    
    return true;
}
