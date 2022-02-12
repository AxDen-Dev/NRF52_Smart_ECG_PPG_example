#ifndef MAX30101_H_
#define MAX30101_H_

#include <stdint.h>
#include "board_define.h"
#include "nrf_drv_twi.h"

extern volatile uint8_t max30101_100_ms_timer;
extern volatile uint8_t twi_read_done;
extern volatile uint8_t twi_write_done;
extern volatile uint8_t twi_address_nack;

void set_max30101_i2c_instance(nrf_drv_twi_t twi_instance);

uint8_t init_max30101();

uint8_t set_max30101_normal_mode();

uint8_t set_max30101_sleep_mode();

uint8_t get_max30101_ir_red_green(uint32_t *ir, uint32_t *red, uint32_t *green);

#endif /* MAX30101_H_ */
