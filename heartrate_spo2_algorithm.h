
#ifndef HEARTRATE_SPO2_ALGORITHM_H_
#define HEARTRATE_SPO2_ALGORITHM_H_

#include <stdio.h>
#include <stdlib.h>

#define MEAN_FILTER_SIZE 10
#define ALPHA 0.8

#define RED_IR_AC_BUFFER_SIZE 12

#define SPO2_MIN_FIND_WAIT_COUNT 80

#define HISTOGRAM_SIZE 16

#define HEARTRATE_MAX_COUNT 200

#define MAX30101_GREEN_PROXY_THR 6000
#define MAX30101_GREEN_PROXY_ERROR_COUNT 6

typedef struct MeanDiffFilte
{
    float values[MEAN_FILTER_SIZE];
    uint8_t index;
    float sum;
    uint8_t count;
} meanDiffFilter_t;

typedef struct ButterworthFilter
{
    float v[2];
    float result;

} butterworthFilter_t;

typedef struct DcFilter
{
    float w;
    float result;
} dcFilter_t;

void updateSpo2RedIrAcc(void);

void getSpo2Value(uint8_t *spo2, uint8_t *count);

uint8_t updateHeartateIR(uint32_t ir, uint32_t red);

uint8_t updateHeartateGreen(uint32_t green);

void initHeartrate(void);

uint8_t getIrHeartrate(void);

uint8_t getGreenHeartrate(void);

void setHeartrateMinMax(uint8_t min, uint8_t max);

void getHeartrateValue(uint8_t *irHr, uint8_t *irHrCount, uint8_t *greenHr,
                       uint8_t *greenHrCount);

#endif /* HEARTRATE_SPO2_ALGORITHM_H_ */
