
#ifndef ECG_ALGORITHM_H_
#define ECG_ALGORITHM_H_

#include <stdio.h>
#include <stdlib.h>

void init_ecg_algorithm();

int32_t update_ecg_algorithm(int32_t value);

void get_ecg_heartrate(uint8_t *hr, uint8_t *dection_count);

uint32_t get_ecg_moving_window_value();

#endif /* ECG_ALGORITHM_H_ */
