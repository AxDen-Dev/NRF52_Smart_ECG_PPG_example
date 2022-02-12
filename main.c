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

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_err.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "boards.h"
#include "app_timer.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "ble_service.h"
#include "board_define.h"
#include "uart_usb_in_out.h"
#include "ecg_algorithm.h"
#include "heartrate_spo2_algorithm.h"
#include "max30003.h"
#include "max30101.h"
#include "si7051.h"
#include "protocol.h"
#include "nrf_drv_twi.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_spi.h"
#include "app_uart.h"
#include "nrf_uart.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_saadc.h"

#define DEVICE_NAME                     "SmartEcgPpg"                         /**< Name of device. Will be included in the advertising data. */

#define APP_BLE_OBSERVER_PRIO           3                                       /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG            1                                       /**< A tag identifying the SoftDevice BLE configuration. */

#define APP_ADV_INTERVAL                600                                      /**< The advertising interval (in units of 0.625 ms; this value corresponds to 40 ms). */
#define APP_ADV_DURATION                BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED   /**< The advertising time-out (in units of seconds). When set to 0, we will never time out. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)        /**< Minimum acceptable connection interval (0.5 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)        /**< Maximum acceptable connection interval (1 second). */
#define SLAVE_LATENCY                   0                                       /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)         /**< Connection supervisory time-out (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(20000)                  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (15 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(5000)                   /**< Time between each call to sd_ble_gap_conn_param_update after the first call (5 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define APP_TIMER_DELAY          APP_TIMER_TICKS(100)
#define MAX30101_APP_TIMER_DELAY          APP_TIMER_TICKS(20)
#define MAX30003_APP_TIMER_DELAY          APP_TIMER_TICKS(8)

#define DEAD_BEEF                       0xDEADBEEF                              /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define SAADC_SAMPLES_IN_BUFFER 1

BLE_SERVICE_DEF(m_service);
NRF_BLE_GATT_DEF(m_gatt); /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr); /**< Context for the Queued Write module.*/

APP_TIMER_DEF(m_app_timer_id);
APP_TIMER_DEF(m_sensor_timer_id);

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID; /**< Handle of the current connection. */

static uint8_t m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET; /**< Advertising handle used to identify an advertising set. */
static uint8_t m_enc_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX]; /**< Buffer for storing an encoded advertising set. */
static uint8_t m_enc_scan_response_data[BLE_GAP_ADV_SET_DATA_SIZE_MAX]; /**< Buffer for storing an encoded scan data. */

volatile uint8_t twi_read_done = 0x00;
volatile uint8_t twi_write_done = 0x00;
volatile uint8_t twi_address_nack = 0x00;
const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(0);

volatile uint8_t spi_xfer_done = 0x00;
static const nrf_drv_spi_t m_spi = NRF_DRV_SPI_INSTANCE(1);

static nrf_saadc_value_t m_buffer_pool_low_power[2][SAADC_SAMPLES_IN_BUFFER];
static volatile uint8_t m_saadc_initialized = 0x00;

static volatile uint8_t app_timer_update = 0x00;
static volatile uint8_t sensor_timer_update = 0x00;
volatile uint8_t max30003_100_ms_timer = 0x00;
volatile uint8_t max30101_100_ms_timer = 0x00;
volatile uint8_t si7051_100_ms_timer = 0x00;

static volatile uint8_t tx_complete_state = 0x01;
static volatile uint8_t notify_enable_state = 0x00;

static int16_t temperature = 0;
static uint8_t ir_heart_rate = 0;
static uint8_t ir_heart_rate_count = 0;
static uint8_t green_heart_rate = 0;
static uint8_t green_heart_rate_count = 0;
static uint8_t spo2 = 0;
static uint8_t spo2_count = 0;
static uint8_t ecg_heart_rate = 0;
static uint8_t ecg_heart_rate_count = 0;
static uint8_t battery_value = 0x00;

static uint8_t mac_address[8] = { 0x00 };

static uint8_t payload_size = 0;
static uint8_t payload[115];

static uint8_t radio_packet_protocol_size = 0;
static radio_packet_protocol_t radio_packet_protocol;

static uint32_t collection_cycle_timeout_count = COLLECTION_CYCLE_TIMEOUT;
static uint32_t collection_cycle_timer_count = 0;
static volatile uint8_t collection_cycle_update = 0x01;

/**@brief Struct that contains pointers to the encoded advertising data. */
static ble_gap_adv_data_t m_adv_data = { .adv_data = { .p_data = m_enc_advdata,
		.len = BLE_GAP_ADV_SET_DATA_SIZE_MAX }, .scan_rsp_data = { .p_data =
		m_enc_scan_response_data, .len = BLE_GAP_ADV_SET_DATA_SIZE_MAX

} };

/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name) {
	app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module.
 */

static void app_time_timeout_handler(void *p_context) {
	static uint8_t one_sec_timer_count = 0;
	UNUSED_PARAMETER(p_context);

	one_sec_timer_count += 1;

	if (one_sec_timer_count > 10) {

		collection_cycle_timer_count += 1;

		if (collection_cycle_timer_count > collection_cycle_timeout_count) {

			collection_cycle_update = 0x01;
			collection_cycle_timer_count = 0;

		}

		one_sec_timer_count = 0;

	}

	max30003_100_ms_timer = 0x01;
	max30101_100_ms_timer = 0x01;
	si7051_100_ms_timer = 0x01;

	app_timer_update = 0x01;

}

static void sensor_time_timeout_handler(void *p_context) {
	UNUSED_PARAMETER(p_context);

	sensor_timer_update = 0x01;

}

static void timers_init(void) {

	// Initialize timer module, making it use the scheduler
	ret_code_t err_code = app_timer_init();
	APP_ERROR_CHECK(err_code);

	err_code = app_timer_create(&m_app_timer_id, APP_TIMER_MODE_REPEATED,
			app_time_timeout_handler);
	APP_ERROR_CHECK(err_code);

	err_code = app_timer_create(&m_sensor_timer_id, APP_TIMER_MODE_REPEATED,
			sensor_time_timeout_handler);
	APP_ERROR_CHECK(err_code);

}

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void) {
	ret_code_t err_code;
	ble_gap_conn_params_t gap_conn_params;
	ble_gap_conn_sec_mode_t sec_mode;

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

	err_code = sd_ble_gap_device_name_set(&sec_mode,
			(const uint8_t*) DEVICE_NAME, strlen(DEVICE_NAME));
	APP_ERROR_CHECK(err_code);

	memset(&gap_conn_params, 0, sizeof(gap_conn_params));

	gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
	gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
	gap_conn_params.slave_latency = SLAVE_LATENCY;
	gap_conn_params.conn_sup_timeout = CONN_SUP_TIMEOUT;

	err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
	APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void) {
	ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
	APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_init(void) {

	ret_code_t err_code;
	ble_advdata_t advdata;
	ble_advdata_t srdata;

	ble_uuid_t adv_uuids[] = { { BLE_UUID_SERVICE, m_service.uuid_type } };

	// Build and set advertising data.
	memset(&advdata, 0, sizeof(advdata));

	advdata.name_type = BLE_ADVDATA_FULL_NAME;
	advdata.include_appearance = true;
	advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

	memset(&srdata, 0, sizeof(srdata));
	srdata.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
	srdata.uuids_complete.p_uuids = adv_uuids;

	err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data,
			&m_adv_data.adv_data.len);
	APP_ERROR_CHECK(err_code);

	err_code = ble_advdata_encode(&srdata, m_adv_data.scan_rsp_data.p_data,
			&m_adv_data.scan_rsp_data.len);
	APP_ERROR_CHECK(err_code);

	ble_gap_adv_params_t adv_params;

	// Set advertising parameters.
	memset(&adv_params, 0, sizeof(adv_params));

	adv_params.primary_phy = BLE_GAP_PHY_1MBPS;
	adv_params.duration = APP_ADV_DURATION;
	adv_params.properties.type =
	BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
	adv_params.p_peer_addr = NULL;
	adv_params.filter_policy = BLE_GAP_ADV_FP_ANY;
	adv_params.interval = APP_ADV_INTERVAL;

	err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data,
			&adv_params);
	APP_ERROR_CHECK(err_code);

}

/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error) {

	APP_ERROR_HANDLER(nrf_error);

}

static void ble_service_tx_complete_handler(uint16_t conn_handle,
		ble_service_t *p_service) {

	tx_complete_state = 0x01;
	NRF_LOG_INFO("TX complete");

}

static void ble_service_notify_state_event_handler(uint16_t conn_handle,
		ble_service_t *p_service, uint8_t p_event_notify_char, uint8_t p_event) {

	if (p_event_notify_char == BLE_DEVICE_CHAR) {

		notify_enable_state = p_event;
		NRF_LOG_INFO("Notify enable %d", notify_enable_state);

	}

}

static void ble_service_write_handler(uint16_t conn_handle,
		ble_service_t *p_service, uint8_t const *buffer, uint8_t buffer_size) {

}

/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void) {

	ret_code_t err_code;
	ble_service_init_t init = { 0 };
	nrf_ble_qwr_init_t qwr_init = { 0 };

	// Initialize Queued Write Module.
	qwr_init.error_handler = nrf_qwr_error_handler;

	err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
	APP_ERROR_CHECK(err_code);

	init.ble_service_tx_complete_handler = ble_service_tx_complete_handler;
	init.ble_service_notify_state_event_handler =
			ble_service_notify_state_event_handler;
	init.ble_write_handler = ble_service_write_handler;

	err_code = ble_service_init(&m_service, &init);
	APP_ERROR_CHECK(err_code);

}

/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module that
 *          are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply
 *       setting the disconnect_on_fail config parameter, but instead we use the event
 *       handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t *p_evt) {

	ret_code_t err_code;

	if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED) {

		err_code = sd_ble_gap_disconnect(m_conn_handle,
		BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);

		APP_ERROR_CHECK(err_code);

	}

}

/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error) {
	APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void) {

	ret_code_t err_code;
	ble_conn_params_init_t cp_init;

	memset(&cp_init, 0, sizeof(cp_init));

	cp_init.p_conn_params = NULL;
	cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
	cp_init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;
	cp_init.max_conn_params_update_count = MAX_CONN_PARAMS_UPDATE_COUNT;
	cp_init.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID;
	cp_init.disconnect_on_fail = false;
	cp_init.evt_handler = on_conn_params_evt;
	cp_init.error_handler = conn_params_error_handler;

	err_code = ble_conn_params_init(&cp_init);
	APP_ERROR_CHECK(err_code);

}

/**@brief Function for starting advertising.
 */
static void advertising_start(void) {

	ret_code_t err_code;

	err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
	APP_ERROR_CHECK(err_code);

}

/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const *p_ble_evt, void *p_context) {
	ret_code_t err_code;

	switch (p_ble_evt->header.evt_id) {
	case BLE_GAP_EVT_CONNECTED:
		NRF_LOG_INFO("Connected")
		;

		tx_complete_state = 0x01;
		notify_enable_state = 0x00;
		m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

		err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
		APP_ERROR_CHECK(err_code);

		break;

	case BLE_GAP_EVT_DISCONNECTED:
		NRF_LOG_INFO("Disconnected")
		;

		tx_complete_state = 0x01;
		notify_enable_state = 0x00;
		m_conn_handle = BLE_CONN_HANDLE_INVALID;

		advertising_start();

		break;

	case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
		// Pairing not supported
		err_code = sd_ble_gap_sec_params_reply(m_conn_handle,
		BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
		APP_ERROR_CHECK(err_code);
		break;

	case BLE_GAP_EVT_PHY_UPDATE_REQUEST: {
		NRF_LOG_DEBUG("PHY update request.");
		ble_gap_phys_t const phys = { .rx_phys = BLE_GAP_PHY_AUTO, .tx_phys =
		BLE_GAP_PHY_AUTO, };
		err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle,
				&phys);
		APP_ERROR_CHECK(err_code);
	}
		break;

	case BLE_GATTS_EVT_SYS_ATTR_MISSING:
		// No system attributes have been stored.
		err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
		APP_ERROR_CHECK(err_code);
		break;

	case BLE_GATTC_EVT_TIMEOUT:
		// Disconnect on GATT Client timeout event.
		NRF_LOG_DEBUG("GATT Client Timeout.")
		;
		err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
		BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		APP_ERROR_CHECK(err_code);
		break;

	case BLE_GATTS_EVT_TIMEOUT:
		// Disconnect on GATT Server timeout event.
		NRF_LOG_DEBUG("GATT Server Timeout.")
		;
		err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
		BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		APP_ERROR_CHECK(err_code);
		break;

	default:
		// No implementation needed.
		break;
	}
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void) {
	ret_code_t err_code;

	err_code = nrf_sdh_enable_request();
	APP_ERROR_CHECK(err_code);

	// Configure the BLE stack using the default settings.
	// Fetch the start address of the application RAM.
	uint32_t ram_start = 0;
	err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
	APP_ERROR_CHECK(err_code);

	// Enable BLE stack.
	err_code = nrf_sdh_ble_enable(&ram_start);
	APP_ERROR_CHECK(err_code);

	// Register a handler for BLE events.
	NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler,
			NULL);

#ifdef NRF528XX

	sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);

#endif

}

static void log_init(void) {
	ret_code_t err_code = NRF_LOG_INIT(NULL);
	APP_ERROR_CHECK(err_code);

	NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**@brief Function for initializing power management.
 */
static void power_management_init(void) {
	ret_code_t err_code;
	err_code = nrf_pwr_mgmt_init();
	APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void) {
	if (NRF_LOG_PROCESS() == false) {
		nrf_pwr_mgmt_run();
	}
}

static void wait_200ms() {

	for (uint8_t i = 0; i < 2; i++) {

		while (!app_timer_update) {

			idle_state_handle();

		}

		app_timer_update = 0x00;

	}

}

static uint8_t update_Heartrate_SPO2() {

	uint8_t i2c_state = 0x01;

	uint16_t printCount = 0;

	uint32_t red = 0x00;
	uint32_t ir = 0x00;
	uint32_t green = 0x00;
	uint8_t ppgErrorCount = 0;

	app_timer_start(m_sensor_timer_id, MAX30101_APP_TIMER_DELAY, NULL);

	i2c_state = i2c_state & set_max30101_normal_mode();

	setHeartrateMinMax(40, 180);

	if (i2c_state) {

		for (uint32_t i = 0; i < 3000; i++) {

			get_max30101_ir_red_green(&ir, &red, &green);

			if (green > MAX30101_GREEN_PROXY_THR) {

				updateHeartateGreen(green);

				if (updateHeartateIR(ir, red) == 0x01) {

					updateSpo2RedIrAcc();

				}

				ppgErrorCount = 0;

			} else {

				ppgErrorCount++;

				if (ppgErrorCount > MAX30101_GREEN_PROXY_ERROR_COUNT) {

					initHeartrate();

					ppgErrorCount = 0;

				}

			}

			printCount++;

			if (printCount > 500) {

				getHeartrateValue(&ir_heart_rate, &ir_heart_rate_count,
						&green_heart_rate, &green_heart_rate_count);

				getSpo2Value(&spo2, &spo2_count);

				set_uart_usb_in_out_green_heartrate(green_heart_rate,
						green_heart_rate_count);

				set_uart_usb_in_out_ir_heartrate(ir_heart_rate,
						ir_heart_rate_count);

				set_uart_usb_in_out_spo2(spo2, spo2_count);

				printCount = 0;

			}

			while (!sensor_timer_update) {

				idle_state_handle();

			}

			sensor_timer_update = 0x00;

		}

	}

	initHeartrate();

	i2c_state = i2c_state & set_max30101_sleep_mode();

	app_timer_stop(m_sensor_timer_id);

	return i2c_state;

}

static void update_ECG() {

	uint16_t printCount = 0;

	app_timer_start(m_sensor_timer_id, MAX30003_APP_TIMER_DELAY, NULL);

	init_ecg_algorithm();

	for (uint32_t i = 0; i < 7500; i++) {

		int32_t sample = get_max30003_ecg_voltage_sample();

		sample = update_ecg_algorithm(sample);

		get_ecg_heartrate(&ecg_heart_rate, &ecg_heart_rate_count);

		printCount++;

		if (printCount > 1250) {

			set_uart_usb_in_out_ecg_heartrate(ecg_heart_rate,
					ecg_heart_rate_count);

			printCount = 0;

		}

		while (!sensor_timer_update) {

			idle_state_handle();

		}

		sensor_timer_update = 0x00;

	}

	app_timer_stop(m_sensor_timer_id);

}

static void get_mac_address() {

	uint32_t err_code;
	ble_gap_addr_t addr;

	err_code = sd_ble_gap_addr_get(&addr);
	APP_ERROR_CHECK(err_code);

	for (uint8_t i = 0; i < 6; i++) {

		mac_address[5 - i] = addr.addr[i];

	}

}

static void saadc_callback(nrf_drv_saadc_evt_t const *p_event) {

	if (p_event->type == NRF_DRV_SAADC_EVT_DONE) {

		ret_code_t err_code;

		err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer,
		SAADC_SAMPLES_IN_BUFFER);
		APP_ERROR_CHECK(err_code);

		int32_t adc_0_value = p_event->data.done.p_buffer[0];

		float battery = (float) adc_0_value / 255;
		battery = battery * 3.6;
		battery *= 3.0;
		battery_value = (uint8_t) (battery * 10);

		nrf_drv_saadc_uninit();

		NRF_SAADC->INTENCLR = (SAADC_INTENCLR_END_Clear
				<< SAADC_INTENCLR_END_Pos);
		NVIC_ClearPendingIRQ(SAADC_IRQn);

		m_saadc_initialized = 0x00;

		NRF_LOG_INFO("BAT %d", battery_value);
		NRF_LOG_FLUSH();

	}

}

static void saadc_init(void) {

	ret_code_t err_code;
	nrf_drv_saadc_config_t saadc_config;
	nrf_saadc_channel_config_t channel_config0;

	saadc_config.resolution = NRF_SAADC_RESOLUTION_8BIT;
	saadc_config.oversample = NRF_SAADC_OVERSAMPLE_DISABLED;
	saadc_config.interrupt_priority = APP_IRQ_PRIORITY_LOW;

	err_code = nrf_drv_saadc_init(&saadc_config, saadc_callback);
	APP_ERROR_CHECK(err_code);

	channel_config0.reference = NRF_SAADC_REFERENCE_INTERNAL;
	channel_config0.gain = NRF_SAADC_GAIN1_6;
	channel_config0.acq_time = NRF_SAADC_ACQTIME_10US;
	channel_config0.mode = NRF_SAADC_MODE_SINGLE_ENDED;
	channel_config0.pin_p = NRF_SAADC_INPUT_AIN3;
	channel_config0.pin_n = NRF_SAADC_INPUT_DISABLED;
	channel_config0.resistor_p = NRF_SAADC_RESISTOR_DISABLED;
	channel_config0.resistor_n = NRF_SAADC_RESISTOR_DISABLED;

	err_code = nrf_drv_saadc_channel_init(0, &channel_config0);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool_low_power[0],
	SAADC_SAMPLES_IN_BUFFER);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool_low_power[1],
	SAADC_SAMPLES_IN_BUFFER);
	APP_ERROR_CHECK(err_code);

}

static void read_battery_level(void) {

	if (!m_saadc_initialized) {

		saadc_init();

	}

	m_saadc_initialized = 0x01;

	nrf_drv_saadc_sample();

	while (m_saadc_initialized) {

		idle_state_handle();

	}

	nrf_gpio_pin_clear(GPIO_BAT_EN);

}

static void twi_scan(void) {

	NRF_LOG_INFO("I2C Scan started.");

	for (uint16_t i = 0; i < 127; i++) {

		uint8_t reg = 0x01;
		uint8_t twi_timeout = 5;

		twi_write_done = 0x00;
		twi_read_done = 0x00;
		twi_address_nack = 0x00;

		nrf_drv_twi_tx(&m_twi, i, &reg, 1, false);

		while (!twi_write_done && --twi_timeout) {

			idle_state_handle();

		}

		if (!twi_address_nack) {

			set_uart_usb_in_out_find_i2c_address(i);

			NRF_LOG_INFO("FIND %02X", i);

		}

		NRF_LOG_FLUSH();

	}

}

static void twi_handler(nrf_drv_twi_evt_t const *p_event, void *p_context) {

	switch (p_event->type) {

	case NRF_DRV_TWI_EVT_DONE:

		twi_write_done = 0x01;
		twi_read_done = 0x01;

		break;

	default:
//	    NRF_DRV_TWI_EVT_ADDRESS_NACK,
//	    NRF_DRV_TWI_EVT_DATA_NACK

		twi_write_done = 0x01;
		twi_read_done = 0x01;
		twi_address_nack = 0x01;

		break;

	}

}

static void twi_init(void) {

	ret_code_t err_code;

	const nrf_drv_twi_config_t twi_config =
			{ .scl = I2C_SCL_GPIO, .sda = I2C_SDA_GPIO, .frequency =
					NRF_DRV_TWI_FREQ_400K, .interrupt_priority =
					APP_IRQ_PRIORITY_HIGH, .clear_bus_init = false };

	err_code = nrf_drv_twi_init(&m_twi, &twi_config, twi_handler, NULL);
	APP_ERROR_CHECK(err_code);

	nrf_drv_twi_enable(&m_twi);

}

static void spi_event_handler(nrf_drv_spi_evt_t const *p_event, void *p_context) {

	spi_xfer_done = 0x01;

}

static void spi_init() {

	ret_code_t err_code;

	nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
	spi_config.ss_pin = SPI_CS_GPIO;
	spi_config.miso_pin = SPI_MISO_GPIO;
	spi_config.mosi_pin = SPI_MOSI_GPIO;
	spi_config.sck_pin = SPI_SCK_GPIO;
	spi_config.orc = 0xFF;
	spi_config.frequency = NRF_DRV_SPI_FREQ_4M;
	spi_config.mode = NRF_DRV_SPI_MODE_0;
	spi_config.bit_order = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST;

	err_code = nrf_drv_spi_init(&m_spi, &spi_config, spi_event_handler, NULL);
	APP_ERROR_CHECK(err_code);

}

static void uart_event_handle(app_uart_evt_t *p_event) {

	switch (p_event->evt_type) {

	case APP_UART_DATA_READY: {
		uint8_t data;
		UNUSED_VARIABLE(app_uart_get(&data));

	}

		break;

	case APP_UART_DATA: {
		uint8_t data;
		UNUSED_VARIABLE(app_uart_get(&data));

		break;

	}

	case APP_UART_COMMUNICATION_ERROR: {

		NRF_LOG_ERROR("COMM ERROR");
		app_uart_flush();

	}
		break;

	case APP_UART_FIFO_ERROR: {

		NRF_LOG_ERROR("FIFO ERROR");
		app_uart_flush();

	}
		break;

	default:
		break;
	}

}

static void init_uart_1() {

	uint32_t err_code;

	const app_uart_comm_params_t comm_params = { .rx_pin_no = UART_1_RX_GPIO,
			.tx_pin_no = UART_1_TX_GPIO, UART_PIN_DISCONNECTED,
			UART_PIN_DISCONNECTED, .flow_control =
					APP_UART_FLOW_CONTROL_DISABLED, .use_parity = false,
			NRF_UART_BAUDRATE_9600 };

	APP_UART_FIFO_INIT(&comm_params, 256, 256, uart_event_handle,
			APP_IRQ_PRIORITY_LOWEST, err_code);

	APP_ERROR_CHECK(err_code);

}

//static void init_uart_2() {
//
//	uint32_t err_code;
//
//	const app_uart_comm_params_t comm_params = { .rx_pin_no = UART_2_RX_GPIO,
//			.tx_pin_no = UART_2_TX_GPIO, UART_PIN_DISCONNECTED,
//			UART_PIN_DISCONNECTED, .flow_control =
//					APP_UART_FLOW_CONTROL_DISABLED, .use_parity = false,
//			NRF_UART_BAUDRATE_9600 };
//
//	APP_UART_FIFO_INIT(&comm_params, 256, 256, uart_event_handle,
//			APP_IRQ_PRIORITY_LOWEST, err_code);
//
//	APP_ERROR_CHECK(err_code);
//
//}

#ifdef NRF528XX_FEM

void fem_init(void) {

	ret_code_t err_code;
	ble_opt_t opt;
	uint32_t gpiote_ch;
	nrf_ppi_channel_t ppi_set_ch;
	nrf_ppi_channel_t ppi_clr_ch;
	nrf_drv_gpiote_out_config_t config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false);

	nrf_gpio_cfg_output(FEM_ANT_SEL);
	nrf_gpio_pin_clear(FEM_ANT_SEL);

	nrf_gpio_cfg_output(FEM_CSD);
	nrf_gpio_pin_set(FEM_CSD);

	nrf_gpio_cfg_output(FEM_CHL);
	nrf_gpio_pin_set(FEM_CHL);

	nrf_gpio_cfg_output(FEM_CPS);

	nrf_gpio_pin_clear(FEM_CPS);

	nrf_gpio_cfg_output(FEM_CTX);
	nrf_gpio_pin_clear(FEM_CTX);

	nrf_gpio_cfg_output(FEM_CRX);
	nrf_gpio_pin_clear(FEM_CRX);

	err_code = nrf_drv_gpiote_init();

	if (err_code != NRF_ERROR_INVALID_STATE) {

		APP_ERROR_CHECK(err_code);

	}

	err_code = nrf_drv_ppi_init();

	if (err_code != NRF_ERROR_INVALID_STATE) {

		APP_ERROR_CHECK(err_code);

	}

	err_code = nrf_drv_ppi_channel_alloc(&ppi_set_ch);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_ppi_channel_alloc(&ppi_clr_ch);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_gpiote_out_init(FEM_CRX, &config);
	APP_ERROR_CHECK(err_code);

	gpiote_ch = nrf_drv_gpiote_out_task_addr_get(FEM_CRX);

	memset(&opt, 0, sizeof(ble_opt_t));
	opt.common_opt.pa_lna.gpiote_ch_id = (gpiote_ch - NRF_GPIOTE_BASE) >> 2; // GPIOTE channel used for radio pin toggling
	opt.common_opt.pa_lna.ppi_ch_id_clr = ppi_clr_ch; // PPI channel used for radio pin clearing
	opt.common_opt.pa_lna.ppi_ch_id_set = ppi_set_ch; // PPI channel used for radio pin setting

	opt.common_opt.pa_lna.pa_cfg.active_high = 1; // Set the pin to be active high
	opt.common_opt.pa_lna.pa_cfg.enable = 1;              // Enable toggling
	opt.common_opt.pa_lna.pa_cfg.gpio_pin = FEM_CTX; // The GPIO pin to toggle

	opt.common_opt.pa_lna.lna_cfg.active_high = 1; // Set the pin to be active high
	opt.common_opt.pa_lna.lna_cfg.enable = 1;             // Enable toggling
	opt.common_opt.pa_lna.lna_cfg.gpio_pin = FEM_CRX; // The GPIO pin to toggle

	err_code = sd_ble_opt_set(BLE_COMMON_OPT_PA_LNA, &opt);
	APP_ERROR_CHECK(err_code);

}

#endif

static void gpio_init() {

	nrf_gpio_cfg_output(GPIO_LED_0);
	nrf_gpio_pin_clear(GPIO_LED_0);

	nrf_gpio_cfg_output(GPIO_LED_1);
	nrf_gpio_pin_clear(GPIO_LED_1);

	nrf_gpio_cfg_output(GPIO_BAT_EN);
	nrf_gpio_pin_clear(GPIO_BAT_EN);

	nrf_gpio_cfg_output(GPIO_SENSOR_POWER_EN);
	nrf_gpio_pin_clear(GPIO_SENSOR_POWER_EN);

	nrf_gpio_cfg_input(GPIO_BTN, NRF_GPIO_PIN_PULLUP);

}

/**@brief Function for application main entry.
 */
int main(void) {

	uint8_t battery_read_step = 0x00;

	log_init();

	timers_init();

	gpio_init();

	twi_init();

	spi_init();

	init_uart_1();

	power_management_init();

	ble_stack_init();

#ifdef NRF528XX_FEM

	fem_init();

#endif

	gap_params_init();

	gatt_init();

	services_init();

	advertising_init();

	conn_params_init();

	advertising_start();

	app_timer_start(m_app_timer_id, APP_TIMER_DELAY, NULL);

	set_uart_usb_in_out_write_string("Start AxDen Smart ECG PPG example");

	get_mac_address();

	set_uart_usb_in_out_mac_address(mac_address);

	nrf_gpio_pin_set(GPIO_SENSOR_POWER_EN);

	wait_200ms();

	twi_scan();

	set_max30101_i2c_instance(m_twi);

	init_max30101();

	set_si7051_i2c_instance(m_twi);

	init_si7051();

	wait_200ms();

	get_si7051_temperature(&temperature);

	set_max30003_spi_instance(m_spi);

	uint8_t rev_id = get_max30003_rev_id();
	NRF_LOG_INFO("MAX30003 REV ID %02X", rev_id);
	NRF_LOG_FLUSH();

	init_max30003_mode_0();

	nrf_gpio_pin_set(GPIO_BAT_EN);

	wait_200ms();

	read_battery_level();

	NRF_LOG_INFO("Print sensor value");

	set_uart_usb_in_out_battery(battery_value);

	set_uart_usb_in_out_temperature(temperature);

	NRF_LOG_INFO("Start main loop");

	radio_packet_protocol.Packet.company_id[0] = COMPANY_ID >> 8;
	radio_packet_protocol.Packet.company_id[1] = COMPANY_ID;
	radio_packet_protocol.Packet.device_id[0] = DEVICE_TYPE >> 8;
	radio_packet_protocol.Packet.device_id[1] = DEVICE_TYPE;
	memcpy(radio_packet_protocol.Packet.mac_address, mac_address, 8);
	radio_packet_protocol.Packet.control_number = 0;

	for (;;) {

		if (!nrf_gpio_pin_read(GPIO_BTN)) {

			nrf_gpio_pin_toggle(GPIO_LED_0);

		}

		if (collection_cycle_update) {

			if (battery_read_step == 0x00) {

				nrf_gpio_pin_set(GPIO_BAT_EN);

				battery_read_step = 0x01;

			} else {

				read_battery_level();

				get_si7051_temperature(&temperature);

				nrf_gpio_pin_clear(GPIO_LED_1);

				update_ECG();

				update_Heartrate_SPO2();

				if (m_conn_handle != BLE_CONN_HANDLE_INVALID) {

					if (notify_enable_state) {

						payload_size = 0;
						memset(payload, 0x00, sizeof(payload));

						payload_size = sprintf((char*) payload, "%d.%d,",
								(battery_value / 10), (battery_value % 10));

						payload_size += sprintf((char*) payload + payload_size,
								"%d.%d,", (temperature / 10),
								abs(temperature % 10));

						payload_size += sprintf((char*) payload + payload_size,
								"%d,", green_heart_rate);

						payload_size += sprintf((char*) payload + payload_size,
								"%d,", green_heart_rate_count);

						payload_size += sprintf((char*) payload + payload_size,
								"%d,", ir_heart_rate);

						payload_size += sprintf((char*) payload + payload_size,
								"%d,", ir_heart_rate_count);

						payload_size += sprintf((char*) payload + payload_size,
								"%d,", spo2);

						payload_size += sprintf((char*) payload + payload_size,
								"%d,", spo2_count);

						payload_size += sprintf((char*) payload + payload_size,
								"%d,", ecg_heart_rate);

						payload_size += sprintf((char*) payload + payload_size,
								"%d,", ecg_heart_rate_count);

						radio_packet_protocol_size = PACKET_HEADER_SIZE;
						radio_packet_protocol_size += payload_size;
						memcpy(radio_packet_protocol.Packet.payload, payload,
								payload_size);

						ble_service_send_notification(m_conn_handle, &m_service,
								radio_packet_protocol.buffer,
								radio_packet_protocol_size);

					}

				}

				battery_read_step = 0x00;

				collection_cycle_update = 0x00;

			}

		}

		while (!app_timer_update) {

			idle_state_handle();

		}

		app_timer_update = 0x00;

		nrf_gpio_pin_toggle(GPIO_LED_1);

		NRF_LOG_FLUSH();

	}
}

/**
 * @}
 */
