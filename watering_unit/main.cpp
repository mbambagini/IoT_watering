#include "mbed.h"
#include "rtos.h"

#include "config.hpp"

#include "DHT11.h"

#include "xbee.hpp"
#include "ssWiSocket.hpp"

#ifndef DEBUG
#define     printf(fmt,...)     
#endif

LocalFileSystem local("local");

// global configuration
watering_unit_config_t global_config;

// ssWi sockets
ssWiSocket* socket_moisture = NULL;
ssWiSocket* socket_temperature = NULL;
ssWiSocket* socket_humidity = NULL;
ssWiSocket* socket_response = NULL;
ssWiSocket* socket_water = NULL;
ssWiSocket* socket_command = NULL;

// thread functions
void thread_sensing_fcn();
void thread_watering_fcn();

// return true if a new message has been received, false otherwise
bool read_from_port(ssWiSocket *socket, int *value);

int main() {
    Timer t;
    t.start();
    
    printf("MAIN - configuration\r\n");
    
    // read configuration    
    FILE *fp = fopen("/local/cfg.txt", "r");
    if (fp == NULL)
        error("missing configuration file\r\n");
    fscanf(fp, "%d", &global_config.address);
    fclose(fp);
    global_config.moisture_port = GET_MOISTURE_PORT(global_config);
    global_config.temperature_port = GET_TEMPERATURE_PORT(global_config);
    global_config.humidity_port = GET_HUMIDITY_PORT(global_config);
    global_config.response_port = GET_RESPONSE_PORT(global_config);
    global_config.water_port = GET_WATER_PORT(global_config);

    // configure network
    XBeeModule xbee(XBEE_PIN_TX, XBEE_PIN_RX, PAN_ID, CHANNEL_ID);
    xbee.init(XBEE_TX_PER_SECOND, XBEE_RX_PER_SECOND);
    socket_moisture = ssWiSocket::createSocket(global_config.moisture_port);
    socket_temperature = 
                       ssWiSocket::createSocket(global_config.temperature_port);
    socket_humidity = ssWiSocket::createSocket(global_config.humidity_port);
    socket_response = ssWiSocket::createSocket(global_config.response_port);
    socket_water = ssWiSocket::createSocket(global_config.water_port);
    socket_command = ssWiSocket::createSocket(PORT_COMMANDS);

    // threads
    Thread thread_sensing(osPriorityAboveNormal);
    thread_sensing.start(&thread_sensing_fcn);
    Thread thread_watering(osPriorityHigh);
    thread_watering.start(&thread_watering_fcn);

    // wait 10 seconds since the board started
    while(t.read() < TIMEOUT_CONF);
    t.stop();

    printf("MAIN - board ready\r\n");

    // handle messages
    bool recv_start_sampling = false;
    bool recv_stop_sampling = false;
    bool recv_watering = false;
    int msg = COMM_NO_VALUE;
    while(true) {
        if (!read_from_port(socket_command, &msg))
            continue;
        if (!recv_start_sampling && msg == COMM_START_SAMPLING) {
            recv_start_sampling = true;
            printf("MAIN - sampling start command\r\n");
            thread_sensing.signal_set(SIGNAL_START_ARRIVED);
            continue;
        }
        if (!recv_stop_sampling && msg == COMM_STOP_SAMPLING) {
            recv_stop_sampling = true;
            printf("MAIN - sampling stop command\r\n");
            thread_sensing.signal_set(SIGNAL_STOP_ARRIVED);
            continue;
        }
        if (!recv_watering &&  msg == GET_WATERING_COMMAND(global_config)) {
            recv_watering = true;
            printf("MAIN - watering start command\r\n");
            thread_watering.signal_set(SIGNAL_WATERING_ARRIVED);
            continue;
        }
        if (msg == COMM_SHUTDOWN) {
            printf("MAIN - shutdown command\r\n");
            recv_start_sampling = false;
            recv_stop_sampling = false;
            recv_watering = false;
            socket_moisture->write(COMM_NO_VALUE);
            socket_temperature->write(COMM_NO_VALUE);
            socket_humidity->write(COMM_NO_VALUE);
            socket_response->write(COMM_NO_VALUE);
            continue;
        }
    }

    printf("MAIN - join threads and end\r\n");

    thread_sensing.join();
    thread_watering.join();

    return 0;
}

void thread_sensing_fcn () {
    AnalogIn sensor_moisture(HW_PIN_MOISTURE);

    while (1) {
        int msg = COMM_SAMPLING_OK;
        DHT11 sensor_dht(HW_PIN_TEMPERATURE);
        printf("SAMP - waiting...\r\n");

        // wait supervisor message
        Thread::signal_wait(SIGNAL_START_ARRIVED);
        printf("SAMP - start\r\n");

        DigitalOut l(LED3);
        l = 1;

        Timer t;
        t.start();
        // wait two seconds for HDT sensor
        wait(2.0);
        
        // sample values
        double sens_mois = 0.0;
        double sens_temp = 0.0;
        double sens_humi = 0.0;
        for (int i = 0; i < NUM_SAMPLES; i++) {
            if (sensor_dht.readData() != 0) {
                printf("SAMP - error %d\r\n", error);
                msg = COMM_SAMPLING_KO;
                break;
            }
            sens_temp += sensor_dht.readTemperature();
            sens_humi += sensor_dht.readHumidity();
            sens_mois += sensor_moisture.read() * 100;
            if (t.read() > TIMEOUT_SAMPLING) {
                // timeout expired, exit
                msg = COMM_SAMPLING_OUT;
                break;
            }
            Thread::wait(INTERVAL_SAMPLING);
        }
        t.stop();
        
        // compute averages
        sens_mois = (sens_mois / NUM_SAMPLES) * 10.0;
        sens_temp = (sens_temp / NUM_SAMPLES) * 10.0;
        sens_humi = (sens_humi / NUM_SAMPLES) * 10.0;
        printf("SAMP - %f, %f, %f\r\n", sens_mois, sens_temp, sens_humi);

        // wait supervisor stop message
        Thread::signal_wait(SIGNAL_STOP_ARRIVED);

        // write averages and response
        int value = (int)sens_mois;
        socket_moisture->write(value == 0 ? 1 : value);
        value = (int)sens_temp;
        socket_temperature->write(value == 0 ? 1 : value);
        value = (int)sens_humi;
        socket_humidity->write(value == 0 ? 1 : value);
        socket_response->write(msg);
        printf("SAMP - end\r\n");
        
        l = 0;
    }
}

void thread_watering_fcn() {
    DigitalOut pump(HW_PIN_PUMP);
    DigitalIn level_sens(HW_PIN_LOW_WATER_LEVEL);

    while (1) {
        pump = 0;
        printf("WATR - waiting...\r\n");

        // wait watering command
        Thread::signal_wait(SIGNAL_WATERING_ARRIVED);
        printf("WATR - start\r\n");
        Timer t;
        t.start();

        DigitalOut l(LED2);
        l = 1;

        // read total second to water
        int seconds = COMM_NO_VALUE;
        while(!read_from_port(socket_water, &seconds) &&
                                                     t.read()<TIMEOUT_WATERING);
        t.stop();
        if (t.read() >= TIMEOUT_WATERING) {
            socket_response->write(COMM_WATERING_KO);
            continue;
        }
        printf("WATR - required time %ds\r\n", seconds);

        // start watering
        int msg = COMM_WATERING_OK;
        double actual_seconds = 0.0;
        while (actual_seconds < seconds) {
            if (!level_sens) {
                // low water, stop pump and exit
                pump = 0;
                printf("WATR - watering level error\r\n");
                msg = COMM_LOW_WATER_LEVEL;
                break;
            }
            // water for 1 second
            t.reset(); t.start();
            double scale = seconds > (actual_seconds + 1.0) ? 1.0 : seconds -
                                                                 actual_seconds;
            pump = 1;
            Thread::wait((unsigned int)(scale * INTERVAL_WATERING));
            t.stop();
            actual_seconds += t.read();
        }
        pump = 0;
        printf("WATR - elapsed time %fs\r\n", actual_seconds);
        socket_response->write(msg);
        
        l = 0;
    }
}

bool read_from_port(ssWiSocket *socket, int *value) {
    int prev = *value;
    *value = socket->read();
    return (*value) != prev;
}
