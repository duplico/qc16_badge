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

// TODO: Move to global config
#define UI_CLOCK_MS 15 // config
#define UI_CLOCK_TICKS (UI_CLOCK_MS * 100) // derived

// let's start KB events from the top
#define UI_EVENT_KB_FLIP Event_Id_31
#define UI_EVENT_KB_PRESS Event_Id_30
#define UI_EVENT_KB_RELEASE Event_Id_29
// and the rest from the bottom:
#define UI_EVENT_GO_IDLE Event_Id_01
#define UI_EVENT_REFRESH Event_Id_00

#define UI_EVENT_ALL (UI_EVENT_KB_FLIP | UI_EVENT_KB_PRESS | UI_EVENT_REFRESH | UI_EVENT_ALL)

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
#define BTN_F2_COIN (BTN_ROW_2 | BTN_COL_3)
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

extern Clock_Handle kb_debounce_clock_h;
extern uint8_t kb_active_key;
void ui_init();

#endif /* APPLICATION_UI_H_ */
