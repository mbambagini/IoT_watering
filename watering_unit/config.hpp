#ifndef __WATERING_CONFIG__
#define __WATERING_CONFIG__

// Hardware pins
#define HW_PIN_MOISTURE             p20
#define HW_PIN_TEMPERATURE          p17
#define HW_PIN_PUMP                 p25
#define HW_PIN_LOW_WATER_LEVEL      p14

// internal signals/events to synchronize threads
#define SIGNAL_START_ARRIVED        0x01
#define SIGNAL_STOP_ARRIVED         0x02
#define SIGNAL_WATERING_ARRIVED     0x03

// number of samples to read when the system starts
#define NUM_SAMPLES                 10

// timeout to wait before starting
#define TIMEOUT_CONF                10.0
// timeout to finish the sampling process
#define TIMEOUT_SAMPLING            60.0
// timeout to read water quantity
#define TIMEOUT_WATERING            30.0

// time to wait between two consecutive samplings
#define INTERVAL_SAMPLING           2500
// time between two steps of the watering algorithm
#define INTERVAL_WATERING           1000

// COMM VALUES
#define COMM_NO_VALUE               0x0000
#define COMM_START_SAMPLING         0xAAAA
#define COMM_STOP_SAMPLING          0x1111
#define COMM_START_WATERING_OFFSET  0xFF00
#define COMM_SHUTDOWN               0x00FF
#define COMM_EXIT                   0x2244
#define COMM_SAMPLING_KO            0x2222
#define COMM_SAMPLING_OUT           0x5555
#define COMM_SAMPLING_OK            0xBBBB
#define COMM_WATERING_KO            0x3333
#define COMM_WATERING_OK            0xCCCC
#define COMM_LOW_WATER_LEVEL        0x7777

// PORTS
#define PORT_COMMANDS               0x00
#define PORT_HUMIDITY_OFFSET        0x01
#define PORT_TEMPERATURE_OFFSET     0x02
#define PORT_MOISTURE_OFFSET        0x03
#define PORT_WATER_OFFSET           0x04
#define PORT_RESPONSE_OFFSET        0x05
#define GET_MOISTURE_PORT(x)        (PORT_MOISTURE_OFFSET + (x.address * 5))
#define GET_TEMPERATURE_PORT(x)     (PORT_TEMPERATURE_OFFSET + (x.address * 5))
#define GET_HUMIDITY_PORT(x)        (PORT_HUMIDITY_OFFSET + (x.address * 5))
#define GET_WATER_PORT(x)           (PORT_WATER_OFFSET + (x.address * 5))
#define GET_RESPONSE_PORT(x)        (PORT_RESPONSE_OFFSET + (x.address * 5))
#define GET_WATERING_COMMAND(x)     (COMM_START_WATERING_OFFSET + x.address)

// NETWORK
#define PAN_ID                      102
#define CHANNEL_ID                  14
#define XBEE_TX_PER_SECOND          3
#define XBEE_RX_PER_SECOND          10
#define XBEE_PIN_TX                 p9
#define XBEE_PIN_RX                 p10

struct watering_unit_config_t {
    int address;
    int moisture_port;
    int temperature_port;
    int humidity_port;
    int water_port;
    int response_port;
};

#endif //__WATERING_CONFIG__
