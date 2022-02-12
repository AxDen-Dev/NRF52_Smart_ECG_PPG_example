#ifndef BOARD_DEFINE_H_
#define BOARD_DEFINE_H_

#define NRF528XX
//#define NRF528XX_FEM

#ifdef NRF528XX

#define SPI_MOSI_GPIO 11
#define SPI_MISO_GPIO 10
#define SPI_SCK_GPIO 9
#define SPI_CS_GPIO 8

#define UART_2_TX_GPIO 7
#define UART_2_RX_GPIO 6

#define ADC_0_GPIO 5 // AIN3
#define ADC_1_GPIO 4 // AIN2

#define UART_1_RX_GPIO 3
#define UART_1_TX_GPIO 2

#define GPIO_0_GPIO 31
#define GPIO_1_GPIO 30
#define GPIO_2_GPIO 29
#define GPIO_3_GPIO 28

#define I2C_SDA_GPIO 27
#define I2C_SCL_GPIO 26

#define GPIO_WKUP_GPIO 25

#endif

#ifdef NRF528XX_FEM

#define FEM_CPS 27
#define FEM_CTX 26
#define FEM_CHL 25
#define FEM_ANT_SEL 18
#define FEM_CSD 19
#define FEM_CRX 20

#define SPI_MOSI_GPIO 16
#define SPI_MISO_GPIO 15
#define SPI_SCK_GPIO 14
#define SPI_CS_GPIO 13

#define UART_2_TX_GPIO 12
#define UART_2_RX_GPIO 11

#define ADC_0_GPIO 4 // AIN2
#define ADC_1_GPIO 3 // AIN1

#define UART_1_RX_GPIO 2
#define UART_1_TX_GPIO 8

#define GPIO_0_GPIO 7
#define GPIO_1_GPIO 6
#define GPIO_2_GPIO 5
#define GPIO_3_GPIO 31

#define I2C_SDA_GPIO 30
#define I2C_SCL_GPIO 29

#define GPIO_WKUP_GPIO 28

#endif

//Application Board PinMap
#define GPIO_LED_0 UART_2_TX_GPIO
#define GPIO_LED_1 UART_2_RX_GPIO

#define GPIO_BTN GPIO_1_GPIO

#define GPIO_BAT_LEVEL ADC_0_GPIO
#define GPIO_BAT_EN ADC_1_GPIO

#define GPIO_SENSOR_POWER_EN GPIO_0_GPIO

#endif /* BOARD_DEFINE_H_ */
