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

/* Defines */
#ifndef CC2640R2_LAUNCHXL
  #define CC2640R2_LAUNCHXL
#endif /* CC2640R2_LAUNCHXL */

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
#define CC2640R2_LAUNCHXL_DIO23_ANALOG          IOID_23
#define CC2640R2_LAUNCHXL_DIO24_ANALOG          IOID_24
#define CC2640R2_LAUNCHXL_DIO25_ANALOG          IOID_25
#define CC2640R2_LAUNCHXL_DIO26_ANALOG          IOID_26
#define CC2640R2_LAUNCHXL_DIO27_ANALOG          IOID_27
#define CC2640R2_LAUNCHXL_DIO28_ANALOG          IOID_28
#define CC2640R2_LAUNCHXL_DIO29_ANALOG          IOID_29
#define CC2640R2_LAUNCHXL_DIO30_ANALOG          IOID_30

/* Digital IOs */
#define CC2640R2_LAUNCHXL_DIO0                  IOID_0
#define CC2640R2_LAUNCHXL_DIO1_RFSW             IOID_1
#define CC2640R2_LAUNCHXL_DIO12                 IOID_12
#define CC2640R2_LAUNCHXL_DIO15                 IOID_15
#define CC2640R2_LAUNCHXL_DIO16_TDO             IOID_16
#define CC2640R2_LAUNCHXL_DIO17_TDI             IOID_17
#define CC2640R2_LAUNCHXL_DIO21                 IOID_21
#define CC2640R2_LAUNCHXL_DIO22                 IOID_22

/* Discrete Inputs */
#define CC2640R2_LAUNCHXL_PIN_BTN1              IOID_13
#define CC2640R2_LAUNCHXL_PIN_BTN2              IOID_14

/* GPIO */
#define CC2640R2_LAUNCHXL_GPIO_LED_ON           1
#define CC2640R2_LAUNCHXL_GPIO_LED_OFF          0

/* I2C */
#define CC2640R2_LAUNCHXL_I2C0_SCL0             IOID_4
#define CC2640R2_LAUNCHXL_I2C0_SDA0             IOID_5

/* LEDs */
#define CC2640R2_LAUNCHXL_PIN_LED_ON            1
#define CC2640R2_LAUNCHXL_PIN_LED_OFF           0
#define CC2640R2_LAUNCHXL_PIN_RLED              IOID_6
#define CC2640R2_LAUNCHXL_PIN_GLED              IOID_7

/* PWM Outputs */
#define CC2640R2_LAUNCHXL_PWMPIN0               CC2640R2_LAUNCHXL_PIN_RLED
#define CC2640R2_LAUNCHXL_PWMPIN1               CC2640R2_LAUNCHXL_PIN_GLED
#define CC2640R2_LAUNCHXL_PWMPIN2               PIN_UNASSIGNED
#define CC2640R2_LAUNCHXL_PWMPIN3               PIN_UNASSIGNED
#define CC2640R2_LAUNCHXL_PWMPIN4               PIN_UNASSIGNED
#define CC2640R2_LAUNCHXL_PWMPIN5               PIN_UNASSIGNED
#define CC2640R2_LAUNCHXL_PWMPIN6               PIN_UNASSIGNED
#define CC2640R2_LAUNCHXL_PWMPIN7               PIN_UNASSIGNED

/* SPI */
#define CC2640R2_LAUNCHXL_SPI_FLASH_CS          IOID_20
#define CC2640R2_LAUNCHXL_FLASH_CS_ON           0
#define CC2640R2_LAUNCHXL_FLASH_CS_OFF          1

/* SPI Board */
#define CC2640R2_LAUNCHXL_SPI0_MISO             IOID_8          /* RF1.20 */
#define CC2640R2_LAUNCHXL_SPI0_MOSI             IOID_9          /* RF1.18 */
#define CC2640R2_LAUNCHXL_SPI0_CLK              IOID_10         /* RF1.16 */
#define CC2640R2_LAUNCHXL_SPI0_CSN              PIN_UNASSIGNED
#define CC2640R2_LAUNCHXL_SPI1_MISO             PIN_UNASSIGNED
#define CC2640R2_LAUNCHXL_SPI1_MOSI             PIN_UNASSIGNED
#define CC2640R2_LAUNCHXL_SPI1_CLK              PIN_UNASSIGNED
#define CC2640R2_LAUNCHXL_SPI1_CSN              PIN_UNASSIGNED

/* UART Board */
#define CC2640R2_LAUNCHXL_UART_RX               IOID_2          /* RXD */
#define CC2640R2_LAUNCHXL_UART_TX               IOID_3          /* TXD */
#define CC2640R2_LAUNCHXL_UART_CTS              IOID_19         /* CTS */
#define CC2640R2_LAUNCHXL_UART_RTS              IOID_18         /* RTS */

/*!
 *  @brief  Initialize the general board specific settings
 *
 *  This function initializes the general board specific settings.
 */
void CC2640R2_LAUNCHXL_initGeneral(void);

/*!
 *  @brief  Turn off the external flash on LaunchPads
 *
 */
void CC2640R2_LAUNCHXL_shutDownExtFlash(void);

/*!
 *  @brief  Wake up the external flash present on the board files
 *
 *  This function toggles the chip select for the amount of time needed
 *  to wake the chip up.
 */
void CC2640R2_LAUNCHXL_wakeUpExtFlash(void);

/*!
 *  @def    CC2640R2_LAUNCHXL_ADCBufName
 *  @brief  Enum of ADCs
 */
typedef enum CC2640R2_LAUNCHXL_ADCBufName {
    CC2640R2_LAUNCHXL_ADCBUF0 = 0,

    CC2640R2_LAUNCHXL_ADCBUFCOUNT
} CC2640R2_LAUNCHXL_ADCBufName;

/*!
 *  @def    CC2640R2_LAUNCHXL_ADCBuf0SourceName
 *  @brief  Enum of ADCBuf channels
 */
typedef enum CC2640R2_LAUNCHXL_ADCBuf0ChannelName {
    CC2640R2_LAUNCHXL_ADCBUF0CHANNEL0 = 0,
    CC2640R2_LAUNCHXL_ADCBUF0CHANNEL1,
    CC2640R2_LAUNCHXL_ADCBUF0CHANNEL2,
    CC2640R2_LAUNCHXL_ADCBUF0CHANNEL3,
    CC2640R2_LAUNCHXL_ADCBUF0CHANNEL4,
    CC2640R2_LAUNCHXL_ADCBUF0CHANNEL5,
    CC2640R2_LAUNCHXL_ADCBUF0CHANNEL6,
    CC2640R2_LAUNCHXL_ADCBUF0CHANNEL7,
    CC2640R2_LAUNCHXL_ADCBUF0CHANNELVDDS,
    CC2640R2_LAUNCHXL_ADCBUF0CHANNELDCOUPL,
    CC2640R2_LAUNCHXL_ADCBUF0CHANNELVSS,

    CC2640R2_LAUNCHXL_ADCBUF0CHANNELCOUNT
} CC2640R2_LAUNCHXL_ADCBuf0ChannelName;

/*!
 *  @def    CC2640R2_LAUNCHXL_ADCName
 *  @brief  Enum of ADCs
 */
typedef enum CC2640R2_LAUNCHXL_ADCName {
    CC2640R2_LAUNCHXL_ADC0 = 0,
    CC2640R2_LAUNCHXL_ADC1,
    CC2640R2_LAUNCHXL_ADC2,
    CC2640R2_LAUNCHXL_ADC3,
    CC2640R2_LAUNCHXL_ADC4,
    CC2640R2_LAUNCHXL_ADC5,
    CC2640R2_LAUNCHXL_ADC6,
    CC2640R2_LAUNCHXL_ADC7,
    CC2640R2_LAUNCHXL_ADCDCOUPL,
    CC2640R2_LAUNCHXL_ADCVSS,
    CC2640R2_LAUNCHXL_ADCVDDS,

    CC2640R2_LAUNCHXL_ADCCOUNT
} CC2640R2_LAUNCHXL_ADCName;

/*!
 *  @def    CC2640R2_LAUNCHXL_CryptoName
 *  @brief  Enum of Crypto names
 */
typedef enum CC2640R2_LAUNCHXL_CryptoName {
    CC2640R2_LAUNCHXL_CRYPTO0 = 0,

    CC2640R2_LAUNCHXL_CRYPTOCOUNT
} CC2640R2_LAUNCHXL_CryptoName;

/*!
 *  @def    CC2640R2_LAUNCHXL_GPIOName
 *  @brief  Enum of GPIO names
 */
typedef enum CC2640R2_LAUNCHXL_GPIOName {
    CC2640R2_LAUNCHXL_GPIO_S1 = 0,
    CC2640R2_LAUNCHXL_GPIO_S2,
    CC2640R2_LAUNCHXL_SPI_MASTER_READY,
    CC2640R2_LAUNCHXL_SPI_SLAVE_READY,
    CC2640R2_LAUNCHXL_GPIO_LED_GREEN,
    CC2640R2_LAUNCHXL_GPIO_LED_RED,
    CC2640R2_LAUNCHXL_GPIO_TMP116_EN,
    CC2640R2_LAUNCHXL_GPIO_SPI_FLASH_CS,
    CC2640R2_LAUNCHXL_SDSPI_CS,
    CC2640R2_LAUNCHXL_GPIO_LCD_CS,
    CC2640R2_LAUNCHXL_GPIO_LCD_POWER,
    CC2640R2_LAUNCHXL_GPIO_LCD_ENABLE,
    CC2640R2_LAUNCHXL_GPIOCOUNT
} CC2640R2_LAUNCHXL_GPIOName;

/*!
 *  @def    CC2640R2_LAUNCHXL_GPTimerName
 *  @brief  Enum of GPTimer parts
 */
typedef enum CC2640R2_LAUNCHXL_GPTimerName {
    CC2640R2_LAUNCHXL_GPTIMER0A = 0,
    CC2640R2_LAUNCHXL_GPTIMER0B,
    CC2640R2_LAUNCHXL_GPTIMER1A,
    CC2640R2_LAUNCHXL_GPTIMER1B,
    CC2640R2_LAUNCHXL_GPTIMER2A,
    CC2640R2_LAUNCHXL_GPTIMER2B,
    CC2640R2_LAUNCHXL_GPTIMER3A,
    CC2640R2_LAUNCHXL_GPTIMER3B,

    CC2640R2_LAUNCHXL_GPTIMERPARTSCOUNT
} CC2640R2_LAUNCHXL_GPTimerName;

/*!
 *  @def    CC2640R2_LAUNCHXL_GPTimers
 *  @brief  Enum of GPTimers
 */
typedef enum CC2640R2_LAUNCHXL_GPTimers {
    CC2640R2_LAUNCHXL_GPTIMER0 = 0,
    CC2640R2_LAUNCHXL_GPTIMER1,
    CC2640R2_LAUNCHXL_GPTIMER2,
    CC2640R2_LAUNCHXL_GPTIMER3,

    CC2640R2_LAUNCHXL_GPTIMERCOUNT
} CC2640R2_LAUNCHXL_GPTimers;

/*!
 *  @def    CC2640R2_LAUNCHXL_I2CName
 *  @brief  Enum of I2C names
 */
typedef enum CC2640R2_LAUNCHXL_I2CName {
    CC2640R2_LAUNCHXL_I2C0 = 0,

    CC2640R2_LAUNCHXL_I2CCOUNT
} CC2640R2_LAUNCHXL_I2CName;

/*!
 *  @def    CC2640R2_LAUNCHXL_NVSName
 *  @brief  Enum of NVS names
 */
typedef enum CC2640R2_LAUNCHXL_NVSName {
#ifndef Board_EXCLUDE_NVS_INTERNAL_FLASH
    CC2640R2_LAUNCHXL_NVSCC26XX0 = 0,
#endif
#ifndef Board_EXCLUDE_NVS_EXTERNAL_FLASH
    CC2640R2_LAUNCHXL_NVSSPI25X0,
#endif

    CC2640R2_LAUNCHXL_NVSCOUNT
} CC2640R2_LAUNCHXL_NVSName;

/*!
 *  @def    CC2640R2_LAUNCHXL_PWM
 *  @brief  Enum of PWM outputs
 */
typedef enum CC2640R2_LAUNCHXL_PWMName {
    CC2640R2_LAUNCHXL_PWM0 = 0,
    CC2640R2_LAUNCHXL_PWM1,
    CC2640R2_LAUNCHXL_PWM2,
    CC2640R2_LAUNCHXL_PWM3,
    CC2640R2_LAUNCHXL_PWM4,
    CC2640R2_LAUNCHXL_PWM5,
    CC2640R2_LAUNCHXL_PWM6,
    CC2640R2_LAUNCHXL_PWM7,

    CC2640R2_LAUNCHXL_PWMCOUNT
} CC2640R2_LAUNCHXL_PWMName;

/*!
 *  @def    CC2640R2_LAUNCHXL_SDName
 *  @brief  Enum of SD names
 */
typedef enum CC2640R2_LAUNCHXL_SDName {
    CC2640R2_LAUNCHXL_SDSPI0 = 0,

    CC2640R2_LAUNCHXL_SDCOUNT
} CC2640R2_LAUNCHXL_SDName;

/*!
 *  @def    CC2640R2_LAUNCHXL_SPIName
 *  @brief  Enum of SPI names
 */
typedef enum CC2640R2_LAUNCHXL_SPIName {
    CC2640R2_LAUNCHXL_SPI0 = 0,
    CC2640R2_LAUNCHXL_SPI1,

    CC2640R2_LAUNCHXL_SPICOUNT
} CC2640R2_LAUNCHXL_SPIName;

/*!
 *  @def    CC2640R2_LAUNCHXL_UARTName
 *  @brief  Enum of UARTs
 */
typedef enum CC2640R2_LAUNCHXL_UARTName {
    CC2640R2_LAUNCHXL_UART0 = 0,

    CC2640R2_LAUNCHXL_UARTCOUNT
} CC2640R2_LAUNCHXL_UARTName;

/*!
 *  @def    CC2640R2_LAUNCHXL_UDMAName
 *  @brief  Enum of DMA buffers
 */
typedef enum CC2640R2_LAUNCHXL_UDMAName {
    CC2640R2_LAUNCHXL_UDMA0 = 0,

    CC2640R2_LAUNCHXL_UDMACOUNT
} CC2640R2_LAUNCHXL_UDMAName;

/*!
 *  @def    CC2640R2_LAUNCHXL_WatchdogName
 *  @brief  Enum of Watchdogs
 */
typedef enum CC2640R2_LAUNCHXL_WatchdogName {
    CC2640R2_LAUNCHXL_WATCHDOG0 = 0,

    CC2640R2_LAUNCHXL_WATCHDOGCOUNT
} CC2640R2_LAUNCHXL_WatchdogName;

/*!
 *  @def    CC2650_LAUNCHXL_TRNGName
 *  @brief  Enum of TRNG names on the board
 */
typedef enum CC2640R2_LAUNCHXL_TRNGName {
    CC2640R2_LAUNCHXL_TRNG0 = 0,
    CC2640R2_LAUNCHXL_TRNGCOUNT
} CC2640R2_LAUNCHXL_TRNGName;

#define Board_init()            CC2640R2_LAUNCHXL_initGeneral()
#define Board_initGeneral()     CC2640R2_LAUNCHXL_initGeneral()
#define Board_shutDownExtFlash() CC2640R2_LAUNCHXL_shutDownExtFlash()
#define Board_wakeUpExtFlash() CC2640R2_LAUNCHXL_wakeUpExtFlash()

/* These #defines allow us to reuse TI-RTOS across other device families */

#define Board_ADC0              CC2640R2_LAUNCHXL_ADC0
#define Board_ADC1              CC2640R2_LAUNCHXL_ADC1

#define Board_ADCBUF0           CC2640R2_LAUNCHXL_ADCBUF0
#define Board_ADCBUF0CHANNEL0   CC2640R2_LAUNCHXL_ADCBUF0CHANNEL0
#define Board_ADCBUF0CHANNEL1   CC2640R2_LAUNCHXL_ADCBUF0CHANNEL1

#define Board_CRYPTO0           CC2640R2_LAUNCHXL_CRYPTO0

#define Board_DIO0              CC2640R2_LAUNCHXL_DIO0
#define Board_DIO1_RFSW         CC2640R2_LAUNCHXL_DIO1_RFSW
#define Board_DIO12             CC2640R2_LAUNCHXL_DIO12
#define Board_DIO15             CC2640R2_LAUNCHXL_DIO15
#define Board_DIO16_TDO         CC2640R2_LAUNCHXL_DIO16_TDO
#define Board_DIO17_TDI         CC2640R2_LAUNCHXL_DIO17_TDI
#define Board_DIO21             CC2640R2_LAUNCHXL_DIO21
#define Board_DIO22             CC2640R2_LAUNCHXL_DIO22

#define Board_DIO23_ANALOG      CC2640R2_LAUNCHXL_DIO23_ANALOG
#define Board_DIO24_ANALOG      CC2640R2_LAUNCHXL_DIO24_ANALOG
#define Board_DIO25_ANALOG      CC2640R2_LAUNCHXL_DIO25_ANALOG
#define Board_DIO26_ANALOG      CC2640R2_LAUNCHXL_DIO26_ANALOG
#define Board_DIO27_ANALOG      CC2640R2_LAUNCHXL_DIO27_ANALOG
#define Board_DIO28_ANALOG      CC2640R2_LAUNCHXL_DIO28_ANALOG
#define Board_DIO29_ANALOG      CC2640R2_LAUNCHXL_DIO29_ANALOG
#define Board_DIO30_ANALOG      CC2640R2_LAUNCHXL_DIO30_ANALOG

#define Board_GPIO_BUTTON0      CC2640R2_LAUNCHXL_GPIO_S1
#define Board_GPIO_BUTTON1      CC2640R2_LAUNCHXL_GPIO_S2
#define Board_GPIO_BTN1         CC2640R2_LAUNCHXL_GPIO_S1
#define Board_GPIO_BTN2         CC2640R2_LAUNCHXL_GPIO_S2
#define Board_GPIO_LED0         CC2640R2_LAUNCHXL_GPIO_LED_RED
#define Board_GPIO_LED1         CC2640R2_LAUNCHXL_GPIO_LED_GREEN
#define Board_GPIO_LED2         CC2640R2_LAUNCHXL_GPIO_LED_RED
#define Board_GPIO_RLED         CC2640R2_LAUNCHXL_GPIO_LED_RED
#define Board_GPIO_GLED         CC2640R2_LAUNCHXL_GPIO_LED_GREEN
#define Board_GPIO_LED_ON       CC2640R2_LAUNCHXL_GPIO_LED_ON
#define Board_GPIO_LED_OFF      CC2640R2_LAUNCHXL_GPIO_LED_OFF
#define Board_GPIO_TMP116_EN    CC2640R2_LAUNCHXL_GPIO_TMP116_EN

#define Board_GPTIMER0A         CC2640R2_LAUNCHXL_GPTIMER0A
#define Board_GPTIMER0B         CC2640R2_LAUNCHXL_GPTIMER0B
#define Board_GPTIMER1A         CC2640R2_LAUNCHXL_GPTIMER1A
#define Board_GPTIMER1B         CC2640R2_LAUNCHXL_GPTIMER1B
#define Board_GPTIMER2A         CC2640R2_LAUNCHXL_GPTIMER2A
#define Board_GPTIMER2B         CC2640R2_LAUNCHXL_GPTIMER2B
#define Board_GPTIMER3A         CC2640R2_LAUNCHXL_GPTIMER3A
#define Board_GPTIMER3B         CC2640R2_LAUNCHXL_GPTIMER3B

#define Board_I2C0              CC2640R2_LAUNCHXL_I2C0
#define Board_I2C_TMP           Board_I2C0

#define Board_NVSINTERNAL       CC2640R2_LAUNCHXL_NVSCC26XX0
#define Board_NVSEXTERNAL       CC2640R2_LAUNCHXL_NVSSPI25X0

#define Board_PIN_BUTTON0       CC2640R2_LAUNCHXL_PIN_BTN1
#define Board_PIN_BUTTON1       CC2640R2_LAUNCHXL_PIN_BTN2
#define Board_PIN_BTN1          CC2640R2_LAUNCHXL_PIN_BTN1
#define Board_PIN_BTN2          CC2640R2_LAUNCHXL_PIN_BTN2
#define Board_PIN_LED0          CC2640R2_LAUNCHXL_PIN_RLED
#define Board_PIN_LED1          CC2640R2_LAUNCHXL_PIN_GLED
#define Board_PIN_LED2          CC2640R2_LAUNCHXL_PIN_RLED
#define Board_PIN_RLED          CC2640R2_LAUNCHXL_PIN_RLED
#define Board_PIN_GLED          CC2640R2_LAUNCHXL_PIN_GLED

#define Board_PWM0              CC2640R2_LAUNCHXL_PWM0
#define Board_PWM1              CC2640R2_LAUNCHXL_PWM1
#define Board_PWM2              CC2640R2_LAUNCHXL_PWM2
#define Board_PWM3              CC2640R2_LAUNCHXL_PWM3
#define Board_PWM4              CC2640R2_LAUNCHXL_PWM4
#define Board_PWM5              CC2640R2_LAUNCHXL_PWM5
#define Board_PWM6              CC2640R2_LAUNCHXL_PWM6
#define Board_PWM7              CC2640R2_LAUNCHXL_PWM7

#define Board_SD0               CC2640R2_LAUNCHXL_SDSPI0

#define Board_SPI0              CC2640R2_LAUNCHXL_SPI0
#define Board_SPI1              CC2640R2_LAUNCHXL_SPI1
#define Board_SPI_FLASH_CS      CC2640R2_LAUNCHXL_SPI_FLASH_CS
#define Board_FLASH_CS_ON       0
#define Board_FLASH_CS_OFF      1

#define Board_SPI_MASTER        CC2640R2_LAUNCHXL_SPI0
#define Board_SPI_SLAVE         CC2640R2_LAUNCHXL_SPI0
#define Board_SPI_MASTER_READY  CC2640R2_LAUNCHXL_SPI_MASTER_READY
#define Board_SPI_SLAVE_READY   CC2640R2_LAUNCHXL_SPI_SLAVE_READY
#define Board_UART0             CC2640R2_LAUNCHXL_UART0

#define Board_WATCHDOG0         CC2640R2_LAUNCHXL_WATCHDOG0

/*
 * These macros are provided for backwards compatibility.
 * Please use the <Driver>_init functions directly rather
 * than Board_init<Driver>.
 */
#define Board_initADC()         ADC_init()
#define Board_initADCBuf()      ADCBuf_init()
#define Board_initGPIO()        GPIO_init()
#define Board_initPWM()         PWM_init()
#define Board_initSPI()         SPI_init()
#define Board_initUART()        UART_init()
#define Board_initWatchdog()    Watchdog_init()

/*
 * These macros are provided for backwards compatibility.
 * Please use the 'Board_PIN_xxx' macros to differentiate
 * them from the 'Board_GPIO_xxx' macros.
 */
#define Board_BUTTON0           Board_PIN_BUTTON0
#define Board_BUTTON1           Board_PIN_BUTTON1
#define Board_BTN1              Board_PIN_BTN1
#define Board_BTN2              Board_PIN_BTN2
#define Board_LED_ON            Board_GPIO_LED_ON
#define Board_LED_OFF           Board_GPIO_LED_OFF
#define Board_LED0              Board_PIN_LED0
#define Board_LED1              Board_PIN_LED1
#define Board_LED2              Board_PIN_LED2
#define Board_RLED              Board_PIN_RLED
#define Board_GLED              Board_PIN_GLED
#define Board_ADCBUFCHANNEL0    Board_ADCBUF0CHANNEL0
#define Board_ADCBUFCHANNEL1    Board_ADCBUF0CHANNEL1

#endif /* STARTUP_BOARD_H_ */
