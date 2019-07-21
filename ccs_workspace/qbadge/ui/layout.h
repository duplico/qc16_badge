/*
 * ui_layout.h
 *
 *  Created on: Jul 12, 2019
 *      Author: george
 */

#ifndef UI_LAYOUT_H_
#define UI_LAYOUT_H_

#include <stdint.h>

#include <ti/grlib/grlib.h>

#define MAINMENU_ICON_COUNT 4
#define MAINMENU_NAME_MAX_LEN 12

#define FILES_LEFT_X 72

#define TOPBAR_HEIGHT 30
#define TOPBAR_ICON_HEIGHT 22
#define TOPBAR_ICON_WIDTH 22
#define TOPBAR_TEXT_WIDTH 18
#define TOPBAR_TEXT_HEIGHT (TOPBAR_HEIGHT - TOPBAR_ICON_HEIGHT)
#define TOPBAR_SEG_WIDTH (TOPBAR_ICON_WIDTH + TOPBAR_TEXT_WIDTH)
#define TOPBAR_SEG_PAD 3
#define TOPBAR_SEG_WIDTH_PADDED (TOPBAR_ICON_WIDTH + TOPBAR_TEXT_WIDTH + TOPBAR_SEG_PAD)
#define TOPBAR_SUB_WIDTH TOPBAR_SEG_WIDTH
#define TOPBAR_SUB_HEIGHT (TOPBAR_HEIGHT - TOPBAR_ICON_HEIGHT - 1)

#define TOPBAR_PROGBAR_HEIGHT 5

#define TOPBAR_HEADSUP_START (3*TOPBAR_SEG_WIDTH_PADDED)


#define BATTERY_BODY_WIDTH (TOPBAR_ICON_WIDTH-BATTERY_ANODE_WIDTH)
#define BATTERY_BODY_HEIGHT 7
#define BATTERY_BODY_VPAD 2
#define BATTERY_ANODE_WIDTH 2
#define BATTERY_ANODE_HEIGHT 4




#define BATTERY_X (EPD_HEIGHT-TOPBAR_ICON_WIDTH-1)
#define BATTERIES_HEIGHT (BATTERY_BODY_HEIGHT*2 + BATTERY_BODY_VPAD)

#define BATTERY_BODY0_Y0 ((TOPBAR_ICON_HEIGHT-BATTERIES_HEIGHT)/2-1)
#define BATTERY_BODY0_Y1 (BATTERY_BODY0_Y0+BATTERY_BODY_HEIGHT)
#define BATTERY_BODY1_Y0 (BATTERY_BODY0_Y1 + BATTERY_BODY_VPAD)
#define BATTERY_BODY1_Y1 (BATTERY_BODY1_Y0+BATTERY_BODY_HEIGHT)

#define BATTERY_ANODE0_Y0 (BATTERY_BODY0_Y0+(BATTERY_BODY_HEIGHT)/2-BATTERY_ANODE_HEIGHT/2)
#define BATTERY_ANODE1_Y0 (BATTERY_BODY1_Y0+(BATTERY_BODY_HEIGHT)/2-BATTERY_ANODE_HEIGHT/2)

#define BATTERY_SEGMENT_PAD 1
#define BATTERY_SEGMENT_WIDTH ((BATTERY_BODY_WIDTH/3)-BATTERY_SEGMENT_PAD)

#define BATTERY_LOW_X0_OFFSET 0
#define BATTERY_MID_X0_OFFSET (BATTERY_SEGMENT_WIDTH + BATTERY_SEGMENT_PAD)
#define BATTERY_HIGH_X0_OFFSET (2*(BATTERY_SEGMENT_WIDTH + BATTERY_SEGMENT_PAD))

#define VBAT_FULL_2DOT 9
#define VBAT_MID_2DOT 3
#define VBAT_LOW_2DOT 1

#define TOP_BAR_LOCKS 0
#define TOP_BAR_COINS 1
#define TOP_BAR_CAMERAS 2

extern uint8_t ui_x_cursor;
extern uint8_t ui_y_cursor;
extern uint8_t ui_x_max;
extern uint8_t ui_y_max;

void ui_draw_top_bar();
void ui_draw_menu_icons(uint8_t selected_index, const Graphics_Image **icons, const char text[][MAINMENU_NAME_MAX_LEN+1], uint16_t pad, uint16_t x, uint16_t y, uint8_t len);
void ui_draw_element(element_type element, uint8_t bar_level, uint8_t bar_capacity, uint32_t number, uint16_t x, uint16_t y);
void ui_draw_hud(Graphics_Context *gr, uint8_t agent_vertical, uint16_t x, uint16_t y);

#endif /* UI_LAYOUT_H_ */
