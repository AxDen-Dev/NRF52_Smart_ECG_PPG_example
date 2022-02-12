#include "math.h"
#include "string.h"
#include "stdint.h"

#include "ecg_algorithm.h"

#define ABS(a)  (a) < 0 ? -(a) : (a)

#define FILTER_INIT_COUNT 255
#define DC_ALPHA 0.8
#define MEAN_FILTER_SIZE 4

#define PI 3.14159265358f

#define MOVEING_WINDOW_SIZE 4
#define HISTOGRAM_SIZE 16

#define THRESHOLD_LEVEL_INIT 300000
#define SIGNAL_LEVEL_INIT 500000
#define NOISE_LEVEL_INIT 100000

#define NO_SIGNAL_LEVEL 250000
#define SIGNAL_RESET_LEVEL 1000000

#define PEACK_DECTION_RESET_COUNT 625

typedef struct DcFilter {
	double w;
	double result;
} dcFilter_t;

typedef struct LowpassFilter1p {

	double pt1K, pt1Dt, pt1RC;
	double pt1PrevInput;

} lowpassFilter1p;

typedef struct MeanDiffFilte {
	double values[MEAN_FILTER_SIZE];
	uint8_t index;
	double sum;
	uint8_t count;
} meanDiffFilter_t;

typedef struct ButterworthFilter {
	double v[2];
	double result;

} butterworthFilter_t;

static dcFilter_t dcFilter;
static lowpassFilter1p lowpassFilter;
static meanDiffFilter_t meanDiff;
static butterworthFilter_t butterworthFilter;

static uint16_t filter_init_count = 0;
static uint32_t ecg_heartrate = 0;

static uint32_t startEcgTime = 0;
static uint32_t beatToBeatTime = 0;
static uint32_t ecgHeartRate = 0;

static uint32_t pan_tompkins_threshold = THRESHOLD_LEVEL_INIT;
static uint32_t pan_tompkins_signal_level = SIGNAL_LEVEL_INIT;
static uint32_t pan_tompkins_noise_level = NOISE_LEVEL_INIT;
static uint8_t pan_tompkins_peak_find = 0x00;
static uint32_t pan_tompkins_peak_value = 0;

static uint32_t current_ecg_max_value_find_time = 0;
static uint32_t prev_ecg_max_value_find_time = 0;

static uint8_t moveing_window_index = 0;
static uint32_t moveing_window[MOVEING_WINDOW_SIZE] = { 0x00 };
static uint32_t moveing_window_filter_value = 0;

static uint32_t peak_dection_reset_count = 0;

static uint16_t ecgPpgHistogramCount[HISTOGRAM_SIZE];
static uint32_t ecgPpgHistogramValue[HISTOGRAM_SIZE];

static dcFilter_t dcRemoval(double x, double prev_w, double alpha) {
	dcFilter_t filtered;
	filtered.w = x + alpha * prev_w;
	filtered.result = filtered.w - prev_w;

	return filtered;
}

static void lowpassFilterInit(float cut, float dt, lowpassFilter1p *filter) {

	filter->pt1RC = 1.0f / (2.0f * PI * cut);
	filter->pt1Dt = dt;
	filter->pt1K = filter->pt1Dt / (filter->pt1RC + filter->pt1Dt);

	filter->pt1PrevInput = 0.0f;

}

double pt1FilterApply(double input, lowpassFilter1p *filter) {

	double output = filter->pt1PrevInput
			+ filter->pt1K * (input - filter->pt1PrevInput);
	filter->pt1PrevInput = output;

	return output;

}

static void lowPassButterworthFilter(float x, butterworthFilter_t *filterResult) {

	filterResult->v[0] = filterResult->v[1];

	filterResult->v[1] = (7.547627247472143974e-1 * x)
			+ (-0.50952544949442879485 * filterResult->v[0]);

	filterResult->result = filterResult->v[0] + filterResult->v[1];

}

static double meanDiffFilter(float M, meanDiffFilter_t *filterValues) {

	double avg = 0;

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

static uint8_t checkForBeatECG(int32_t sample) {

	uint8_t valid_value = 0x02;
	uint32_t squaring = ABS(sample);

	moveing_window[moveing_window_index++] = squaring;

	if (moveing_window_index >= MOVEING_WINDOW_SIZE) {

		moveing_window_index = 0;

	}

	uint64_t moveing_window_average = 0;

	for (uint8_t i = 0; i < MOVEING_WINDOW_SIZE; i++) {

		moveing_window_average += moveing_window[i];

	}

	moveing_window_average /= MOVEING_WINDOW_SIZE;
	moveing_window_filter_value = moveing_window_average;

	if (moveing_window_average >= NO_SIGNAL_LEVEL) {

		if (moveing_window_average > pan_tompkins_peak_value) {

			pan_tompkins_peak_value = moveing_window_average;

		}

		pan_tompkins_peak_find = 0x01;

	} else {

		if (pan_tompkins_peak_find) {

			pan_tompkins_threshold = pan_tompkins_noise_level
					+ 0.25
							* (pan_tompkins_signal_level
									- pan_tompkins_noise_level);

			if (pan_tompkins_peak_value >= pan_tompkins_threshold) {

				current_ecg_max_value_find_time = beatToBeatTime;

				valid_value = 0x01;

			} else {

				valid_value = 0x03;

			}

			if (valid_value == 0x01) {

				pan_tompkins_signal_level = 0.125 * pan_tompkins_peak_value
						+ 0.875 * pan_tompkins_signal_level;

			} else {

				pan_tompkins_noise_level = 0.125 * pan_tompkins_peak_value
						+ 0.875 * pan_tompkins_noise_level;

			}

		}

		pan_tompkins_peak_value = 0;
		pan_tompkins_peak_find = 0x00;

	}

	if (moveing_window_filter_value >= SIGNAL_RESET_LEVEL) {

		pan_tompkins_threshold = THRESHOLD_LEVEL_INIT;
		pan_tompkins_signal_level = SIGNAL_LEVEL_INIT;
		pan_tompkins_noise_level = NOISE_LEVEL_INIT;

	}

	if (pan_tompkins_noise_level >= pan_tompkins_signal_level) {

		pan_tompkins_threshold = THRESHOLD_LEVEL_INIT;
		pan_tompkins_signal_level = SIGNAL_LEVEL_INIT;
		pan_tompkins_noise_level = NOISE_LEVEL_INIT;

	}

	return valid_value;

}

void init_ecg_algorithm() {

	lowpassFilter.pt1Dt = 0;
	lowpassFilter.pt1K = 0;
	lowpassFilter.pt1PrevInput = 0;
	lowpassFilter.pt1RC = 0;

	lowpassFilterInit(150.0f, 0.008, &lowpassFilter);

	filter_init_count = 0;
	ecg_heartrate = 0;

	startEcgTime = 0;
	beatToBeatTime = 0;
	ecgHeartRate = 0;

	memset(ecgPpgHistogramValue, 0x00, sizeof(ecgPpgHistogramValue));
	memset(ecgPpgHistogramCount, 0x00, sizeof(ecgPpgHistogramCount));

	moveing_window_index = 0;
	memset(moveing_window, 0x00, sizeof(moveing_window));

	pan_tompkins_threshold = THRESHOLD_LEVEL_INIT;
	pan_tompkins_signal_level = SIGNAL_LEVEL_INIT;
	pan_tompkins_noise_level = NOISE_LEVEL_INIT;

	peak_dection_reset_count = 0;

}

int32_t update_ecg_algorithm(int32_t value) {

	beatToBeatTime++;

	peak_dection_reset_count++;

	if (peak_dection_reset_count > PEACK_DECTION_RESET_COUNT) {

		init_ecg_algorithm();

		peak_dection_reset_count = 0;

	}

	value = pt1FilterApply(value, &lowpassFilter);

	dcFilter = dcRemoval((double) value, dcFilter.w, DC_ALPHA);

	value = meanDiffFilter(dcFilter.result, &meanDiff);

	lowPassButterworthFilter(value, &butterworthFilter);

	value = butterworthFilter.result;

	if (filter_init_count > FILTER_INIT_COUNT) {

		uint8_t state = checkForBeatECG(value);

		if (state == 1) {

			if (startEcgTime == 0) {

				prev_ecg_max_value_find_time = current_ecg_max_value_find_time;
				startEcgTime = 1;

			} else {

				uint32_t totalTimeCount = current_ecg_max_value_find_time
						- prev_ecg_max_value_find_time;
				prev_ecg_max_value_find_time = current_ecg_max_value_find_time;

				startEcgTime = 1;

				if (totalTimeCount > 0) {

					ecgHeartRate = 60.0f / (totalTimeCount * 0.008f);

					if (ecgHeartRate >= 20 && ecgHeartRate <= 180) {

						int32_t histogramPpgValue = ecgHeartRate / 10;
						histogramPpgValue -= 2;

						if (histogramPpgValue < 0) {

							histogramPpgValue = 0;

						} else if (histogramPpgValue >= HISTOGRAM_SIZE) {

							histogramPpgValue = HISTOGRAM_SIZE - 1;

						}

						ecgPpgHistogramValue[histogramPpgValue] += ecgHeartRate;
						ecgPpgHistogramCount[histogramPpgValue] += 1;

						peak_dection_reset_count = 0;

					}

				}

			}

		} else if (state == 0x03) {

			beatToBeatTime = 0;
			startEcgTime = 0;

		}

		return value;

	} else {

		filter_init_count++;
		beatToBeatTime = 0;

		return 0;

	}

}

void get_ecg_heartrate(uint8_t *hr, uint8_t *dection_count) {

	uint16_t maxCount = 0;
	uint32_t avrEcgHr = 0;
	uint32_t avrEcgHrCount = 0;

	for (uint8_t i = 0; i < HISTOGRAM_SIZE; i++) {

		if (ecgPpgHistogramCount[i] > maxCount) {

			avrEcgHr = ecgPpgHistogramValue[i];
			avrEcgHrCount = ecgPpgHistogramCount[i];

			maxCount = ecgPpgHistogramCount[i];

		}

	}

	if (avrEcgHrCount > 0) {

		uint32_t value = avrEcgHr / avrEcgHrCount;

		if (value > UINT8_MAX) {

			value = UINT8_MAX;

		}

		*hr = value;

		value = avrEcgHrCount;

		if (value > UINT8_MAX) {

			value = UINT8_MAX;

		}

		*dection_count = value;

	} else {

		*hr = 0;
		*dection_count = 0;

	}

}

uint32_t get_ecg_moving_window_value() {

	return moveing_window_filter_value;

}
