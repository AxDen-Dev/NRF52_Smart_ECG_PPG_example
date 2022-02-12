#include "si7051.h"
#include "string.h"

#include "nrf_pwr_mgmt.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#define Si7051_ADDRESS 0x40

static nrf_drv_twi_t m_twi;

static void wait_100_ms() {

	si7051_100_ms_timer = 0x00;

	while (!si7051_100_ms_timer) {

		if (NRF_LOG_PROCESS() == false) {

			nrf_pwr_mgmt_run();

		}

	}

}

void set_si7051_i2c_instance(nrf_drv_twi_t twi_instance) {

	m_twi = twi_instance;

}

static uint8_t i2c_write8(uint8_t address, uint8_t reg) {

	uint16_t timeout = UINT16_MAX;
	uint8_t txBuffer = reg;

	twi_write_done = 0x00;
	twi_read_done = 0x00;
	twi_address_nack = 0x00;

	ret_code_t result = nrf_drv_twi_tx(&m_twi, address, &txBuffer, 1, true);

	if (result == NRF_SUCCESS) {

		while (!twi_write_done && --timeout) {

		}

		if (!timeout || twi_address_nack) {

			return 0x00;

		} else {

			return 0x01;

		}

	} else {

		return 0x00;

	}

}

static uint8_t i2c_read_buffer(uint8_t address, uint8_t *read_value,
		const uint8_t buffer_size) {

	uint16_t timeout = UINT16_MAX;
	uint8_t rxBuffer[buffer_size];

	twi_write_done = 0x00;
	twi_read_done = 0x00;
	twi_address_nack = 0x00;

	ret_code_t result = nrf_drv_twi_rx(&m_twi, address, rxBuffer, buffer_size);

	if (result == NRF_SUCCESS) {

		while (!twi_write_done && --timeout) {

		}

		memcpy(read_value, rxBuffer, buffer_size);

		if (!timeout || twi_address_nack) {

			return 0x00;

		} else {

			return 0x01;

		}

	} else {

		return 0x00;

	}

}

uint8_t init_si7051(void) {

	uint8_t i2c_state = i2c_write8(Si7051_ADDRESS, 0xFE);

	for (uint8_t i = 0; i < 2; i++) {

		wait_100_ms();

	}

	return i2c_state;

}

uint8_t get_si7051_temperature(int16_t *temperature_output) {

	uint8_t i2c_state = 0x00;
	uint8_t read_buffer[2] = { 0x00 };

	i2c_state = i2c_write8(Si7051_ADDRESS, 0xF3);

	for (uint8_t i = 0; i < 2; i++) {

		wait_100_ms();

	}

	i2c_state = i2c_state & i2c_read_buffer(Si7051_ADDRESS, read_buffer, 2);

	uint16_t temperature = (read_buffer[0] << 8) + read_buffer[1];
	float temperature_si7051 =
			((((float) temperature * 175.72) / 65536) - 46.85);
	*temperature_output = (int16_t) (temperature_si7051 * 10);

	return i2c_state;

}

