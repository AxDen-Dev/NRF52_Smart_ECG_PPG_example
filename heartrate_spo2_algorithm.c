
#include "heartrate_spo2_algorithm.h"
#include <string.h>
#include <math.h>

#define min(a,b)  (((a) < (b)) ? (a) : (b))
#define max(a,b)  (((a) > (b)) ? (a) : (b))

static meanDiffFilter_t meanDiffIR;
static meanDiffFilter_t meanDiffGreen;

static butterworthFilter_t lpbFilterIR;
static butterworthFilter_t lpbFilterGreen;

static dcFilter_t dcIR;
static dcFilter_t dcGreen;

static uint32_t startGreenTime = 0;
static uint32_t stopGreenTime = 0;
static uint8_t findGreenPositiveMax = 0x00;
static int32_t positiveGreenMax = 0;
static uint8_t enterPositiveGreen = 0x00;
static uint8_t enterNegativeGreen = 0x00;
static uint8_t findGreenNegativeMax = 0x00;
static uint32_t positiveGreenMaxTimeCount = 0;
static uint32_t negativeGreenMaxTimeCount = 0;
static uint32_t prevPositiveGreenMaxTimeCount = 0;
static uint32_t prevNegativeGreenMaxTimeCount = 0;
static int32_t negativeGreenMax = 0;
static float greenHR = 0;

static uint32_t startIrTime = 0;
static uint32_t stopIrTime = 0;
static uint8_t findIrPositiveMax = 0x00;
static int32_t positiveIrMax = 0;
static uint8_t enterPositiveIr = 0x00;
static uint8_t enterNegativeIr = 0x00;
static uint8_t findIrNegativeMax = 0x00;
static uint32_t positiveIrMaxTimeCount = 0;
static uint32_t negativeIrMaxTimeCount = 0;
static uint32_t prevPositiveIrMaxTimeCount = 0;
static uint32_t prevNegativeIrMaxTimeCount = 0;
static int32_t negativeIrMax = 0;
static float irHR = 0;

static uint8_t heartrateMaxLimit = 180;
static uint8_t heartrateMinLimit = 40;

static uint16_t irPpgHistogramCount[HISTOGRAM_SIZE];
static uint32_t irPpgHistogramValue[HISTOGRAM_SIZE];

static uint16_t greenPpgHistogramCount[HISTOGRAM_SIZE];
static uint32_t greenPpgHistogramValue[HISTOGRAM_SIZE];

static uint32_t rawAcIrValue = 0;
static uint32_t rawAcRedValue = 0;

static uint32_t irAcPeakValue = 0;
static uint32_t redAcPeakValue = 0;

static uint32_t irAcPeakTimeCountValue = 0;
static uint32_t redAcPeakTimeCountValue = 0;

static uint8_t irRedAcBufferIndex = 0;
static uint32_t irAcArrayBuffer[RED_IR_AC_BUFFER_SIZE];
static uint32_t redAcArrayBuffer[RED_IR_AC_BUFFER_SIZE];

static uint32_t acTimeCount = 0;
static uint32_t irAcTimeIndexArrayBuffer[RED_IR_AC_BUFFER_SIZE];
static uint32_t redAcTimeIndexArrayBuffer[RED_IR_AC_BUFFER_SIZE];

static uint8_t redIrAcSpo2MinFindState = 0x00;
static uint8_t redIrAcSpo2MinFindWaitCount = 0;
static uint32_t irAcSpo2MinValue = 0;
static uint32_t irAcSpo2MinTimeCountValue = 0;
static uint32_t redAcSpo2MinValue = 0;
static uint32_t redAcSpo2MinTimeCountValue = 0;

static float averageSpo2Value = 0;
static uint16_t averageSpo2Count = 0;

static dcFilter_t dcRemoval(float x, float prev_w, float alpha)
{
    dcFilter_t filtered;
    filtered.w = x + alpha * prev_w;
    filtered.result = filtered.w - prev_w;

    return filtered;
}

static void lowPassButterworthFilter(float x, butterworthFilter_t *filterResult)
{
    filterResult->v[0] = filterResult->v[1];

    filterResult->v[1] = (1.602003508877367088e-1 * x)
            + (0.67959929822452658232f * filterResult->v[0]);

    filterResult->result = filterResult->v[0] + filterResult->v[1];

}

static float meanDiffFilter(float M, meanDiffFilter_t *filterValues)
{

    float avg = 0;

    filterValues->sum -= filterValues->values[filterValues->index];
    filterValues->values[filterValues->index] = M;
    filterValues->sum += filterValues->values[filterValues->index];

    filterValues->index++;
    filterValues->index = filterValues->index % MEAN_FILTER_SIZE;

    if (filterValues->count < MEAN_FILTER_SIZE)
        filterValues->count++;

    avg = filterValues->sum / filterValues->count;
    return avg - M;

}

static void redIrAcBufferUpdate(void)
{

    irAcArrayBuffer[irRedAcBufferIndex] = rawAcIrValue;
    redAcArrayBuffer[irRedAcBufferIndex] = rawAcRedValue;

    irAcTimeIndexArrayBuffer[irRedAcBufferIndex] = acTimeCount;
    redAcTimeIndexArrayBuffer[irRedAcBufferIndex] = acTimeCount;

    irRedAcBufferIndex += 1;

    if (irRedAcBufferIndex >= RED_IR_AC_BUFFER_SIZE)
    {

        irRedAcBufferIndex = 0;

    }

}

static uint8_t checkForBeatIR(float sample)
{

    if (sample > 0)
    {

        if (enterNegativeIr)
        {

            if (!findIrNegativeMax)
            {

                findIrNegativeMax = 0x00;
                findIrPositiveMax = 0x00;
                enterPositiveIr = 0x00;
                enterNegativeIr = 0x00;
                positiveIrMax = 0;
                negativeIrMax = 0;

                return 0x03;

            }

        }

        if (findIrNegativeMax)
        {

            if ((positiveIrMax - negativeIrMax) >= 15
                    && (positiveIrMax - negativeIrMax) <= 3000)
            {

                findIrNegativeMax = 0x00;
                findIrPositiveMax = 0x00;
                enterPositiveIr = 0x00;
                enterNegativeIr = 0x00;
                positiveIrMax = 0;
                negativeIrMax = 0;

                return 0x01;

            }
            else
            {

                findIrNegativeMax = 0x00;
                findIrPositiveMax = 0x00;
                enterPositiveIr = 0x00;
                enterNegativeIr = 0x00;
                positiveIrMax = 0;
                negativeIrMax = 0;

                return 0x03;

            }

        }

        enterPositiveIr = 0x01;

        if (sample >= 10)
        {

            if (sample > positiveIrMax)
            {

                positiveIrMax = sample;
                positiveIrMaxTimeCount = acTimeCount;

                findIrPositiveMax = 0x01;

            }

        }

    }
    else if (sample < 0)
    {

        if (enterPositiveIr)
        {

            if (!findIrPositiveMax)
            {

                findIrNegativeMax = 0x00;
                findIrPositiveMax = 0x00;
                enterPositiveIr = 0x00;
                enterNegativeIr = 0x00;
                positiveIrMax = 0;
                negativeIrMax = 0;

                return 0x03;

            }

        }

        enterNegativeIr = 0x01;

        if (sample <= -5)
        {

            if (sample < negativeIrMax)
            {

                negativeIrMax = sample;
                negativeIrMaxTimeCount = acTimeCount;

                findIrNegativeMax = 0x01;

            }

        }

    }

    return 0x02;

}

static uint8_t checkForBeatGreen(float sample)
{

    if (sample > 0)
    {

        if (enterNegativeGreen)
        {

            if (!findGreenNegativeMax)
            {

                findGreenNegativeMax = 0x00;
                findGreenPositiveMax = 0x00;
                enterPositiveGreen = 0x00;
                enterNegativeGreen = 0x00;
                positiveGreenMax = 0;
                negativeGreenMax = 0;

                return 0x03;

            }

        }

        if (findGreenNegativeMax)
        {

            if ((positiveGreenMax - negativeGreenMax) >= 15
                    && (positiveGreenMax - negativeGreenMax) <= 3000)
            {

                findGreenNegativeMax = 0x00;
                findGreenPositiveMax = 0x00;
                enterPositiveGreen = 0x00;
                enterNegativeGreen = 0x00;
                positiveGreenMax = 0;
                negativeGreenMax = 0;

                return 0x01;

            }
            else
            {

                findGreenNegativeMax = 0x00;
                findGreenPositiveMax = 0x00;
                enterPositiveGreen = 0x00;
                enterNegativeGreen = 0x00;
                positiveGreenMax = 0;
                negativeGreenMax = 0;

                return 0x03;

            }

        }

        enterPositiveGreen = 0x01;

        if (sample >= 10)
        {

            if (sample > positiveGreenMax)
            {

                positiveGreenMax = sample;
                positiveGreenMaxTimeCount = acTimeCount;

                findGreenPositiveMax = 0x01;

            }

        }

    }
    else if (sample < 0)
    {

        if (enterPositiveGreen)
        {

            if (!findGreenPositiveMax)
            {

                findGreenNegativeMax = 0x00;
                findGreenPositiveMax = 0x00;
                enterPositiveGreen = 0x00;
                enterNegativeGreen = 0x00;
                positiveGreenMax = 0;
                negativeGreenMax = 0;

                return 0x03;

            }

        }

        enterNegativeGreen = 0x01;

        if (sample <= -5)
        {

            if (sample < negativeGreenMax)
            {

                negativeGreenMax = sample;
                negativeGreenMaxTimeCount = acTimeCount;

                findGreenNegativeMax = 0x01;

            }

        }

    }

    return 0x02;

}

void updateSpo2RedIrAcc(void)
{

    uint32_t irMaxValue = 0;
    uint32_t redMaxValue = 0;

    uint32_t irMaxTimeCountValue = 0;
    uint32_t redMaxTimeCountValue = 0;

    for (uint8_t i = 0; i < RED_IR_AC_BUFFER_SIZE; i++)
    {

        if (irAcArrayBuffer[i] > irMaxValue)
        {

            irMaxValue = irAcArrayBuffer[i];
            irMaxTimeCountValue = irAcTimeIndexArrayBuffer[i];

        }

        if (redAcArrayBuffer[i] > redMaxValue)
        {

            redMaxValue = redAcArrayBuffer[i];
            redMaxTimeCountValue = redAcTimeIndexArrayBuffer[i];

        }

    }

    if (redIrAcSpo2MinFindState == 0x01)
    {

        double irGradientX = (double) irMaxTimeCountValue
                - (double) irAcPeakTimeCountValue;
        double irGradientY = (double) irMaxValue - (double) irAcPeakValue;
        double irGradient = irGradientY / irGradientX;

        double irConstant = irMaxValue - irGradient * irMaxTimeCountValue;

        double redGradientX = (double) redMaxTimeCountValue
                - (double) redAcPeakTimeCountValue;
        double redGradientY = (double) redMaxValue - (double) redAcPeakValue;
        double redGradient = redGradientY / redGradientX;

        double redConstant = redMaxValue - redGradient * redMaxTimeCountValue;

        double irAcSpo2 = irGradient * irAcSpo2MinTimeCountValue + irConstant;
        double redAcSpo2 = redGradient * redAcSpo2MinTimeCountValue
                + redConstant;

        double irAcRatio = (irAcSpo2 - irAcSpo2MinValue) / irAcSpo2MinValue;
        double redRatio = (redAcSpo2 - redAcSpo2MinValue) / redAcSpo2MinValue;

        if (redRatio > 0 && irAcRatio > 0)
        {

            double R = redRatio / irAcRatio;

            float spo2 = (-45.06 * R + 30.354) * R + 94.845;

            if (spo2 >= 94 && spo2 <= 100)
            {

                averageSpo2Value += spo2;
                averageSpo2Count++;

            }

        }

    }

    irAcSpo2MinValue = irMaxValue;
    redAcSpo2MinValue = redMaxValue;

    irAcPeakValue = irMaxValue;
    redAcPeakValue = redMaxValue;

    irAcPeakTimeCountValue = irMaxTimeCountValue;
    redAcPeakTimeCountValue = redMaxTimeCountValue;

    redIrAcSpo2MinFindWaitCount = 0;
    redIrAcSpo2MinFindState = 0x01;

}

void getSpo2Value(uint8_t *spo2, uint8_t *count)
{

    uint32_t avrSpo2 = roundf(averageSpo2Value / averageSpo2Count);

    *spo2 = avrSpo2;

    if (averageSpo2Count > 255)
    {

        averageSpo2Count = 255;

    }

    *count = averageSpo2Count;

}

uint8_t updateHeartateIR(uint32_t ir, uint32_t red)
{

    rawAcIrValue = ir;
    rawAcRedValue = red;

    if (redIrAcSpo2MinFindState == 0x01)
    {

        if (irAcSpo2MinValue > rawAcIrValue)
        {

            irAcSpo2MinValue = rawAcIrValue;
            irAcSpo2MinTimeCountValue = rawAcRedValue;

        }

        if (redAcSpo2MinValue > rawAcRedValue)
        {

            redAcSpo2MinValue = rawAcRedValue;
            redAcSpo2MinTimeCountValue = rawAcRedValue;

        }

        if (redIrAcSpo2MinFindWaitCount > SPO2_MIN_FIND_WAIT_COUNT)
        {

            redIrAcSpo2MinFindState = 0x00;
            redIrAcSpo2MinFindWaitCount = 0;

        }

        redIrAcSpo2MinFindWaitCount++;

    }

    redIrAcBufferUpdate();

    dcIR = dcRemoval((float) ir, dcIR.w, ALPHA);

    float meanDiffResIR = meanDiffFilter(dcIR.result, &meanDiffIR);

    lowPassButterworthFilter(meanDiffResIR, &lpbFilterIR);

    uint8_t state = checkForBeatIR(lpbFilterIR.result);

    if (state == 1)
    {

        if (startIrTime == 0)
        {

            prevPositiveIrMaxTimeCount = positiveIrMaxTimeCount;
            prevNegativeIrMaxTimeCount = negativeIrMaxTimeCount;
            startIrTime = 1;

        }
        else
        {

            int32_t totalTimeCount = positiveIrMaxTimeCount
                    - prevPositiveIrMaxTimeCount;

            prevPositiveIrMaxTimeCount = positiveIrMaxTimeCount;
            prevNegativeIrMaxTimeCount = negativeIrMaxTimeCount;
            startIrTime = 1;

            if (totalTimeCount > 0)
            {

                irHR = 60.0f / (totalTimeCount * 0.02f);

                if (irHR >= heartrateMinLimit && irHR <= heartrateMaxLimit)
                {

                    int32_t histogramPpgValue = irHR / 10;
                    histogramPpgValue -= 2;

                    if (histogramPpgValue < 0)
                    {

                        histogramPpgValue = 0;

                    }
                    else if (histogramPpgValue >= HISTOGRAM_SIZE)
                    {

                        histogramPpgValue = HISTOGRAM_SIZE - 1;

                    }

                    irPpgHistogramValue[histogramPpgValue] += irHR;
                    irPpgHistogramCount[histogramPpgValue] += 1;

                    return 0x01;

                }

            }

        }

    }
    else if (state == 0x03)
    {

        startIrTime = 0;

    }

    return 0x00;

}

uint8_t updateHeartateGreen(uint32_t green)
{

    acTimeCount++;

    dcGreen = dcRemoval((float) green, dcGreen.w, ALPHA);

    float meanDiffResGreen = meanDiffFilter(dcGreen.result, &meanDiffGreen);

    lowPassButterworthFilter(meanDiffResGreen, &lpbFilterGreen);

    uint8_t state = checkForBeatGreen(lpbFilterGreen.result);

    if (state == 1)
    {

        if (startGreenTime == 0)
        {

            prevPositiveGreenMaxTimeCount = positiveGreenMaxTimeCount;
            prevNegativeGreenMaxTimeCount = negativeGreenMaxTimeCount;
            startGreenTime = 1;

        }
        else
        {

            int32_t totalTimeCount = positiveGreenMaxTimeCount
                    - prevPositiveGreenMaxTimeCount;

            prevPositiveGreenMaxTimeCount = positiveGreenMaxTimeCount;
            prevNegativeGreenMaxTimeCount = negativeGreenMaxTimeCount;
            startGreenTime = 1;

            if (totalTimeCount > 0)
            {

                greenHR = 60.0f / (totalTimeCount * 0.02f);

                if (greenHR >= heartrateMinLimit
                        && greenHR <= heartrateMaxLimit)
                {

                    int32_t histogramPpgValue = greenHR / 10;
                    histogramPpgValue -= 2;

                    if (histogramPpgValue < 0)
                    {

                        histogramPpgValue = 0;

                    }
                    else if (histogramPpgValue >= HISTOGRAM_SIZE)
                    {

                        histogramPpgValue = HISTOGRAM_SIZE - 1;

                    }

                    greenPpgHistogramValue[histogramPpgValue] += greenHR;
                    greenPpgHistogramCount[histogramPpgValue] += 1;

                    return 0x01;

                }

            }

        }

    }
    else if (state == 3)
    {

        startGreenTime = 0;

    }

    return 0x00;

}

void initHeartrate(void)
{

    startGreenTime = 0;
    stopGreenTime = 0;
    findGreenPositiveMax = 0x00;
    positiveGreenMax = 0;
    enterPositiveGreen = 0x00;
    enterNegativeGreen = 0x00;
    findGreenNegativeMax = 0x00;
    negativeGreenMax = 0;
    prevPositiveGreenMaxTimeCount = 0;
    positiveGreenMaxTimeCount = 0;
    prevNegativeGreenMaxTimeCount = 0;
    negativeGreenMaxTimeCount = 0;
    greenHR = 0;

    startIrTime = 0;
    stopIrTime = 0;
    findIrPositiveMax = 0x00;
    positiveIrMax = 0;
    enterPositiveIr = 0x00;
    enterNegativeIr = 0x00;
    findIrNegativeMax = 0x00;
    negativeIrMax = 0;
    positiveIrMaxTimeCount = 0;
    negativeIrMaxTimeCount = 0;
    prevPositiveIrMaxTimeCount = 0;
    prevNegativeIrMaxTimeCount = 0;
    irHR = 0;

    redIrAcSpo2MinFindState = 0x00;
    irAcSpo2MinValue = 0;
    redAcSpo2MinValue = 0;

    averageSpo2Value = 0;
    averageSpo2Count = 0;

    irAcSpo2MinValue = 0;
    redAcSpo2MinValue = 0;

    irAcPeakValue = 0;
    redAcPeakValue = 0;

    irAcPeakTimeCountValue = 0;
    redAcPeakTimeCountValue = 0;

    irAcSpo2MinTimeCountValue = 0;
    redAcSpo2MinTimeCountValue = 0;

    redIrAcSpo2MinFindWaitCount = 0;
    redIrAcSpo2MinFindState = 0x00;

    irRedAcBufferIndex = 0;
    memset(irAcArrayBuffer, 0x00, sizeof(irAcArrayBuffer));
    memset(redAcArrayBuffer, 0x00, sizeof(redAcArrayBuffer));

    acTimeCount = 0;
    memset(irAcTimeIndexArrayBuffer, 0x00, sizeof(irAcTimeIndexArrayBuffer));
    memset(redAcTimeIndexArrayBuffer, 0x00, sizeof(redAcTimeIndexArrayBuffer));

    memset(irPpgHistogramCount, 0x00, sizeof(irPpgHistogramCount));
    memset(irPpgHistogramValue, 0x00, sizeof(irPpgHistogramValue));

    memset(greenPpgHistogramCount, 0x00, sizeof(greenPpgHistogramCount));
    memset(greenPpgHistogramValue, 0x00, sizeof(greenPpgHistogramValue));

}

uint8_t getIrHeartrate(void)
{

    return irHR;

}

uint8_t getGreenHeartrate(void)
{

    return greenHR;

}

void setHeartrateMinMax(uint8_t min, uint8_t max)
{

    heartrateMinLimit = min;
    heartrateMaxLimit = max;

}

void getHeartrateValue(uint8_t *irHr, uint8_t *irHrCount, uint8_t *greenHr,
                       uint8_t *greenHrCount)
{

    uint32_t avrIrHr = 0;
    uint32_t avrIrHrCount = 0;

    uint32_t avrGreenHr = 0;
    uint32_t avrGreenHrCount = 0;

    uint16_t greenMaxCount = 0;

    for (uint8_t i = 0; i < HISTOGRAM_SIZE; i++)
    {

        if (greenPpgHistogramCount[i] > greenMaxCount)
        {

            avrIrHr = irPpgHistogramValue[i];
            avrIrHrCount = irPpgHistogramCount[i];

            avrGreenHr = greenPpgHistogramValue[i];
            avrGreenHrCount = greenPpgHistogramCount[i];

            greenMaxCount = greenPpgHistogramCount[i];

        }

    }

    if (avrGreenHrCount == 0)
    {

        avrIrHr = 0;
        avrIrHrCount = 0;

        avrGreenHr = 0;
        avrGreenHrCount = 0;

    }
    else
    {

        avrGreenHr /= avrGreenHrCount;

        if (avrGreenHrCount > 255)
        {

            avrGreenHrCount = 255;

        }

        if (avrIrHrCount == 0)
        {

            avrIrHr = 0;
            avrIrHrCount = 0;

        }
        else
        {

            avrIrHr /= avrIrHrCount;

            if (avrIrHrCount > 255)
            {

                avrIrHrCount = 255;

            }

        }

    }

    *irHr = avrIrHr;
    *irHrCount = avrIrHrCount;

    *greenHr = avrGreenHr;
    *greenHrCount = avrGreenHrCount;

}
