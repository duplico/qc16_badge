/*
 * ui.h
 *
 *  Created on: Jun 3, 2019
 *      Author: george
 */

#ifndef APPLICATION_UI_H_
#define APPLICATION_UI_H_

#include <stdint.h>

#include <ti/sysbios/knl/Event.h>

#include <ti/grlib/grlib.h>

#define UI_CLOCK_TICKS (UI_CLOCK_MS * 100) // derived

// (below 10 are PORTRAIT)
// UI screens:
#define UI_SCREEN_IDLE 0
#define UI_SCREEN_COLORPICKER 1
#define UI_SCREEN_MAINMENU 10
#define UI_SCREEN_INFO  11
#define UI_SCREEN_MISSIONS 12
#define UI_SCREEN_SCAN 13
#define UI_SCREEN_FILES 14
#define UI_SCREEN_MAINMENU_END 14
#define UI_SCREEN_STORY1 101

#define LOWEST_LANDSCAPE_SCREEN UI_SCREEN_MAINMENU

// Keyboard related:

// let's start KB events from the top
#define UI_EVENT_KB_FLIP Event_Id_31
#define UI_EVENT_KB_PRESS Event_Id_30
#define UI_EVENT_KB_RELEASE Event_Id_29
// and the rest from the bottom:
#define UI_EVENT_BATTERY_UPDATE Event_Id_02
#define UI_EVENT_RADAR_UPDATE Event_Id_01
#define UI_EVENT_REFRESH Event_Id_00

#define UI_EVENT_ALL (UI_EVENT_KB_FLIP | UI_EVENT_KB_PRESS | UI_EVENT_REFRESH | UI_EVENT_BATTERY_UPDATE | UI_EVENT_RADAR_UPDATE | UI_EVENT_REFRESH)

// TODO: move all this shit to keypad.h

#define BTN_STAT_MASK   0300
#define BTN_RC_MASK     0077
#define BTN_ROW_MASK    0070
#define BTN_COL_MASK    0007
#define BTN_PRESSED     0100
#define BTN_ROW_1       0010
#define BTN_ROW_2       0020
#define BTN_ROW_3       0030
#define BTN_ROW_4       0040
#define BTN_COL_1       0001
#define BTN_COL_2       0002
#define BTN_COL_3       0003
#define BTN_COL_4       0004
#define BTN_COL_5       0005

#define kb_active_key_masked (kb_active_key & BTN_RC_MASK)

#define BTN_NONE 0000
#define BTN_UP (BTN_ROW_1 | BTN_COL_1)
#define BTN_DOWN (BTN_ROW_1 | BTN_COL_2)
#define BTN_LEFT (BTN_ROW_1 | BTN_COL_3)
#define BTN_RIGHT (BTN_ROW_1 | BTN_COL_4)
#define BTN_F1_LOCK (BTN_ROW_2 | BTN_COL_1)
#define BTN_F2_COIN (BTN_ROW_2 | BTN_COL_2)
#define BTN_F3_CAMERA (BTN_ROW_2 | BTN_COL_4)
#define BTN_F4_PICKER (BTN_ROW_2 | BTN_COL_5)
#define BTN_RED (BTN_ROW_3 | BTN_COL_1)
#define BTN_ORG (BTN_ROW_3 | BTN_COL_2)
#define BTN_YEL (BTN_ROW_3 | BTN_COL_3)
#define BTN_GRN (BTN_ROW_3 | BTN_COL_4)
#define BTN_BLU (BTN_ROW_3 | BTN_COL_5)
#define BTN_PUR (BTN_ROW_4 | BTN_COL_1)
#define BTN_ROT (BTN_ROW_4 | BTN_COL_2)
#define BTN_BACK (BTN_ROW_4 | BTN_COL_3)
#define BTN_OK (BTN_ROW_4 | BTN_COL_5)

// Visual configurations:
// The screensaver stackup:
// 7 blocks of 18 px high
// 170 px high photo section
#define UI_IDLE_BLOCK_HEIGHT_PX 18
#define UI_IDLE_BLOCK0_HEIGHT 1
#define UI_IDLE_BLOCK1_HEIGHT 4
#define UI_IDLE_BLOCK2_HEIGHT 2

#define UI_IDLE_BLOCK0_TOP_PX       0
#define UI_IDLE_BLOCK0_HEIGHT_PX    (UI_IDLE_BLOCK0_HEIGHT * UI_IDLE_BLOCK_HEIGHT_PX)
#define UI_IDLE_BLOCK1_TOP_PX       UI_IDLE_BLOCK0_HEIGHT_PX
#define UI_IDLE_BLOCK1_HEIGHT_PX    (UI_IDLE_BLOCK1_HEIGHT * UI_IDLE_BLOCK_HEIGHT_PX)
#define UI_IDLE_BLOCK2_TOP_PX       (UI_IDLE_BLOCK0_HEIGHT_PX + UI_IDLE_BLOCK1_HEIGHT_PX)
#define UI_IDLE_BLOCK2_HEIGHT_PX    (UI_IDLE_BLOCK2_HEIGHT * UI_IDLE_BLOCK_HEIGHT_PX)

#define UI_IDLE_PHOTO_TOP           (UI_IDLE_BLOCK2_TOP_PX + UI_IDLE_BLOCK2_HEIGHT_PX)

#define UI_PICKER_TOP               UI_IDLE_PHOTO_TOP

#define UI_PICKER_TAB_H 32
#define UI_PICKER_TAB_W 32

#define UI_PICKER_COLORBOX_H 32
#define UI_PICKER_COLORBOX_BPAD 16

// Data

extern uint8_t ui_current;
extern uint8_t kb_active_key;
extern Event_Handle ui_event_h;
extern Graphics_Context ui_gr_context_landscape;
extern Graphics_Context ui_gr_context_portrait;

// Modals:
extern uint8_t ui_colorpicking;


// Functions

UInt pop_events(UInt *events_ptr, UInt events_to_check);
void ui_init();
uint8_t ui_is_landscape();

#endif /* APPLICATION_UI_H_ */
