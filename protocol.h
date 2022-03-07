#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include "stdio.h"

#define COLLECTION_CYCLE_TIMEOUT 180

#define CONFIG_FILE     (0x8010)
#define CONFIG_REC_KEY  (0x7010)

#define PACKET_TYPE_UPLINK_EVENT_HEADER_0 0x04
#define PACKET_TYPE_UPLINK_SENSOR_DATA_HEADER_0 0x06

#define PACKET_TYPE_DOWNLINK_ACK_HEADER_0 0x00
#define PACKET_TYPE_DOWNLINK_SETUP_DATA_HEADER_0 0x03

#define CHIP_MANUFACTURER_NORDIC 0x01

#define CHIP_IDENTIFIER_NRF52832 0x01

#define COMMUNICATION_TYPE_BLE_CONNECTION_1M 0x01

#define EVENT_TYPE_POWER_OFF 0x00
#define EVENT_TYPE_POWER_ON 0x01
#define EVENT_TYPE_RUN_TIME_EVENT 0x02

#define PRODUCT_ID_AA_MB_02 0x02

#define NODE_EVENT_PACKET_SIZE 14
#define NODE_DATA_PACKET_SIZE 21

typedef union {

	struct {

		uint8_t DEVICE_ROLE :1;
		uint8_t PACKET_TYPE :3;
		uint8_t CHIP_MANUFACTURER :4;

	} bits;

	uint8_t value;

} packet_header_0_t;

typedef union {

	struct {

		uint8_t CHIP_IDENTIFIER :5;
		uint8_t COMMUNICATION_TYPE :3;

	} bits;

	uint8_t value;

} packet_header_1_t;

typedef union {

	struct {

		uint8_t EVENT_TYPE :2;
		uint8_t UNKNOWN_RESET :1;
		uint8_t COMMUNICATION_TIMEOUT_RESET :1;
		uint8_t SENSOR_ERROR_RESET :1;
		uint8_t MAIN_LOOP_STUCK_RESET :1;
		uint8_t WDT_RESET :1;
		uint8_t SETUP_COMMAND_RESET :1;

	} bits;

	uint8_t value;

} packet_event_0_t;

typedef union {

	struct {

		uint8_t MAX30003_ERROR :1;
		uint8_t MAX30101_ERROR :1;
		uint8_t SI7051_ERROR :1;
		uint8_t RESERVED_0 :1;
		uint8_t RESERVED_1 :1;
		uint8_t RESERVED_2 :1;
		uint8_t RESERVED_3 :1;
		uint8_t RESERVED_4 :1;

	} bits;

	uint8_t value;

} packet_event_1_t;

typedef union {

	struct {

		uint16_t PRODUCT_ID :14;
		uint16_t DEVICE_TYPE :1;
		uint16_t PRODUCT_TYPE :1;

	} bits;

	uint16_t value;

} product_id_t;

typedef union {

	struct {

		uint8_t header_0;
		uint8_t header_1;
		uint8_t mac_address[8];
		uint8_t product_id[2];
		uint8_t event_value_0;
		uint8_t event_value_1;

	} Event_Packet;

	struct {

		uint8_t header_0;
		uint8_t header_1;
		uint8_t mac_address[8];
		uint8_t packet_info; //11Byte
		uint8_t battery_value;
		uint8_t temperature[2];
		uint8_t green_heart_rate;
		uint8_t green_heart_rate_count;
		uint8_t ir_heart_rate;
		uint8_t ir_heart_rate_count;
		uint8_t spo2;
		uint8_t spo2_count;
		uint8_t ecg_heart_rate;
		uint8_t ecg_heart_rate_count;

	} Data_Packet;

	uint8_t buffer[22];

} node_packet_data_t;

#endif /* PROTOCOL_H_ */
