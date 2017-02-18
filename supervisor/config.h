#ifndef __SUPERVISOR_CONFIG__
#define __SUPERVISOR_CONFIG__

// TIME
#define TIMEOUT_READ                10.0
#define INTERVAL_SAMPLING           40.0
#define INTERVAL_SYNC               5.0
#define INTERVAL_1_SECOND           1.0
#define INTERVAL_60_SECONDS         60
#define INTERVAL_POWER_START        20.0

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

#define MAX_NUM_NODES               10

// FILES
#define FILE_CNT                    "/local/CNT.txt"
#define FILE_CFG                    "/local/cfg.txt"
#define FILE_SNS                    "/local/SNS.txt"
#define FILE_WTR                    "/local/WTR.txt"
#define FILE_BSY                    "/local/BSY.txt"

// NETWORK
#define PAN_ID                      102
#define CHANNEL_ID                  14
#define XBEE_TX_PER_SECOND          3
#define XBEE_RX_PER_SECOND          10
#define XBEE_PIN_TX                 p9
#define XBEE_PIN_RX                 p10

#define POWER_EN_PIN                p27
#define POWER_ON                    0
#define POWER_OFF                   1

struct watering_unit_node_t {
    int address;
    int watering_wait;
    int watering_seconds;
};

struct global_confg_t {
    int count;
    int wait_minutes;
    int num_units;
    watering_unit_node_t nodes[MAX_NUM_NODES];
};

#endif //__SUPERVISOR_CONFIG__
