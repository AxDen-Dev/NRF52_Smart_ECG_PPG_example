#ifndef SI7051_H_
#define SI7051_H_

#include <stdint.h>
#include "board_define.h"
#include "nrf_drv_twi.h"

extern volatile uint8_t si7051_100_ms_timer;
extern volatile uint8_t twi_read_done;
extern volatile uint8_t twi_write_done;
extern volatile uint8_t twi_address_nack;

void set_si7051_i2c_instance(nrf_drv_twi_t twi_instance);

uint8_t init_si7051(void);

uint8_t get_si7051_temperature(int16_t *temperature_output);

#endif /* SI7051_H_ */
