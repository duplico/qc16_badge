/*
 * epd_phy.h
 *
 *  Created on: May 6, 2019
 *      Author: george
 */

#ifndef QUEERCON_EPD_PHY_H_
#define QUEERCON_EPD_PHY_H_

#include <stdint.h>

#define EPD_WIDTH  128
#define EPD_HEIGHT 296

// TODO: Move to board.h?

#define EPAPER_SDIO     IOID_9
#define EPAPER_SCLK     IOID_10
#define EPAPER_CSN      CC2640R2_LAUNCHXL_DIO0
#define EPAPER_DCN      CC2640R2_LAUNCHXL_DIO1_RFSW
#define EPAPER_RESN     CC2640R2_LAUNCHXL_DIO12
#define EPAPER_BUSY     CC2640R2_LAUNCHXL_DIO15

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

extern uint8_t epd_display_buffer[(EPD_HEIGHT * EPD_WIDTH + 7) / 8];

// TODO: Refactor these out of here: (make them static)
void epd_phy_spi_cmd(uint8_t cmd);
void epd_phy_spi_data(uint8_t dat);

/// Activate the display to show the written buffer.
void epd_phy_activate();
/// Clear the screen.
void epd_clear();
/// Enter deep sleep mode.
void epd_phy_deepsleep();
/// Initialize the GPIO and peripherals needed for the EPD.
void epd_phy_init_gpio();
/// Initialize the display buffer for the e-paper display.
void epd_init_display_buffer(uint16_t ulValue);
/// Reset, wake, and initialize the e-paper display, to get it ready to write.
void epd_phy_init(uint8_t partial_update);
void epd_phy_flush_buffer();

#endif /* QUEERCON_EPD_PHY_H_ */
