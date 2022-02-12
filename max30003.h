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

void init_max30003_mode_0();

void init_max30003_mode_1();

void set_max30003_sync();

void set_max30003_fifo_reset();

int32_t get_max30003_ecg_voltage_sample();

uint32_t get_max30003_rtor_interval();

#endif /* MAX30003_H_ */
