/*
 * board.h
 *
 *  Created on: Apr 11, 2019
 *      Author: george
 */

#ifndef STARTUP_BOARD_H_
#define STARTUP_BOARD_H_

#include <ti/drivers/ADC.h>
#include <ti/drivers/ADCBuf.h>
#include <ti/drivers/PWM.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/Watchdog.h>

/* Includes */
#include <ti/drivers/PIN.h>
#include <ti/devices/cc26x0r2/driverlib/ioc.h>

/* Externs */
extern const PIN_Config BoardGpioInitTable[];

/*
 *  ============================================================================
 *  RF Front End and Bias configuration symbols for TI reference designs and
 *  kits. This symbol sets the RF Front End configuration in ble_user_config.h
 *  and selects the appropriate PA table in ble_user_config.c.
 *  Other configurations can be used by editing these files.
 *
 *  Define only one symbol:
 *  CC2650EM_7ID    - Differential RF and internal biasing
                      (default for CC2640R2 LaunchPad)
 *  CC2650EM_5XD    – Differential RF and external biasing
 *  CC2650EM_4XS    – Single-ended RF on RF-P and external biasing
 *  CC2640R2DK_CXS  - WCSP: Single-ended RF on RF-N and external biasing
 *                    (Note that the WCSP is only tested and characterized for
 *                     single ended configuration, and it has a WCSP-specific
 *                     PA table)
 *
 *  Note: CC2650EM_xxx reference designs apply to all CC26xx devices.
 *  ==========================================================================
 */
#define CC2650EM_7ID

/* Mapping of pins to board signals using general board aliases
 *      <board signal alias>                  <pin mapping>
 */

/* Analog Capable DIOs */
#define VBAT_IO_ANALOG  IOID_23
#define LIGHT_IO_ANALOG IOID_30

/* Digital IOs */

/* Discrete Inputs */
#define QC16_PIN_BTN1              IOID_13
#define QC16_PIN_BTN2              IOID_14

/* GPIO */
#define QC16_GPIO_LED_ON           1
#define QC16_GPIO_LED_OFF          0

/* I2C */
// TODO: for prod, this will be:
#define QC16_I2C0_SCL0             IOID_4 // IOID_28
#define QC16_I2C0_SDA0             IOID_5 // IOID_29

/* LEDs */
#define QC16_PIN_LED_ON            1
#define QC16_PIN_LED_OFF           0
#define QC16_PIN_RLED              IOID_6
#define QC16_PIN_GLED              IOID_7

/* PWM Outputs */
#define QC16_PWMPIN0               QC16_PIN_RLED
#define QC16_PWMPIN1               QC16_PIN_GLED
#define QC16_PWMPIN2               PIN_UNASSIGNED
#define QC16_PWMPIN3               PIN_UNASSIGNED
#define QC16_PWMPIN4               PIN_UNASSIGNED
#define QC16_PWMPIN5               PIN_UNASSIGNED
#define QC16_PWMPIN6               PIN_UNASSIGNED
#define QC16_PWMPIN7               PIN_UNASSIGNED

/* SPI */
#define QC16_SPI_FLASH_CS          IOID_20
#define QC16_FLASH_CS_ON           0
#define QC16_FLASH_CS_OFF          1

// SPI for the epaper display:
/// EPD SPI MOSI
#define EPAPER_SDIO     IOID_25
/// EPD SPI SCLK
#define EPAPER_SCLK     IOID_26
#define EPAPER_CSN      IOID_0
#define EPAPER_DCN      IOID_1
#define EPAPER_RESN     IOID_12
#define EPAPER_BUSY     IOID_15

/* SPI Board */
#define QC16_SPI0_MISO             IOID_8          /* RF1.20 */
#define QC16_SPI0_MOSI             IOID_9          /* RF1.18 */
#define QC16_SPI0_CLK              IOID_10         /* RF1.16 */
#define QC16_SPI0_CSN              PIN_UNASSIGNED
#define QC16_SPI1_MISO             PIN_UNASSIGNED
#define QC16_SPI1_MOSI             EPAPER_SDIO
#define QC16_SPI1_CLK              EPAPER_SCLK
#define QC16_SPI1_CSN              PIN_UNASSIGNED


/* UART Board */
#define QC16_UART_RX               IOID_2          /* RXD */
#define QC16_UART_TX               IOID_3          /* TXD */
#define QC16_UART_CTS              IOID_19         /* CTS */
#define QC16_UART_RTS              IOID_18         /* RTS */

/*!
 *  @brief  Initialize the general board specific settings
 *
 *  This function initializes the general board specific settings.
 */
void QC16_initGeneral(void);

/*!
 *  @def    QC16_ADCBufName
 *  @brief  Enum of ADCs
 */
typedef enum QC16_ADCBufName {
    QC16_ADCBUF0 = 0,

    QC16_ADCBUFCOUNT
} QC16_ADCBufName;

/*!
 *  @def    QC16_ADCBuf0SourceName
 *  @brief  Enum of ADCBuf channels
 */
typedef enum QC16_ADCBuf0ChannelName {
    QC16_ADCBUF0CHANNEL0 = 0,
//    QC16_ADCBUF0CHANNEL1,
//    QC16_ADCBUF0CHANNEL2,
//    QC16_ADCBUF0CHANNEL3,
//    QC16_ADCBUF0CHANNEL4,
//    QC16_ADCBUF0CHANNEL5,
//    QC16_ADCBUF0CHANNEL6,
    QC16_ADCBUF0CHANNEL7,
    QC16_ADCBUF0CHANNELVDDS,
    QC16_ADCBUF0CHANNELDCOUPL,
    QC16_ADCBUF0CHANNELVSS,

    QC16_ADCBUF0CHANNELCOUNT
} QC16_ADCBuf0ChannelName;

/*!
 *  @def    QC16_ADCName
 *  @brief  Enum of ADCs
 */
typedef enum QC16_ADCName {
    QC16_ADC_VBAT = 0,
    QC16_ADC_LIGHT,
    QC16_ADCDCOUPL,
    QC16_ADCVSS,
    QC16_ADCVDDS,

    QC16_ADCCOUNT
} QC16_ADCName;

// TODO: The following indexes the pin init array thing
/*!
 *  @def    QC16_GPIOName
 *  @brief  Enum of GPIO names
 */
typedef enum QC16_GPIOName {
    QC16_GPIO_S1 = 0,
    QC16_GPIO_S2,
    QC16_SPI_MASTER_READY,
    QC16_SPI_SLAVE_READY,
    QC16_GPIO_LED_GREEN,
    QC16_GPIO_LED_RED,
    QC16_GPIO_TMP116_EN,
    QC16_GPIO_SPI_FLASH_CS,
    QC16_SDSPI_CS,
    QC16_GPIO_LCD_CS,
    QC16_GPIO_LCD_POWER,
    QC16_GPIO_LCD_ENABLE,
    QC16_GPIOCOUNT
} QC16_GPIOName;

/*!
 *  @def    QC16_GPTimerName
 *  @brief  Enum of GPTimer parts
 */
typedef enum QC16_GPTimerName {
    QC16_GPTIMER0A = 0,
    QC16_GPTIMER0B,
    QC16_GPTIMER1A,
    QC16_GPTIMER1B,
    QC16_GPTIMER2A,
    QC16_GPTIMER2B,
    QC16_GPTIMER3A,
    QC16_GPTIMER3B,

    QC16_GPTIMERPARTSCOUNT
} QC16_GPTimerName;

/*!
 *  @def    QC16_GPTimers
 *  @brief  Enum of GPTimers
 */
typedef enum QC16_GPTimers {
    QC16_GPTIMER0 = 0,
    QC16_GPTIMER1,
    QC16_GPTIMER2,
    QC16_GPTIMER3,

    QC16_GPTIMERCOUNT
} QC16_GPTimers;

/*!
 *  @def    QC16_I2CName
 *  @brief  Enum of I2C names
 */
typedef enum QC16_I2CName {
    QC16_I2C0 = 0,

    QC16_I2CCOUNT
} QC16_I2CName;

/*!
 *  @def    QC16_NVSName
 *  @brief  Enum of NVS names
 */
typedef enum QC16_NVSName {
#ifndef Board_EXCLUDE_NVS_INTERNAL_FLASH
    QC16_NVSCC26XX0 = 0,
#endif
#ifndef Board_EXCLUDE_NVS_EXTERNAL_FLASH
    QC16_NVSSPI25X0,
#endif

    QC16_NVSCOUNT
} QC16_NVSName;

/*!
 *  @def    QC16_PWM
 *  @brief  Enum of PWM outputs
 */
typedef enum QC16_PWMName {
    QC16_PWM0 = 0,
    QC16_PWM1,
    QC16_PWM2,
    QC16_PWM3,
    QC16_PWM4,
    QC16_PWM5,
    QC16_PWM6,
    QC16_PWM7,

    QC16_PWMCOUNT
} QC16_PWMName;

/*!
 *  @def    QC16_SDName
 *  @brief  Enum of SD names
 */
typedef enum QC16_SDName {
    QC16_SDSPI0 = 0,

    QC16_SDCOUNT
} QC16_SDName;

/*!
 *  @def    QC16_SPIName
 *  @brief  Enum of SPI names
 */
typedef enum QC16_SPIName {
    QC16_SPI0 = 0,
    QC16_SPI1,

    QC16_SPICOUNT
} QC16_SPIName;

/*!
 *  @def    QC16_UARTName
 *  @brief  Enum of UARTs
 */
typedef enum QC16_UARTName {
    QC16_UART0 = 0,

    QC16_UARTCOUNT
} QC16_UARTName;

/*!
 *  @def    QC16_UDMAName
 *  @brief  Enum of DMA buffers
 */
typedef enum QC16_UDMAName {
    QC16_UDMA0 = 0,

    QC16_UDMACOUNT
} QC16_UDMAName;

/*!
 *  @def    QC16_WatchdogName
 *  @brief  Enum of Watchdogs
 */
typedef enum QC16_WatchdogName {
    QC16_WATCHDOG0 = 0,

    QC16_WATCHDOGCOUNT
} QC16_WatchdogName;

/*!
 *  @def    CC2650_LAUNCHXL_TRNGName
 *  @brief  Enum of TRNG names on the board
 */
typedef enum QC16_TRNGName {
    QC16_TRNG0 = 0,
    QC16_TRNGCOUNT
} QC16_TRNGName;

#define Board_init()            QC16_initGeneral()
#define Board_initGeneral()     QC16_initGeneral()

#endif /* STARTUP_BOARD_H_ */
