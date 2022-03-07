#ifndef MAX30003_H_
#define MAX30003_H_

#include <stdint.h>
#include <stdbool.h>
#include "board_define.h"
#include "nrf_drv_spi.h"

extern volatile uint8_t max30003_100_ms_timer;
extern volatile uint8_t spi_xfer_done;

void set_max30003_spi_instance(nrf_drv_spi_t spi_instance);

uint8_t get_max30003_rev_id();

uint8_t init_max30003_mode_0();

uint8_t init_max30003_mode_1();

uint8_t set_max30003_sync();

uint8_t set_max30003_fifo_reset();

uint8_t get_max30003_ecg_voltage_sample(int32_t *value);

#endif /* MAX30003_H_ */
