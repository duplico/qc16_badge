/*
 * epd_driver.c
 *
 *  Created on: May 6, 2019
 *      Author: george
 */


/* --COPYRIGHT--,BSD
 * Copyright (c) 2013, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
//*****************************************************************************
//
//! \addtogroup display_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// READ ME
//
// This template driver is intended to be modified for creating new LCD drivers
// It is setup so that only qc12_oledPixelDraw() and DPYCOLORTRANSLATE()
// and some LCD size configuration settings in the header file qc12_oled.h
// are REQUIRED to be written. These functions are marked with the string
// "TemplateDisplayFix" in the comments so that a search through qc12_oled.c and
// qc12_oled.h can quickly identify the necessary areas of change.
//
// qc12_oledPixelDraw() is the base function to write to the LCD
// display. Functions like WriteData(), WriteCommand(), and SetAddress()
// are suggested to be used to help implement the qc12_oledPixelDraw()
// function, but are not required. SetAddress() should be used by other pixel
// level functions to help optimize them.
//
// This is not an optimized driver however and will significantly impact
// performance. It is highly recommended to first get the prototypes working
// with the single pixel writes, and then go back and optimize the driver.
// Please see application note www.ti.com/lit/pdf/slaa548 for more information
// on how to fully optimize LCD driver files. In int16_t, driver optimizations
// should take advantage of the auto-incrementing of the LCD controller.
// This should be utilized so that a loop of WriteData() can be used instead
// of a loop of qc12_oledPixelDraw(). The pixel draw loop contains both a
// SetAddress() + WriteData() compared to WriteData() alone. This is a big time
// saver especially for the line draws and qc12_oledPixelDrawMultiple.
// More optimization can be done by reducing function calls by writing macros,
// eliminating unnecessary instructions, and of course taking advantage of other
// features offered by the LCD controller. With so many pixels on an LCD screen
// each instruction can have a large impact on total drawing time.
//
//*****************************************************************************


//*****************************************************************************
//
// Include Files
//
//*****************************************************************************

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <ti/grlib/grlib.h>
//#include <driverlib/MSP430FR5xx_6xx/driverlib.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>

#include <board.h>
#include "epd_driver.h"

//*****************************************************************************
//
// Global Variables
//
//*****************************************************************************

/* Global buffer for the display. */
uint8_t oled_memory[(LCD_X_SIZE * LCD_Y_SIZE * BPP + 7) / 8];

//*****************************************************************************
//
// Suggested functions to help facilitate writing the required functions below
//
//*****************************************************************************

#define GRAM_BUFFER(mapped_x, mapped_y) oled_memory[((LCD_X_SIZE/8) * mapped_y) + (mapped_x / 8)]

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
    // TODO: yield
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
function :  Clear screen
parameter:
******************************************************************************/
void EPD_Clear(void)
{
    uint16_t Width, Height;
    Width = (LCD_X_SIZE % 8 == 0)? (LCD_X_SIZE / 8 ): (LCD_X_SIZE / 8 + 1);
    Height = LCD_Y_SIZE;
    EPD_SetWindows(0, 0, LCD_X_SIZE, LCD_Y_SIZE);
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
function :  Enter sleep mode
parameter:
******************************************************************************/
void EPD_Sleep(void)
{
    // TODO: Actually use this.
    spi_cmd(DEEP_SLEEP_MODE);
    spi_data(0x01);
    // EPD_WaitUntilIdle();
}

// Initializes the pins required for the GPIO-based LCD interface.
// This function configures the GPIO pins used to control the LCD display
// when the basic GPIO interface is in use. On exit, the LCD controller
// has been reset and is ready to receive command and data writes.
static void InitGPIOLCDInterface()
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
}


// Initialize DisplayBuffer.
// This function initializes the display buffer and discards any cached data.
static void
InitLCDDisplayBuffer(uint16_t ulValue)
{
    // Ok, so this buffer contains the data to be displayed.
    //  Each pixel is just one bit. Natively this is "portrait" address
    // TODO: Is this explanation correct?
    //  mode, with the first byte occupying the leftmost 8 pixels
    //  of the top row, with the connector at the bottom.

    // Therefore, our buffer is height*width/8 bytes.

    // Let's clear it out.
    uint8_t init_byte = ulValue ? 0xff : 0x00;
    memset(oled_memory, init_byte, sizeof(oled_memory));
}

uint8_t EPD_Init(const unsigned char* lut)
{
    EPD_Reset();

    spi_cmd(DRIVER_OUTPUT_CONTROL);
    spi_data((LCD_Y_SIZE - 1) & 0xFF);
    spi_data(((LCD_Y_SIZE - 1) >> 8) & 0xFF);
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

// Initializes the display driver.
// This function initializes the LCD controller
// TemplateDisplayFix
void
qc12_oledInit(uint8_t invert)
{
    InitGPIOLCDInterface();
    InitLCDDisplayBuffer(0);
    EPD_Init(lut_full_update);
//    EPD_Clear();
//    EPD_Sleep();
}

//*****************************************************************************
//
// All the following functions (below) for the LCD driver are required by grlib
//
//*****************************************************************************

//*****************************************************************************
//
//! Draws a pixel on the screen.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX is the X coordinate of the pixel.
//! \param lY is the Y coordinate of the pixel.
//! \param ulValue is the color of the pixel.
//!
//! This function sets the given pixel to a particular color.  The coordinates
//! of the pixel are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
// TemplateDisplayFix
static void
qc12_oledPixelDraw(const Graphics_Display * pvDisplayData, int16_t lX, int16_t lY,
                                   uint16_t ulValue)
{
    if (lX < 0 || lY < 0) {
        return;
    }
    /* This function already has checked that the pixel is within the extents of
       the LCD screen and the color ulValue has already been translated to the LCD.
     */
    uint16_t mapped_x = MAPPED_X(lX, lY);
    uint16_t mapped_y = MAPPED_Y(lX, lY);

    // Now, mapped_x and mapped_y are the correct indices, transformed
    //  if necessary, to index our pixel buffer. We can get the needed
    //  byte, with GRAM_BUFFER. And then we need to RIGHT-shift the
    //  X coordinate, as needed.
    uint8_t val = BIT7 >> (mapped_x % 8);

    // TODO:
    volatile uint16_t buffer_addr;
    buffer_addr = ((LCD_X_SIZE/8) * mapped_y) + (mapped_x / 8);

    if (ulValue) {
//        GRAM_BUFFER(mapped_y, mapped_x) |= val;
        oled_memory[buffer_addr] |= val;
    } else {
//        GRAM_BUFFER(mapped_y, mapped_x) &= ~val;
        oled_memory[buffer_addr] &= ~val;
    }
}

//*****************************************************************************
//
//! Draws a horizontal sequence of pixels on the screen.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX is the X coordinate of the first pixel.
//! \param lY is the Y coordinate of the first pixel.
//! \param lX0 is sub-pixel offset within the pixel data, which is valid for 1
//! or 4 bit per pixel formats.
//! \param lCount is the number of pixels to draw.
//! \param lBPP is the number of bits per pixel; must be 1, 4, or 8.
//! \param pucData is a pointer to the pixel data.  For 1 and 4 bit per pixel
//! formats, the most significant bit(s) represent the left-most pixel.
//! \param pucPalette is a pointer to the palette used to draw the pixels.
//!
//! This function draws a horizontal sequence of pixels on the screen, using
//! the supplied palette.  For 1 bit per pixel format, the palette contains
//! pre-translated colors; for 4 and 8 bit per pixel formats, the palette
//! contains 24-bit RGB values that must be translated before being written to
//! the display.
//!
//! \return None.
//
//*****************************************************************************
static void
qc12_oledPixelDrawMultiple(const Graphics_Display * pvDisplayData, int16_t lX,
                                           int16_t lY, int16_t lX0, int16_t lCount,
                                           int16_t lBPP,
                                           const uint8_t *pucData,
                                           const uint32_t *pucPalette)
{
    uint16_t ulByte;
    // Loop while there are more pixels to draw
    while(lCount > 0)
    {
        // Get the next byte of image data
        ulByte = *pucData++;

        // Loop through the pixels in this byte of image data
        for(; (lX0 < 8) && lCount; lX0++, lCount--)
        {
            // Draw this pixel in the appropriate color
            if (((uint16_t *)pucPalette)[(ulByte >> (7 - lX0)) & 1]) {
                qc12_oledPixelDraw(pvDisplayData, lX, lY, 1);
            }
            lX++;
        }

        // Start at the beginning of the next byte of image data
        lX0 = 0;
    }
}

//*****************************************************************************
//
//! Draws a horizontal line.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX1 is the X coordinate of the start of the line.
//! \param lX2 is the X coordinate of the end of the line.
//! \param lY is the Y coordinate of the line.
//! \param ulValue is the color of the line.
//!
//! This function draws a horizontal line on the display.  The coordinates of
//! the line are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
static void
qc12_oledLineDrawH(const Graphics_Display * pvDisplayData, int16_t lX1, int16_t lX2,
                                   int16_t lY, uint16_t ulValue)
{
  /* Ideally this function shouldn't call pixel draw. It should have it's own
  definition using the built in auto-incrementing of the LCD controller and its
  own calls to SetAddress() and WriteData(). Better yet, SetAddress() and WriteData()
  can be made into macros as well to eliminate function call overhead. */

  do
  {
    qc12_oledPixelDraw(pvDisplayData, lX1, lY, ulValue);
  }
  while(lX1++ < lX2);
}

//*****************************************************************************
//
//! Draws a vertical line.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX is the X coordinate of the line.
//! \param lY1 is the Y coordinate of the start of the line.
//! \param lY2 is the Y coordinate of the end of the line.
//! \param ulValue is the color of the line.
//!
//! This function draws a vertical line on the display.  The coordinates of the
//! line are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
static void
qc12_oledLineDrawV(const Graphics_Display * pvDisplayData, int16_t lX, int16_t lY1,
                                   int16_t lY2, uint16_t ulValue)
{
  do
  {
    qc12_oledPixelDraw(pvDisplayData, lX, lY1, ulValue);
  }
  while(lY1++ < lY2);
}

//*****************************************************************************
//
//! Fills a rectangle.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param pRect is a pointer to the structure describing the rectangle.
//! \param ulValue is the color of the rectangle.
//!
//! This function fills a rectangle on the display.  The coordinates of the
//! rectangle are assumed to be within the extents of the display, and the
//! rectangle specification is fully inclusive (in other words, both sXMin and
//! sXMax are drawn, along with sYMin and sYMax).
//!
//! \return None.
//
//*****************************************************************************
static void
qc12_oledRectFill(const Graphics_Display * pvDisplayData, const tRectangle *pRect,
                                  uint16_t ulValue)
{
  int16_t x0 = pRect->sXMin;
  int16_t x1 = pRect->sXMax;
  int16_t y0 = pRect->sYMin;
  int16_t y1 = pRect->sYMax;

  while(y0++ <= y1)
  {
    qc12_oledLineDrawH(pvDisplayData, x0, x1, y0, ulValue);
  }
}

//*****************************************************************************
//
//! Translates a 24-bit RGB color to a display driver-specific color.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param ulValue is the 24-bit RGB color.  The least-significant byte is the
//! blue channel, the next byte is the green channel, and the third byte is the
//! red channel.
//!
//! This function translates a 24-bit RGB color into a value that can be
//! written into the display's frame buffer in order to reproduce that color,
//! or the closest possible approximation of that color.
//!
//! \return Returns the display-driver specific color.
//
//*****************************************************************************
static uint32_t qc12_oledColorTranslate(const Graphics_Display * pvDisplayData,
                                        uint32_t  ulValue)
{
    /* The DPYCOLORTRANSLATE macro should be defined in TemplateDriver.h */

    //
    // Translate from a 24-bit RGB color to a color accepted by the LCD.
    //
    return(DPYCOLORTRANSLATE(ulValue));
}

/******************************************************************************
function :  Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_Display(uint8_t *Image)
{
    uint16_t Width, Height;
    Width = (LCD_X_SIZE % 8 == 0)? (LCD_X_SIZE / 8 ): (LCD_X_SIZE / 8 + 1);
    Height = LCD_Y_SIZE;

    uint32_t Addr = 0;
    // UDOUBLE Offset = ImageName;
    EPD_SetWindows(0, 0, LCD_X_SIZE, LCD_Y_SIZE);
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

//*****************************************************************************
//
//! Flushes any cached drawing operations.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//!
//! This functions flushes any cached drawing operations to the display.  This
//! is useful when a local frame buffer is used for drawing operations, and the
//! flush would copy the local frame buffer to the display.
//!
//! \return None.
//
//*****************************************************************************
static void
qc12_oledFlush(const Graphics_Display *pvDisplayData)
{
    // TODO: Determine whether to clear and replace, or do a selective update.
    EPD_Display(oled_memory);
//    EPD_Sleep();
}

//*****************************************************************************
//
//! Send command to clear screen.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//!
//! This function does a clear screen and the Display Buffer contents
//! are initialized to the current background color.
//!
//! \return None.
//
//*****************************************************************************
static void
qc12_oledClearScreen (const Graphics_Display *pvDisplayData, uint16_t ulValue)
{
    // This fills the entire display to clear it
    // Some LCD drivers support a simple command to clear the display
    // TODO: implement differently.

    uint8_t init_byte = ulValue ? 0xff : 0x00;
    memset(oled_memory, init_byte, sizeof(oled_memory));
}

//*****************************************************************************
//
//! The display structure that describes the driver for the blank template.
//
//*****************************************************************************
const Graphics_Display_Functions gf_epd =
{
    .pfnPixelDraw = qc12_oledPixelDraw,
    .pfnPixelDrawMultiple = qc12_oledPixelDrawMultiple,
    .pfnLineDrawH = qc12_oledLineDrawH,
    .pfnLineDrawV = qc12_oledLineDrawV,
    .pfnRectFill = qc12_oledRectFill,
    .pfnColorTranslate = qc12_oledColorTranslate,
    .pfnFlush = qc12_oledFlush,
    .pfnClearDisplay = qc12_oledClearScreen
};

//*****************************************************************************
//
//! This structure defines the characteristics of a display driver.
//
//*****************************************************************************

Graphics_Display g_sqc12_oled = {
    .size = sizeof(Graphics_Display),
    .displayData = oled_memory,
#if defined(LANDSCAPE) || defined(LANDSCAPE_FLIP)
    .width = LCD_Y_SIZE,
    .heigth = LCD_X_SIZE,
#else
    .width = LCD_X_SIZE,
    .heigth = LCD_Y_SIZE, // heigth??? lol.
#endif
    .pFxns = &gf_epd
};

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
