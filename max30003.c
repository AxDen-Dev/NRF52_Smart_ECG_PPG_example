#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "max30003.h"
#include "max30003_define.h"

#include "nrf_gpio.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

static nrf_drv_spi_t m_spi;
static uint8_t max30003_spi_write_buffer[4];
static uint8_t max30003_spi_read_buffer[4];

static void wait_100_ms() {

	max30003_100_ms_timer = 0x00;

	while (!max30003_100_ms_timer) {

		if (NRF_LOG_PROCESS() == false) {

			nrf_pwr_mgmt_run();

		}

	}

}

static void wait_100_ms_timer(uint32_t wait) {

	for (uint32_t i = 0; i < wait; i++) {

		wait_100_ms();

	}

}

void set_max30003_spi_instance(nrf_drv_spi_t spi_instance) {

	m_spi = spi_instance;

}

void max30003_spi_write(uint8_t address, uint32_t data) {

	uint16_t timeout = UINT16_MAX;

	memset(max30003_spi_write_buffer, 0x00, sizeof(max30003_spi_write_buffer));
	memset(max30003_spi_read_buffer, 0x00, sizeof(max30003_spi_read_buffer));

	max30003_spi_write_buffer[0] = (address << 1);
	max30003_spi_write_buffer[1] = ((0x00FF0000 & data) >> 16);
	max30003_spi_write_buffer[2] = ((0x0000FF00 & data) >> 8);
	max30003_spi_write_buffer[3] = (0x000000FF & data);

	spi_xfer_done = 0x00;

	nrf_drv_spi_transfer(&m_spi, max30003_spi_write_buffer, 4,
			max30003_spi_read_buffer, 4);

	while (!spi_xfer_done && --timeout) {

	}

}

void max30003_spi_read(uint8_t address, uint32_t *data) {

	uint16_t timeout = UINT16_MAX;

	memset(max30003_spi_write_buffer, 0x00, sizeof(max30003_spi_write_buffer));
	memset(max30003_spi_read_buffer, 0x00, sizeof(max30003_spi_read_buffer));

	max30003_spi_write_buffer[0] = (address << 1) | 1;

	spi_xfer_done = 0x00;

	nrf_drv_spi_transfer(&m_spi, max30003_spi_write_buffer, 4,
			max30003_spi_read_buffer, 4);

	while (!spi_xfer_done && --timeout) {

	}

	*data = 0x00;
	*data |= max30003_spi_read_buffer[1] << 16;
	*data |= max30003_spi_read_buffer[2] << 8;
	*data |= max30003_spi_read_buffer[3];

}

void max30003_spi_read_ecg_sample(uint8_t address, int32_t *data) {

	//uint16_t timeout = UINT16_MAX;

	memset(max30003_spi_write_buffer, 0x00, sizeof(max30003_spi_write_buffer));
	memset(max30003_spi_read_buffer, 0x00, sizeof(max30003_spi_read_buffer));

	max30003_spi_write_buffer[0] = (address << 1) | 1;

	spi_xfer_done = 0x00;

	nrf_drv_spi_transfer(&m_spi, max30003_spi_write_buffer, 4,
			max30003_spi_read_buffer, 4);

	while (!spi_xfer_done) {

	}

	uint32_t data0 = (uint32_t) (max30003_spi_read_buffer[1]);
	data0 = data0 << 24;
	uint32_t data1 = (uint32_t) (max30003_spi_read_buffer[2]);
	data1 = data1 << 16;
	uint32_t data2 = (uint32_t) (max30003_spi_read_buffer[3]);
	data2 = data2 >> 6;
	data2 = data2 & 0x03;

	uint32_t ecg = (uint32_t) (data0 | data1 | data2);
	*data = (int32_t) ecg;

}

uint8_t get_max30003_rev_id() {

	max30003_info_reg max30003_info;

	max30003_spi_read(MAX30003_INFO, &max30003_info.value);

	uint8_t id = max30003_info.bits.REV_ID;

	return id;

}

void init_max30003_mode_0() {

	max30003_sw_rst_reg max30003_sw_rst;
	max30003_sw_rst.value = 0x00;
	max30003_spi_write(MAX30003_SW_RST, max30003_sw_rst.value);

	wait_100_ms_timer(5);

	max30003_cnfg_gen_reg max30003_cnfg_gen;
//    max30003_cnfg_gen.value = 0x080004;
	max30003_cnfg_gen.value = 0x00;
	max30003_cnfg_gen.bits.EN_ECG = 1; //ECG Channel enabled
	max30003_cnfg_gen.bits.RBIASN = 0; //ECGN is connected to VMID through a resistor (selected by RBIASV)
	max30003_cnfg_gen.bits.RBIASP = 0; //ECGP is connected to VMID through a resistor (selected by RBIASV)
	max30003_cnfg_gen.bits.RBIASV = 1;
	max30003_cnfg_gen.bits.EN_RBIAS = 1;
	max30003_cnfg_gen.bits.IMAG = 2;
	max30003_cnfg_gen.bits.EN_DCLOFF = 1; //01 = DCLOFF Detection applied to the ECGP/N pins
	max30003_spi_write(MAX30003_CNFG_GEN, max30003_cnfg_gen.value);

	max30003_cnfg_cal_reg max30003_cnfg_cal;
//    max30003_cnfg_cal.value = 0x720000;
	max30003_cnfg_cal.value = 0x00;
	max30003_cnfg_cal.bits.EN_VCAL = 1;
	max30003_cnfg_cal.bits.VMODE = 1;
	max30003_cnfg_cal.bits.VMAG = 1;
	max30003_spi_write(MAX30003_CNFG_CAL, max30003_cnfg_cal.value);

	max30003_cnfg_emux_reg max30003_cnfg_emux;
//    max30003_cnfg_emux.value = 0x0B0000;
	max30003_cnfg_emux.value = 0x00;
	max30003_cnfg_emux.bits.CALP_SEL = 2;
	max30003_cnfg_emux.bits.CALN_SEL = 3;
	max30003_spi_write(MAX30003_CNFG_EMUX, max30003_cnfg_emux.value);

	max30003_cnfg_ecg_reg max30003_cnfg_ecg;
	//max30003_cnfg_ecg.value = 0x805000;
	max30003_cnfg_ecg.value = 0x00;
	max30003_cnfg_ecg.bits.RATE = 2;
	max30003_cnfg_ecg.bits.GAIN = 3;
	max30003_cnfg_ecg.bits.DHPF = 1;
	max30003_cnfg_ecg.bits.DLPF = 1;
	max30003_spi_write(MAX30003_CNFG_ECG, max30003_cnfg_ecg.value);

	max30003_cnfg_rtor_reg max30003_cnfg_rtor;
	//max30003_cnfg_rtor.value = 0x3FC600;
	max30003_cnfg_rtor.value = 0x00;
	max30003_cnfg_rtor.bits.WNDW = 3;
	max30003_cnfg_rtor.bits.GAIN = 1;
	max30003_cnfg_rtor.bits.EN_RTOR = 1;
	max30003_cnfg_rtor.bits.PAVG = 2;
	max30003_cnfg_rtor.bits.PTSF = 3;
	max30003_spi_write(MAX30003_CNFG_RTOR1, max30003_cnfg_rtor.value);

	max30003_synch_reg max30003_synch;
	max30003_synch.value = 0x00;
	max30003_spi_write(MAX30003_SYNCH, max30003_synch.value);

}

void init_max30003_mode_1() {

	max30003_sw_rst_reg max30003_sw_rst;
	max30003_sw_rst.value = 0x00;
	max30003_spi_write(MAX30003_SW_RST, max30003_sw_rst.value);

	wait_100_ms_timer(5);

	max30003_cnfg_gen_reg max30003_cnfg_gen;
	max30003_cnfg_gen.value = 0x00;
	max30003_cnfg_gen.bits.EN_ECG = 1; //ECG Channel enabled
	max30003_cnfg_gen.bits.RBIASN = 1; //ECGN is connected to VMID through a resistor (selected by RBIASV)
	max30003_cnfg_gen.bits.RBIASP = 1; //ECGP is connected to VMID through a resistor (selected by RBIASV)
	max30003_cnfg_gen.bits.RBIASV = 1;
	max30003_cnfg_gen.bits.EN_RBIAS = 1;
	max30003_cnfg_gen.bits.IMAG = 2;
	max30003_cnfg_gen.bits.EN_DCLOFF = 1; //01 = DCLOFF Detection applied to the ECGP/N pins
	max30003_spi_write(MAX30003_CNFG_GEN, max30003_cnfg_gen.value);

	max30003_cnfg_ecg_reg max30003_cnfg_ecg;
	max30003_cnfg_ecg.value = 0x00;
	max30003_cnfg_ecg.bits.RATE = 2;
	max30003_cnfg_ecg.bits.GAIN = 3;
	max30003_cnfg_ecg.bits.DHPF = 1;
	max30003_cnfg_ecg.bits.DLPF = 1;
	max30003_spi_write(MAX30003_CNFG_ECG, max30003_cnfg_ecg.value);

	max30003_cnfg_rtor_reg max30003_cnfg_rtor;
	max30003_cnfg_rtor.value = 0x00;
	max30003_cnfg_rtor.bits.EN_RTOR = 1;
	max30003_spi_write(MAX30003_CNFG_RTOR1, max30003_cnfg_rtor.value);

	max30003_cnfg_emux_reg max30003_cnfg_emux;
	max30003_cnfg_emux.value = 0x00;
	max30003_cnfg_emux.bits.OPENN = 0;
	max30003_cnfg_emux.bits.OPENP = 0;
	max30003_spi_write(MAX30003_CNFG_EMUX, max30003_cnfg_emux.value);

	max30003_mngr_dyn_reg max30003_mngr_dyn;
	max30003_mngr_dyn.value = 0x00;
	max30003_mngr_dyn.bits.FAST = 0;
	max30003_spi_write(MAX30003_MNGR_DYN, max30003_mngr_dyn.value);

	max30003_synch_reg max30003_synch;
	max30003_synch.value = 0x00;
	max30003_spi_write(MAX30003_SYNCH, max30003_synch.value);

}

void set_max30003_sync() {

	max30003_synch_reg max30003_synch;
	max30003_synch.value = 0x00;
	max30003_spi_write(MAX30003_SYNCH, max30003_synch.value);

}

void set_max30003_fifo_reset() {

	max30003_fifo_rst_reg max30003_fifo_rst;
	max30003_fifo_rst.value = 0x00;
	max30003_spi_write(MAX30003_FIFO_RST, max30003_fifo_rst.value);

}

int32_t get_max30003_ecg_voltage_sample() {

	int32_t value;
	max30003_spi_read_ecg_sample(MAX30003_ECG, &value);

	return value;

}

uint32_t get_max30003_rtor_interval() {

	max30003_rtor_reg max30003_rtor;
	max30003_rtor.value = 0x00;
	max30003_spi_read(MAX30003_RTOR, &max30003_rtor.value);

	return max30003_rtor.bits.RTOR_INTERVAL;

}
