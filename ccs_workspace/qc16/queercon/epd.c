/*
 * epd.c
 *
 *  Created on: Apr 11, 2019
 *      Author: george
 */

#include <stdlib.h>

#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include "epd.h"

Task_Struct epaperTask;
Char epaperTaskStack[660];

#define EPAPER_SDIO     IOID_9
#define EPAPER_SCLK     IOID_10
#define EPAPER_CSN      CC2640R2_LAUNCHXL_DIO0
#define EPAPER_DCN      CC2640R2_LAUNCHXL_DIO1_RFSW
#define EPAPER_RESN     CC2640R2_LAUNCHXL_DIO12
#define EPAPER_BUSY     CC2640R2_LAUNCHXL_DIO15

unsigned char masterRxBuffer[5];
unsigned char masterTxBuffer[5];

SPI_Handle epaper_spi;
PIN_Handle epaper_pin;

void spi_cmd(uint8_t cmd) {
    uint8_t tx_buf[1];
    tx_buf[0] = cmd;
    SPI_Transaction transaction;
    transaction.count = 1;
    transaction.txBuf = (void *) tx_buf;
    transaction.rxBuf = (void *) masterRxBuffer;
    // Set DC low for CMD
    PIN_setOutputValue(epaper_pin, EPAPER_DCN, 0);
    // Set CS low
    PIN_setOutputValue(epaper_pin, EPAPER_CSN, 0);
    // Transmit
    SPI_transfer(epaper_spi, &transaction);
    // Set CS high
    PIN_setOutputValue(epaper_pin, EPAPER_CSN, 1);
}

void spi_data(uint8_t dat) {
    uint8_t tx_buf[1];
    tx_buf[0] = dat;
    SPI_Transaction transaction;
    transaction.count = 1;
    transaction.txBuf = (void *) tx_buf;
    transaction.rxBuf = (void *) masterRxBuffer;
    // Set DC high for DATA
    PIN_setOutputValue(epaper_pin, EPAPER_DCN, 1);
    // Set CS low
    PIN_setOutputValue(epaper_pin, EPAPER_CSN, 0);
    // Transmit
    SPI_transfer(epaper_spi, &transaction);
    // Set CS high
    PIN_setOutputValue(epaper_pin, EPAPER_CSN, 1);
}


const unsigned char lut_full_update[] = {
    0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22,
    0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88,
    0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51,
    0x35, 0x51, 0x51, 0x19, 0x01, 0x00
};

const unsigned char lut_partial_update[] = {
    0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// EPD2IN9 commands
#define DRIVER_OUTPUT_CONTROL                       0x01
#define BOOSTER_SOFT_START_CONTROL                  0x0C
#define GATE_SCAN_START_POSITION                    0x0F
#define DEEP_SLEEP_MODE                             0x10
#define DATA_ENTRY_MODE_SETTING                     0x11
#define SW_RESET                                    0x12
#define TEMPERATURE_SENSOR_CONTROL                  0x1A
#define MASTER_ACTIVATION                           0x20
#define DISPLAY_UPDATE_CONTROL_1                    0x21
#define DISPLAY_UPDATE_CONTROL_2                    0x22
#define WRITE_RAM                                   0x24
#define WRITE_VCOM_REGISTER                         0x2C
#define WRITE_LUT_REGISTER                          0x32
#define SET_DUMMY_LINE_PERIOD                       0x3A
#define SET_GATE_TIME                               0x3B
#define BORDER_WAVEFORM_CONTROL                     0x3C
#define SET_RAM_X_ADDRESS_START_END_POSITION        0x44
#define SET_RAM_Y_ADDRESS_START_END_POSITION        0x45
#define SET_RAM_X_ADDRESS_COUNTER                   0x4E
#define SET_RAM_Y_ADDRESS_COUNTER                   0x4F
#define TERMINATE_FRAME_READ_WRITE                  0xFF

/******************************************************************************
function :  Software reset
parameter:
******************************************************************************/
static void EPD_Reset(void)
{

    // Reset display driver IC (Pulse EPAPER_RESN low for ?????)
    PIN_setOutputValue(epaper_pin, EPAPER_RESN, 1);
    Task_sleep(100000); // Sleep system ticks (not sure how long these are)
    PIN_setOutputValue(epaper_pin, EPAPER_RESN, 0);
    Task_sleep(100000); // Sleep system ticks (not sure how long these are)
    PIN_setOutputValue(epaper_pin, EPAPER_RESN, 1);
    Task_sleep(100000); // Sleep system ticks (not sure how long these are)
}

/******************************************************************************
function :  Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
void EPD_WaitUntilIdle(void)
{
    // Wait for busy=high
    while (PIN_getInputValue(EPAPER_BUSY)); //LOW: idle, HIGH: busy
}

/******************************************************************************
function :  Setting the display window
parameter:
******************************************************************************/
static void EPD_SetWindows(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend)
{
    spi_cmd(SET_RAM_X_ADDRESS_START_END_POSITION);
    spi_data((Xstart >> 3) & 0xFF);
    spi_data((Xend >> 3) & 0xFF);

    spi_cmd(SET_RAM_Y_ADDRESS_START_END_POSITION);
    spi_data(Ystart & 0xFF);
    spi_data((Ystart >> 8) & 0xFF);
    spi_data(Yend & 0xFF);
    spi_data((Yend >> 8) & 0xFF);
}

/******************************************************************************
function :  Set Cursor
parameter:
******************************************************************************/
static void EPD_SetCursor(uint16_t Xstart, uint16_t Ystart)
{
    spi_cmd(SET_RAM_X_ADDRESS_COUNTER);
    spi_data((Xstart >> 3) & 0xFF);

    spi_cmd(SET_RAM_Y_ADDRESS_COUNTER);
    spi_data(Ystart & 0xFF);
    spi_data((Ystart >> 8) & 0xFF);

}

/******************************************************************************
function :  Turn On Display
parameter:
******************************************************************************/
static void EPD_TurnOnDisplay(void)
{
    spi_cmd(DISPLAY_UPDATE_CONTROL_2);
    spi_data(0xC4);
    spi_cmd(MASTER_ACTIVATION);
    spi_cmd(TERMINATE_FRAME_READ_WRITE);

    EPD_WaitUntilIdle();
}

/******************************************************************************
function :  Initialize the e-Paper register
parameter:
******************************************************************************/
uint8_t EPD_Init(const unsigned char* lut)
{
    EPD_Reset();

    spi_cmd(DRIVER_OUTPUT_CONTROL);
    spi_data((EPD_HEIGHT - 1) & 0xFF);
    spi_data(((EPD_HEIGHT - 1) >> 8) & 0xFF);
    spi_data(0x00);                     // GD = 0; SM = 0; TB = 0;
    spi_cmd(BOOSTER_SOFT_START_CONTROL);
    spi_data(0xD7);
    spi_data(0xD6);
    spi_data(0x9D);
    spi_cmd(WRITE_VCOM_REGISTER);
    spi_data(0xA8);                     // VCOM 7C
    spi_cmd(SET_DUMMY_LINE_PERIOD);
    spi_data(0x1A);                     // 4 dummy lines per gate
    spi_cmd(SET_GATE_TIME);
    spi_data(0x08);                     // 2us per line
    spi_cmd(BORDER_WAVEFORM_CONTROL);
    spi_data(0x03);
    spi_cmd(DATA_ENTRY_MODE_SETTING);
    spi_data(0x03);

    //set the look-up table register
    spi_cmd(WRITE_LUT_REGISTER);
    for (uint16_t i = 0; i < 30; i++) {
        spi_data(lut[i]);
    }
    return 0;
}

/******************************************************************************
function :  Clear screen
parameter:
******************************************************************************/
void EPD_Clear(void)
{
    uint16_t Width, Height;
    Width = (EPD_WIDTH % 8 == 0)? (EPD_WIDTH / 8 ): (EPD_WIDTH / 8 + 1);
    Height = EPD_HEIGHT;
    EPD_SetWindows(0, 0, EPD_WIDTH, EPD_HEIGHT);
    for (uint16_t j = 0; j < Height; j++) {
        EPD_SetCursor(0, j);
        spi_cmd(WRITE_RAM);
        for (uint16_t i = 0; i < Width; i++) {
            spi_data(0XFF);
        }
    }
    EPD_TurnOnDisplay();
}

/******************************************************************************
function :  Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_Display(uint8_t *Image)
{
    uint16_t Width, Height;
    Width = (EPD_WIDTH % 8 == 0)? (EPD_WIDTH / 8 ): (EPD_WIDTH / 8 + 1);
    Height = EPD_HEIGHT;

    uint32_t Addr = 0;
    // UDOUBLE Offset = ImageName;
    EPD_SetWindows(0, 0, EPD_WIDTH, EPD_HEIGHT);
    for (uint16_t j = 0; j < Height; j++) {
        EPD_SetCursor(0, j);
        spi_cmd(WRITE_RAM);
        for (uint16_t i = 0; i < Width; i++) {
            Addr = i + j * Width;
            spi_data(Image[Addr]);
        }
    }
    EPD_TurnOnDisplay();
}

/******************************************************************************
function :  Enter sleep mode
parameter:
******************************************************************************/
void EPD_Sleep(void)
{
    spi_cmd(DEEP_SLEEP_MODE);
    spi_data(0x01);
    // EPD_WaitUntilIdle();
}

uint8_t black_image[16*296] = {0,};

void epaper_spi_task_fn(UArg a0, UArg a1)
{
    PIN_State epaper_pin_state;
    PIN_Config BoardGpioInitTable[] = {
    EPAPER_CSN | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MIN,
    EPAPER_DCN | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MIN,
    EPAPER_RESN | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MIN,
    EPAPER_BUSY | PIN_INPUT_EN,
    PIN_TERMINATE
    };

    epaper_pin = PIN_open(&epaper_pin_state, BoardGpioInitTable);

    SPI_Params      spiParams;

    /* Open SPI as master (default) */
    SPI_Params_init(&spiParams); // Defaults are OK
    epaper_spi = SPI_open(Board_SPI_MASTER, &spiParams);
    if (epaper_spi == NULL) {
        while (1);
    }

    EPD_Init(lut_full_update);
    EPD_Clear();

    for (uint16_t i=0; i<16*296; i++) {
        black_image[i] = 0b10101010;
    }
    EPD_Display(black_image);

    EPD_Sleep();

    for (;;)
    {
        // Get the ticks since startup
        uint32_t tickStart = Clock_getTicks();
        Task_yield();
    }
}

