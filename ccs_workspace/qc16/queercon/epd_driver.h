/*
 * epd_driver.h
 *
 *  Created on: May 6, 2019
 *      Author: george
 */

#ifndef QUEERCON_EPD_DRIVER_H_
#define QUEERCON_EPD_DRIVER_H_

#include <ti/grlib/grlib.h>

//*****************************************************************************
// Number of pixels on LCD X-axis
#define LCD_X_SIZE 128 //296
// Number of pixels on LCD Y-axis
#define LCD_Y_SIZE 296 //128
// Number of bits required to draw one pixel on the LCD screen
#define BPP 1

#define EPD_WIDTH       128
#define EPD_HEIGHT      296

// Define LCD Screen Orientation Here
//#define LANDSCAPE
//#define LANDSCAPE_FLIP
#define PORTRAIT
//#define PORTRAIT_FLIP


//*****************************************************************************
//
// Defines for the pins that are used to communicate with the LCD Driver
//
//*****************************************************************************

// TODO: Mode:

#define BIT0 0b00000001
#define BIT1 0b00000010
#define BIT2 0b00000100
#define BIT3 0b00001000
#define BIT4 0b00010000
#define BIT5 0b00100000
#define BIT6 0b01000000
#define BIT7 0b10000000

// TODO: Move to board.h?

#define EPAPER_SDIO     IOID_9
#define EPAPER_SCLK     IOID_10
#define EPAPER_CSN      CC2640R2_LAUNCHXL_DIO0
#define EPAPER_DCN      CC2640R2_LAUNCHXL_DIO1_RFSW
#define EPAPER_RESN     CC2640R2_LAUNCHXL_DIO12
#define EPAPER_BUSY     CC2640R2_LAUNCHXL_DIO15

//*****************************************************************************
//
// Defines for LCD driver configuration
//
//*****************************************************************************

/* Defines for pixels, colors, masks, etc. Anything qc12_oled.c needs*/


//*****************************************************************************
//
// This driver operates in four different screen orientations.  They are:
//
// * Portrait - The screen is taller than it is wide. This is selected by defining
//              PORTRAIT.
//
// * Landscape - The screen is wider than it is tall. This is selected by defining
//               LANDSCAPE.
//
// * Portrait flip - The screen is taller than it is wide. This is
//                   selected by defining PORTRAIT_FLIP.
//
// * Landscape flip - The screen is wider than it is tall. This is
//                    selected by defining LANDSCAPE_FLIP.
//
// These can also be imagined in terms of screen rotation; if landscape mode is
// 0 degrees of screen rotation, portrait flip is 90 degrees of clockwise
// rotation, landscape flip is 180 degrees of rotation, and portrait is
// 270 degress of clockwise rotation.
//
// If no screen orientation is selected, "landscape" mode will be used.
//
//*****************************************************************************
#if ! defined(PORTRAIT) && ! defined(PORTRAIT_FLIP) && \
    ! defined(LANDSCAPE) && ! defined(LANDSCAPE_FLIP)
#define LANDSCAPE
#endif

//*****************************************************************************
//
// Various definitions controlling coordinate space mapping and drawing
// direction in the four supported orientations.
//
//*****************************************************************************
#ifdef PORTRAIT
#define MAPPED_X(x, y) (LCD_X_SIZE - (y) - 1)
#define MAPPED_Y(x, y) (x)
#endif
#ifdef LANDSCAPE
#define MAPPED_X(x, y) (x)
#define MAPPED_Y(x, y) (y)
#endif
#ifdef PORTRAIT_FLIP
#define MAPPED_X(x, y)  (y)
#define MAPPED_Y(x, y)  (x)
#endif
#ifdef LANDSCAPE_FLIP
#define MAPPED_X(x, y)  (LCD_X_SIZE - (x) - 1)
#define MAPPED_Y(x, y)  (LCD_Y_SIZE - (y) - 1)
#endif


//*****************************************************************************
//
// Various LCD Controller command name labels and associated control bits
//
//*****************************************************************************

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

//*****************************************************************************
//
// Macros for the Display Driver
//
//*****************************************************************************

/* All macros can go here. This is typically the color translation function (example below)
and could also include Set_Address(), Write_Data(), etc. */


//
// Translates a 24-bit RGB color to a display driver-specific color.
//
// \param c is the 24-bit RGB color.  The least-significant byte is the blue
// channel, the next byte is the green channel, and the third byte is the red
// channel.
//
// This macro translates a 24-bit RGB color into a value that can be written
// into the display's frame buffer in order to reproduce that color, or the
// closest possible approximation of that color. This particular example
// requires the 8-8-8 24 bit RGB color to convert into 5-6-5 16 bit RGB Color
// Your conversion should be made to fit your LCD settings.
//
// \return Returns the display-driver specific color

#define DPYCOLORTRANSLATE(c) (c ? 1 : 0)

//*****************************************************************************
//
// Prototypes for the globals exported by this driver.
//
//*****************************************************************************
extern void qc12_oledInit(uint8_t invert);
extern void qc12oled_WriteCommand(uint8_t ucCommand);
extern Graphics_Display g_sqc12_oled;
extern const Graphics_Display_Functions gf_epd;
extern uint8_t oled_memory[];

#endif /* QUEERCON_EPD_DRIVER_H_ */
