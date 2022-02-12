#ifndef UART_USB_IN_OUT_H_
#define UART_USB_IN_OUT_H_

#include <stdint.h>
#include <stdbool.h>
#include "board_define.h"

void set_uart_usb_in_out_write_string(char *data);

void set_uart_usb_in_out_find_i2c_address(uint8_t address);

void set_uart_usb_in_out_mac_address(uint8_t *mac_address);

void set_uart_usb_in_out_temperature(int16_t temperature);

void set_uart_usb_in_out_pressure(uint32_t pressure);

void set_uart_usb_in_out_humidity(uint8_t humidity);

void set_uart_usb_in_out_lux(uint16_t lux);

void set_uart_usb_in_out_battery(uint8_t battery);

void set_uart_usb_in_out_ir_heartrate(uint8_t heartrate, uint8_t count);

void set_uart_usb_in_out_green_heartrate(uint8_t heartrate, uint8_t count);

void set_uart_usb_in_out_spo2(uint8_t spo2, uint8_t count);

void set_uart_usb_in_out_ecg_heartrate(uint8_t heartrate, uint8_t count);

#endif /* UART_USB_IN_OUT_H_ */
