/**
 * Copyright (c) 2013 - 2021, Nordic Semiconductor ASA
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

#include "sdk_common.h"

#include "ble_service.h"
#include "ble_srv_common.h"

static void on_write(ble_service_t *p_ble_service, ble_evt_t const *p_ble_evt) {

	ble_gatts_evt_write_t const *p_evt_write =
			&p_ble_evt->evt.gatts_evt.params.write;

	if ((p_evt_write->handle == p_ble_service->device_char_handles.cccd_handle)
			&& (p_ble_service->ble_service_notify_state_event_handler != NULL)) {

		if (p_evt_write->len == 2) {

			if (ble_srv_is_notification_enabled(p_evt_write->data)) {

				//Notify On
				p_ble_service->ble_service_notify_state_event_handler(
						p_ble_evt->evt.gap_evt.conn_handle, p_ble_service,
						BLE_DEVICE_CHAR, 0x01);

			} else {

				//Notify Off
				p_ble_service->ble_service_notify_state_event_handler(
						p_ble_evt->evt.gap_evt.conn_handle, p_ble_service,
						BLE_DEVICE_CHAR, 0x00);

			}

		}

	} else if ((p_evt_write->handle
			== p_ble_service->device_char_handles.value_handle)
			&& (p_ble_service->ble_write_handler != NULL)) {

		if (p_evt_write->len > 0) {

			p_ble_service->ble_write_handler(p_ble_evt->evt.gap_evt.conn_handle,
					p_ble_service, p_evt_write->data, p_evt_write->len);

		}

	}

}

void ble_service_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context) {

	ble_service_t *p_ble_service = (ble_service_t*) p_context;

	switch (p_ble_evt->header.evt_id) {

	case BLE_GATTS_EVT_WRITE:

		on_write(p_ble_service, p_ble_evt);

		break;

	case BLE_GATTS_EVT_HVN_TX_COMPLETE:

		if (p_ble_service->ble_service_tx_complete_handler != NULL) {

			p_ble_service->ble_service_tx_complete_handler(
					p_ble_evt->evt.gap_evt.conn_handle, p_ble_service);

		}

		break;

	default:
		// No implementation needed.
		break;
	}

}

static uint32_t device_serial_number_char_add(ble_service_t *p_service,
		const ble_service_init_t *p_service_init) {

	unsigned char init_data[] = { "00000000" };

	ble_add_char_params_t add_char_params;

	memset(&add_char_params, 0, sizeof(add_char_params));
	add_char_params.uuid = DEVICE_SERIAL_UUID_CHAR;
	add_char_params.init_len = 9;
	add_char_params.max_len = 9;
	add_char_params.char_props.read = 1;
	add_char_params.p_init_value = init_data;
	add_char_params.read_access = SEC_OPEN;

	return characteristic_add(p_service->device_service_handle,
			&add_char_params, &p_service->device_info_serial_handles);

}

static uint32_t device_hw_char_add(ble_service_t *p_service,
		const ble_service_init_t *p_service_init) {

	unsigned char init_data[] = { "0.0.1" };

	ble_add_char_params_t add_char_params;

	memset(&add_char_params, 0, sizeof(add_char_params));

	add_char_params.uuid = DEVICE_HW_UUID_CHAR;
	add_char_params.init_len = 5;
	add_char_params.max_len = 5;
	add_char_params.char_props.read = 1;
	add_char_params.p_init_value = init_data;
	add_char_params.read_access = SEC_OPEN;

	return characteristic_add(p_service->device_service_handle,
			&add_char_params, &p_service->device_info_hw_handles);

}

static uint32_t device_sw_char_add(ble_service_t *p_service,
		const ble_service_init_t *p_service_init) {

	unsigned char init_data[] = { "0.0.1" };

	ble_add_char_params_t add_char_params;

	memset(&add_char_params, 0, sizeof(add_char_params));

	add_char_params.uuid = DEVICE_SW_UUID_CHAR;
	add_char_params.init_len = 5;
	add_char_params.max_len = 5;
	add_char_params.char_props.read = 1;
	add_char_params.p_init_value = init_data;
	add_char_params.read_access = SEC_OPEN;

	return characteristic_add(p_service->device_service_handle,
			&add_char_params, &p_service->device_info_sw_handles);

}

static uint32_t device_manufacturer_char_add(ble_service_t *p_service,
		const ble_service_init_t *p_service_init) {

	unsigned char init_data[] = { "AXDEN" };

	ble_add_char_params_t add_char_params;

	memset(&add_char_params, 0, sizeof(add_char_params));

	add_char_params.uuid = DEVCIE_MANUFACTURER_CHAR;
	add_char_params.init_len = sizeof(init_data) - 1;
	add_char_params.max_len = sizeof(init_data) - 1;
	add_char_params.char_props.read = 1;
	add_char_params.p_init_value = init_data;
	add_char_params.read_access = SEC_OPEN;

	return characteristic_add(p_service->device_service_handle,
			&add_char_params, &p_service->device_info_manufacturer_handles);

}

uint32_t ble_service_device_info_init(ble_service_t *p_service,
		const ble_service_init_t *p_service_init) {

	uint32_t err_code;
	ble_uuid_t ble_uuid;

	BLE_UUID_BLE_ASSIGN(ble_uuid, DEVICE_INFO_UUID_SERVICE);

	err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid,
			&p_service->device_info_service_handle);
	VERIFY_SUCCESS(err_code);

	err_code = device_serial_number_char_add(p_service, p_service_init);
	VERIFY_SUCCESS(err_code);

	err_code = device_hw_char_add(p_service, p_service_init);
	VERIFY_SUCCESS(err_code);

	err_code = device_sw_char_add(p_service, p_service_init);
	VERIFY_SUCCESS(err_code);

	err_code = device_manufacturer_char_add(p_service, p_service_init);
	VERIFY_SUCCESS(err_code);

	return NRF_SUCCESS;
}

static uint32_t device_char_add(ble_service_t *p_service,
		const ble_service_init_t *p_service_init) {

	ble_add_char_params_t add_char_params;

	memset(&add_char_params, 0, sizeof(add_char_params));
	add_char_params.uuid = BLE_UUID_DEVICE_CHAR;
	add_char_params.uuid_type = p_service->uuid_type;
	add_char_params.init_len = 213;
	add_char_params.max_len = 213;

	add_char_params.is_var_len = true;

	add_char_params.char_props.notify = 1;
	add_char_params.char_props.write = 1;

	add_char_params.cccd_write_access = SEC_OPEN;
	add_char_params.write_access = SEC_OPEN;

	return characteristic_add(p_service->device_service_handle,
			&add_char_params, &p_service->device_char_handles);

}

uint32_t ble_service_device_init(ble_service_t *p_service,
		const ble_service_init_t *p_service_init) {

	uint32_t err_code;
	ble_uuid_t ble_uuid;

	// Add service.
	ble_uuid128_t base_uuid = { BLE_UUID_BASE };
	err_code = sd_ble_uuid_vs_add(&base_uuid, &p_service->uuid_type);
	VERIFY_SUCCESS(err_code);

	ble_uuid.type = p_service->uuid_type;
	ble_uuid.uuid = BLE_UUID_DEVICE_SERVICE;

	err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid,
			&p_service->device_service_handle);
	VERIFY_SUCCESS(err_code);

	err_code = device_char_add(p_service, p_service_init);
	VERIFY_SUCCESS(err_code);

	return NRF_SUCCESS;

}

uint32_t ble_service_init(ble_service_t *p_service,
		const ble_service_init_t *p_service_init) {

	p_service->ble_service_tx_complete_handler =
			p_service_init->ble_service_tx_complete_handler;

	p_service->ble_service_notify_state_event_handler =
			p_service_init->ble_service_notify_state_event_handler;

	p_service->ble_write_handler = p_service_init->ble_write_handler;

	ble_service_device_info_init(p_service, p_service_init);

	return ble_service_device_init(p_service, p_service_init);

}

uint32_t ble_service_send_notification(uint16_t conn_handle,
		ble_service_t *p_service, uint8_t *buffer, uint16_t buffer_length) {

	ble_gatts_hvx_params_t params;
	uint16_t len = buffer_length;

	memset(&params, 0, sizeof(params));
	params.type = BLE_GATT_HVX_NOTIFICATION;
	params.handle = p_service->device_char_handles.value_handle;
	params.p_data = buffer;
	params.p_len = &len;

	return sd_ble_gatts_hvx(conn_handle, &params);

}

