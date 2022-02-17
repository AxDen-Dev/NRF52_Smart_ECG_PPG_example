/**
 * Copyright (c) 2015 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef BLE_SERVICE_H__
#define BLE_SERVICE_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_SERVICE_BLE_OBSERVER_PRIO 2

#define BLE_SERVICE_DEF(_name)                                                                          \
static ble_service_t _name;                                                                             \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
		BLE_SERVICE_BLE_OBSERVER_PRIO,                                                     \
		ble_service_on_ble_evt, &_name)

#define DEVICE_INFO_UUID_SERVICE 0x180A
#define DEVICE_MODEL_NUMBER_UUID_CHAR 0x2A24
#define DEVICE_SERIAL_NUMBER_UUID_CHAR 0x2A25
#define DEVICE_HW_REVISION_UUID_CHAR 0x2A27
#define DEVICE_SW_REVISION_UUID_CHAR 0x2A28
#define DEVCIE_MANUFACTURER_CHAR 0x2A29

#define BLE_UUID_BASE        {0xBD, 0xE9, 0x6A, 0xF2, 0x08, 0x69, 0x11, 0xEC, \
        0x9A, 0x03, 0x02, 0x42, 0x00, 0x00, 0x00, 0x03}

#define BLE_IDLE_UUID_SCAN_SERVICE     0xAC12
#define BLE_DATA_READY_UUID_SCAN_SERVICE     0xAC13

#define BLE_UUID_DEVICE_SERVICE     0xAC14
#define BLE_UUID_AGGREGATOR_DEVICE_CHAR 0xAC15
#define BLE_UUID_STREAM_DEVICE_CHAR 0xAC16

typedef enum {

	BLE_AGGREGATOR_CHAR = 0x01, BLE_STREAM_CHAR

} BLE_NOTIFY_EVENTS_CHAR;

// Forward declaration of the ble_service_t type.
typedef struct ble_service_s ble_service_t;

typedef void (*ble_service_tx_complete_handler_t)(uint16_t conn_handle,
		ble_service_t *p_service);

typedef void (*ble_service_notify_state_event_handler_t)(uint16_t conn_handle,
		ble_service_t *p_service, uint8_t p_event_notify_char, uint8_t p_event);

typedef void (*ble_service_write_handler_t)(uint16_t conn_handle,
		ble_service_t *p_service, uint8_t event_type, uint8_t const *buffer,
		uint8_t buffer_size);

/** @brief BLE Service init structure. This structure contains all options and data needed for
 *        initialization of the service.*/
typedef struct {
	ble_service_tx_complete_handler_t ble_service_tx_complete_handler;
	ble_service_notify_state_event_handler_t ble_service_notify_state_event_handler;
	ble_service_write_handler_t ble_write_handler;
} ble_service_init_t;

/**@brief BLE Service structure. This structure contains various status information for the service. */
struct ble_service_s {

	uint16_t device_info_service_handle;
	ble_gatts_char_handles_t device_info_model_number_handles;
	ble_gatts_char_handles_t device_info_serial_number_handles;
	ble_gatts_char_handles_t device_info_hw_revision_handles;
	ble_gatts_char_handles_t device_info_sw_revision_handles;
	ble_gatts_char_handles_t device_info_manufacturer_handles;

	uint8_t uuid_type;
	uint16_t device_service_handle;
	ble_gatts_char_handles_t device_aggregator_char_handles;
	ble_gatts_char_handles_t device_stream_char_handles;

	ble_service_tx_complete_handler_t ble_service_tx_complete_handler;
	ble_service_notify_state_event_handler_t ble_service_notify_state_event_handler;
	ble_service_write_handler_t ble_write_handler;

};

/**@brief Function for initializing BLE Service.
 *
 *
 * @retval NRF_SUCCESS If the service was initialized successfully. Otherwise, an error code is returned.
 */
uint32_t ble_service_init(ble_service_t *p_service,
		const ble_service_init_t *p_service_init);

/**@brief Function for handling the application's BLE stack events.
 *
 * @details This function handles all events from the BLE stack that are of interest to the LED Button Service.
 *
 */
void ble_service_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);

/**@brief Function for sending notification.
 *
 *
 * @retval NRF_SUCCESS If the notification was sent successfully. Otherwise, an error code is returned.
 */
uint32_t ble_service_send_aggregator_notification(uint16_t conn_handle,
		ble_service_t *p_service, uint8_t *buffer, uint16_t buffer_length);

uint32_t ble_service_send_stream_notification(uint16_t conn_handle,
		ble_service_t *p_service, uint8_t *buffer, uint16_t buffer_length);

#ifdef __cplusplus
}
#endif

#endif // BLE_SERVICE_H__

/** @} */
