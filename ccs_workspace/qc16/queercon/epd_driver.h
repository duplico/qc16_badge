/*
 * epd_driver.h
 *
 *  Created on: May 6, 2019
 *      Author: george
 */

#ifndef QUEERCON_EPD_DRIVER_H_
#define QUEERCON_EPD_DRIVER_H_

#include <ti/grlib/grlib.h>

#include "epd_phy.h"

//*****************************************************************************
// Number of pixels on LCD X-axis
#define LCD_X_SIZE EPD_WIDTH
// Number of pixels on LCD Y-axis
#define LCD_Y_SIZE EPD_HEIGHT
// Number of bits required to draw one pixel on the LCD screen
#define BPP 1

// Define LCD Screen Orientation Here
//#define PORTRAIT
//#define PORTRAIT_FLIP
#define LANDSCAPE
//#define LANDSCAPE_FLIP


//*****************************************************************************
//
// Defines for the pins that are used to communicate with the LCD Driver
//
//*****************************************************************************

// TODO: Move to qc16.h once created:

#define BIT0 0b00000001
#define BIT1 0b00000010
#define BIT2 0b00000100
#define BIT3 0b00001000
#define BIT4 0b00010000
#define BIT5 0b00100000
#define BIT6 0b01000000
#define BIT7 0b10000000


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
//              LANDSCAPE.
//
// * Landscape - The screen is wider than it is tall. This is selected by defining
//               PORTRAIT.
//
// * Portrait flip - The screen is taller than it is wide. This is
//                   selected by defining LANDSCAPE_FLIP.
//
// * Landscape flip - The screen is wider than it is tall. This is
//                    selected by defining PORTRAIT_FLIP.
//
// These can also be imagined in terms of screen rotation; if landscape mode is
// 0 degrees of screen rotation, portrait flip is 90 degrees of clockwise
// rotation, landscape flip is 180 degrees of rotation, and portrait is
// 270 degress of clockwise rotation.
//
// If no screen orientation is selected, "landscape" mode will be used.
//
//*****************************************************************************
#if ! defined(LANDSCAPE) && ! defined(LANDSCAPE_FLIP) && \
    ! defined(PORTRAIT) && ! defined(PORTRAIT_FLIP)
#define LANDSCAPE
#endif

//*****************************************************************************
//
// Various definitions controlling coordinate space mapping and drawing
// direction in the four supported orientations.
//
//*****************************************************************************
#ifdef LANDSCAPE
#define MAPPED_X(x, y) (LCD_X_SIZE - (y) - 1)
#define MAPPED_Y(x, y) (x)
#endif
#ifdef PORTRAIT
#define MAPPED_X(x, y) (x)
#define MAPPED_Y(x, y) (y)
#endif
#ifdef LANDSCAPE_FLIP
#define MAPPED_X(x, y)  (y)
#define MAPPED_Y(x, y)  (x)
#endif
#ifdef PORTRAIT_FLIP
#define MAPPED_X(x, y)  (LCD_X_SIZE - (x) - 1)
#define MAPPED_Y(x, y)  (LCD_Y_SIZE - (y) - 1)
#endif


//*****************************************************************************
//
// Various LCD Controller command name labels and associated control bits
//
//*****************************************************************************


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
// \return Returns BLACK if the input is black; WHITE otherwise.

#define DPYCOLORTRANSLATE(c) (c ? 1 : 0)

//*****************************************************************************
//
// Prototypes for the globals exported by this driver.
//
//*****************************************************************************
extern void init_epd();
extern void qc12oled_WriteCommand(uint8_t ucCommand);
extern Graphics_Display epd_grGraphicsDisplay;
extern const Graphics_Display_Functions epd_grDisplayFunctions;

#endif /* QUEERCON_EPD_DRIVER_H_ */
