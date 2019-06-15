/*
 * ui.c
 *
 *  Created on: Jun 3, 2019
 *      Author: george
 */
#include <qbadge.h>
#include <stdint.h>
#include <stdio.h>

#include <xdc/runtime/Error.h>

#include <ti/grlib/grlib.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/drivers/PIN.h>

#include <queercon/epd_driver.h>
#include "queercon/ht16d35b.h"

#include "ui.h"
#include "board.h"

static Event_Handle ui_event_h;
Graphics_Context ui_gr_context;

Clock_Handle kb_debounce_clock_h;
PIN_Handle kb_pin_h;
PIN_State btn_state;
PIN_Config btn_row_scan[] = {
    QC16_PIN_KP_ROW_1 | PIN_INPUT_EN | PIN_PULLDOWN,
    QC16_PIN_KP_ROW_2 | PIN_INPUT_EN | PIN_PULLDOWN,
    QC16_PIN_KP_ROW_3 | PIN_INPUT_EN | PIN_PULLDOWN,
    QC16_PIN_KP_ROW_4 | PIN_INPUT_EN | PIN_PULLDOWN,
    QC16_PIN_KP_COL_1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH,
    QC16_PIN_KP_COL_2 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH,
    QC16_PIN_KP_COL_3 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH,
    QC16_PIN_KP_COL_4 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH,
    QC16_PIN_KP_COL_5 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH,
    PIN_TERMINATE
};

uint8_t kb_active_key = BTN_NONE;

#define UI_STACKSIZE 2048
Task_Struct ui_task;
uint8_t ui_task_stack[UI_STACKSIZE];

void kb_clock_swi(UArg a0) {
    static uint8_t button_press = BTN_NONE;
    static uint8_t button_press_prev = BTN_NONE;
    static uint8_t kb_mashed;

    button_press_prev = button_press;
    button_press = BTN_NONE;
    kb_mashed = 0;

    // Set all columns high.
    PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_1, 1);
    PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_2, 1);
    PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_3, 1);
    PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_4, 1);
    PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_5, 1);

    // Check to see if that pulled any rows high.
    for (uint8_t r=1; r<5; r++) {
        // this is row r
        // row pin to read is r+QC16_PIN_KP_ROW_1-1
        PIN_Id pin_to_read;
        if (r < 4) {
            pin_to_read = QC16_PIN_KP_ROW_1 + r - 1;
        } else {
            // TODO: This will change for the black prototype & prod.
            pin_to_read = QC16_PIN_KP_ROW_4;
        }

        if (PIN_getInputValue(pin_to_read)) {
            // high => pressed
            if (button_press & 0xF0) {
                // another row already pressed, kb is mashed.
                // give up. a mashed keyboard helps nobody.
                kb_mashed = 1;
                // TODO: return?
                break;
            }
            button_press = r << 4; // row is the upper nibble.

            // Now, figure out which column it is. Set all columns low:
            PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_1, 0);
            PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_2, 0);
            PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_3, 0);
            PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_4, 0);
            PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_5, 0);

            // set each column high, in turn:
            for (uint8_t c=1; c<6; c++) {
                PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_1 + c - 1, 1);
                if (PIN_getInputValue(pin_to_read)) {
                    // If the relevant row reads high, this col,row is pressed.
                    if (button_press & 0x0F) {
                        // if multiple columns are pressed already, then
                        // we have a mashing situation on our hands.
                        kb_mashed = 1;
                    }
                    button_press |= c; // row is already in the upper nibble.
                }
                PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_1 + c - 1, 0);
            }
        }
    }

    // If the high nibble AND low nibble aren't BOTH populated with a value,
    //  then we got a malformed press (row without col, or col without row),
    //  and we should force it to be BTN_NONE.
    if (!((button_press & 0x0f) && (button_press & 0xf0))) {
        button_press = BTN_NONE;
    }
    //  If MULTIPLE, ignore due to mashing.
    if (kb_mashed) {
        // If the keypad is mashed, we don't want to do ANYTHING.
        //  Specifically, we don't want to treat this as a press OR a release.
        //  Instead, we'll just ignore any input that's mashed, and return to
        //  normalcy once the keypad is unmashed.
        // TODO: What happens if we press one key, mash, then release the
        //  original key? We need to make sure that a release event, if we
        //  are producing those, is correctly issued in addition to the
        //  press event.
    } else {
        if (button_press && button_press == button_press_prev) {
            // A button is pressed.
            // TODO: this repeatedly fires
            kb_active_key = button_press;
            Event_post(ui_event_h, UI_EVENT_KB_PRESS);
        } else if (kb_active_key && button_press == button_press_prev) {
            // A button is released.
            // TODO: signal a release
            // No need to clear kb_active_key.
        }
        //  We should have two event signals: KB_PRESS, KB_RELEASE
        //  And then we need to store, somewhere, which key is relevant.
    }
}

////////////////////////////////////

static const unsigned char pixel_cityscapesquare1BPP_UNCOMP[] =
{
0x87, 0xff, 0xf4, 0x2f, 0xd9, 0x91, 0xe3, 0x97, 0xbf, 0xf7, 0xfd, 0x10, 0x49, 0xff, 0xff, 0xf8,
0x0f, 0xff, 0xf5, 0x2f, 0xcc, 0xb1, 0xeb, 0x9a, 0xdf, 0xf7, 0xff, 0x10, 0x79, 0xff, 0xfc, 0x00,
0x3f, 0xff, 0xf1, 0x2f, 0xc6, 0x31, 0xeb, 0x89, 0x5f, 0xf0, 0x7f, 0x10, 0x49, 0xfe, 0x00, 0x00,
0xff, 0xff, 0xf3, 0x2f, 0xd3, 0x31, 0xe3, 0x80, 0x1f, 0xe0, 0x30, 0x00, 0x61, 0xc0, 0x3f, 0xfc,
0xcf, 0xff, 0xf3, 0xaf, 0xc9, 0xb1, 0xe3, 0xeb, 0xff, 0xff, 0xff, 0xff, 0xe9, 0x87, 0xff, 0xfc,
0x8f, 0xff, 0xb3, 0xaf, 0xcc, 0x21, 0xfb, 0x08, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xfc,
0x3f, 0xfe, 0x71, 0xaf, 0xc2, 0x11, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfc,
0x7f, 0xfa, 0x71, 0xaf, 0xd1, 0x11, 0xe1, 0x83, 0xc4, 0x3d, 0x0f, 0x00, 0x40, 0x00, 0x00, 0x00,
0xff, 0xe4, 0xf1, 0x3f, 0xcc, 0x91, 0xf3, 0xa3, 0x40, 0x7d, 0x57, 0x10, 0x79, 0xff, 0xff, 0xe0,
0xff, 0xe3, 0xb1, 0x3f, 0xc6, 0x11, 0xe3, 0x83, 0xc3, 0xfd, 0x03, 0x10, 0x69, 0xff, 0xff, 0xfc,
0xfc, 0xe3, 0xf9, 0x3f, 0xc3, 0x11, 0xe3, 0x82, 0xc7, 0xfd, 0x01, 0x10, 0x79, 0xff, 0xff, 0xfc,
0xf4, 0xe7, 0xf9, 0x3f, 0xd1, 0x11, 0xe9, 0x83, 0x67, 0xed, 0x33, 0x10, 0x69, 0xff, 0xff, 0xfc,
0xdc, 0xff, 0xfb, 0x3f, 0xcc, 0x91, 0xe8, 0x81, 0xe3, 0x7f, 0x11, 0x10, 0x79, 0xff, 0xff, 0xfc,
0x46, 0x9f, 0xff, 0x3f, 0xc6, 0x11, 0xe2, 0x02, 0xc7, 0xff, 0x33, 0x10, 0x49, 0xff, 0xe0, 0x00,
0x5b, 0xf7, 0xff, 0x3f, 0xd3, 0x19, 0xe3, 0x83, 0x63, 0xfd, 0x13, 0x10, 0x79, 0xff, 0xf0, 0x00,
0x0f, 0xf7, 0xff, 0xbf, 0xd9, 0x99, 0xe8, 0x04, 0x07, 0xff, 0x33, 0x10, 0x49, 0xff, 0xff, 0x3c,
0x1b, 0xf4, 0xff, 0xbf, 0xcc, 0x19, 0xe0, 0x00, 0x07, 0xfd, 0x13, 0x10, 0x79, 0xff, 0xff, 0x3c,
0xdd, 0xd7, 0xff, 0xbf, 0xc6, 0x19, 0xe0, 0x00, 0x03, 0xff, 0x31, 0x10, 0x49, 0xff, 0xfe, 0x1c,
0xfc, 0x19, 0x3f, 0x3f, 0xd3, 0x11, 0xe0, 0x00, 0x00, 0x7c, 0x13, 0x10, 0x79, 0xff, 0xf0, 0x00,
0xc9, 0xed, 0x3f, 0x3f, 0xd8, 0x90, 0x00, 0x00, 0x00, 0x0f, 0x31, 0x10, 0x49, 0xff, 0xf7, 0xfc,
0xad, 0xb5, 0xbf, 0x3f, 0xcc, 0x10, 0x00, 0x00, 0x00, 0x0d, 0x53, 0x10, 0x69, 0xff, 0xf7, 0x7c,
0xc2, 0xd5, 0x3f, 0x3f, 0xc3, 0x10, 0x00, 0x00, 0x00, 0x05, 0x33, 0x10, 0x79, 0xff, 0xf6, 0x7c,
0x36, 0xd7, 0xff, 0x3f, 0xd1, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x53, 0x10, 0x69, 0xff, 0xf6, 0x3c,
0x8e, 0xdf, 0xfe, 0x3f, 0x4c, 0x30, 0x00, 0x00, 0x00, 0x0d, 0x33, 0x10, 0x79, 0xff, 0xf4, 0x3c,
0xe6, 0xff, 0x7f, 0x3f, 0xc4, 0x30, 0x00, 0x00, 0x00, 0x0f, 0x50, 0x10, 0x69, 0xff, 0xf6, 0x3c,
0x71, 0xe5, 0x7f, 0x3f, 0xc3, 0x21, 0xe0, 0x00, 0x01, 0xfd, 0x33, 0x10, 0x79, 0xff, 0xf6, 0x3c,
0x3c, 0x9d, 0x3f, 0xbf, 0x59, 0x21, 0x00, 0x00, 0x00, 0x0f, 0x51, 0x10, 0x49, 0xff, 0xf6, 0x3c,
0x8e, 0x07, 0xfc, 0xbf, 0xcc, 0x20, 0x00, 0x00, 0x00, 0x0d, 0x33, 0x10, 0x79, 0xff, 0xf6, 0x3c,
0xe7, 0xb7, 0xfb, 0xbf, 0x46, 0x30, 0x80, 0x00, 0x00, 0x66, 0x73, 0x10, 0x49, 0xff, 0xf6, 0x3c,
0xe1, 0x8c, 0x7f, 0x3f, 0xd3, 0x20, 0x00, 0x00, 0x00, 0x05, 0x33, 0x10, 0x79, 0xff, 0xf6, 0x3c,
0x00, 0x06, 0xfb, 0x3f, 0xd9, 0xa0, 0x00, 0x00, 0x00, 0x0e, 0x73, 0x10, 0x49, 0xff, 0xf6, 0x3c,
0xc1, 0x82, 0x7f, 0x3f, 0x4c, 0x20, 0x00, 0x00, 0x00, 0x0d, 0x31, 0x10, 0x79, 0xff, 0xf6, 0x3c,
0x00, 0x63, 0xfe, 0x3f, 0xc7, 0x31, 0xf0, 0x00, 0x01, 0xbc, 0x73, 0x10, 0x49, 0xff, 0xf6, 0x3c,
0xc0, 0x0f, 0xef, 0x3f, 0x51, 0xa1, 0xc0, 0x1e, 0x00, 0x3d, 0x33, 0x10, 0x79, 0xff, 0xf6, 0x3c,
0x00, 0x0e, 0x0d, 0xbf, 0xd8, 0xa0, 0x00, 0x1f, 0x00, 0x0c, 0x73, 0x10, 0x69, 0xff, 0xf6, 0x3c,
0x00, 0x00, 0x2c, 0xff, 0xc4, 0x20, 0xe0, 0x1f, 0x01, 0x05, 0x13, 0x10, 0x69, 0xff, 0xf7, 0xfc,
0x86, 0x08, 0x2c, 0x7f, 0xc3, 0x30, 0x00, 0x1f, 0x00, 0x0d, 0x30, 0x10, 0x79, 0xff, 0xf3, 0x60,
0xe6, 0x1a, 0x2c, 0x3f, 0xd1, 0xa0, 0x00, 0x1e, 0x00, 0x0f, 0x13, 0x00, 0x69, 0xff, 0xf0, 0x00,
0xf8, 0x82, 0x2d, 0x11, 0xcc, 0xa0, 0x00, 0x00, 0x00, 0x0d, 0x31, 0x10, 0x7b, 0xff, 0xf3, 0x04,
0xf8, 0x12, 0x2d, 0x82, 0x06, 0x21, 0x08, 0x00, 0x02, 0x1f, 0x13, 0x10, 0x60, 0xff, 0xf7, 0xcc,
0xe0, 0x16, 0x2c, 0x03, 0xc3, 0xb0, 0x03, 0x9f, 0xc7, 0xff, 0x23, 0x10, 0x6d, 0x7f, 0xf0, 0x00,
0xe0, 0x06, 0x2c, 0x07, 0xf0, 0xa0, 0xc3, 0x8f, 0xe7, 0xfb, 0x13, 0x10, 0x6c, 0x7f, 0xf8, 0x30,
0xc0, 0x62, 0x2c, 0xc7, 0xfc, 0x20, 0x1b, 0x9d, 0xe5, 0x7f, 0x23, 0x10, 0x03, 0x7f, 0xff, 0xfc,
0x48, 0xf0, 0xac, 0xc6, 0xfe, 0x21, 0x0b, 0x9f, 0xe7, 0xf9, 0x10, 0x10, 0x4f, 0xbf, 0xff, 0xfc,
0x09, 0xf0, 0x2c, 0xc5, 0x3f, 0x30, 0x83, 0x8f, 0xe7, 0xff, 0x33, 0x10, 0x7d, 0xbf, 0xff, 0xfc,
0xcf, 0x38, 0xad, 0x87, 0xe7, 0xd0, 0x0b, 0x8d, 0xe7, 0xf9, 0x11, 0x10, 0x67, 0x3f, 0xff, 0xfc,
0xcd, 0x9c, 0x2d, 0xa0, 0x00, 0x60, 0x5b, 0x9f, 0xc7, 0xfd, 0x33, 0x10, 0x79, 0xbf, 0xff, 0xfc,
0xcf, 0x9e, 0x0c, 0x80, 0x00, 0x30, 0x4b, 0x8f, 0xe7, 0x69, 0x13, 0x00, 0x3f, 0x1f, 0xff, 0xfc,
0xdf, 0xf0, 0x6c, 0x80, 0x97, 0x80, 0x53, 0x8f, 0xe7, 0xfd, 0x32, 0x00, 0x3b, 0xc3, 0xff, 0xfc,
0x9f, 0xf0, 0x7c, 0x80, 0x13, 0xa0, 0x1f, 0x9f, 0xe7, 0xfd, 0x13, 0x00, 0x1b, 0x47, 0xff, 0xfc,
0x8f, 0xfc, 0x7c, 0x80, 0x01, 0xb0, 0x1f, 0x8f, 0xe7, 0x7d, 0x31, 0x00, 0x3b, 0x53, 0xff, 0xfc,
0x83, 0xfe, 0x7d, 0x80, 0x41, 0xb8, 0xc1, 0x8d, 0xe7, 0xfd, 0x13, 0x00, 0x1b, 0x57, 0xff, 0xfc,
0x83, 0xfe, 0x7c, 0x01, 0xf1, 0xbc, 0xda, 0x9f, 0xe7, 0x7d, 0x31, 0x00, 0x39, 0x53, 0xff, 0xfc,
0x8f, 0xee, 0x7c, 0x01, 0x72, 0x9e, 0x77, 0x9f, 0xe7, 0xff, 0x53, 0x00, 0x3f, 0x50, 0x1f, 0xfc,
0x8f, 0xfe, 0x7c, 0x00, 0x72, 0x9f, 0x13, 0x8d, 0xe7, 0xfd, 0x33, 0x00, 0x20, 0x07, 0xdf, 0xfc,
0x86, 0xce, 0x7e, 0x00, 0xe2, 0x87, 0x8b, 0x97, 0xe5, 0x2f, 0x52, 0x00, 0x00, 0x01, 0xdf, 0xfc,
0x07, 0xe6, 0x7c, 0x00, 0x06, 0x82, 0xe7, 0x9f, 0xe5, 0xfd, 0x33, 0x00, 0x02, 0x09, 0xdf, 0xfc,
0x07, 0x7e, 0x7c, 0x24, 0x0a, 0x80, 0xe1, 0x8f, 0xe7, 0xfe, 0x61, 0x00, 0x61, 0x08, 0xdb, 0xfc,
0x07, 0x3e, 0x7f, 0x00, 0x1c, 0x80, 0xc8, 0x97, 0xe7, 0xfd, 0x23, 0x00, 0x00, 0x00, 0xd8, 0xfc,
0x07, 0xae, 0x7f, 0xc0, 0x1e, 0x80, 0x1b, 0x9f, 0xe7, 0xfc, 0x63, 0x00, 0x6b, 0x10, 0xca, 0x7c,
0x8f, 0x1a, 0x7f, 0xe0, 0x3e, 0x60, 0x1f, 0x8f, 0xe7, 0x3d, 0x23, 0x00, 0x00, 0x00, 0x4a, 0x7c,
0xfe, 0xc6, 0x7f, 0xe0, 0x79, 0x70, 0x1f, 0x9f, 0xe5, 0xfc, 0x63, 0x00, 0x6b, 0x91, 0x0a, 0xfc,
0x7f, 0x7a, 0x7f, 0xe0, 0x71, 0x38, 0x09, 0x80, 0x27, 0xfc, 0x22, 0x00, 0x00, 0x00, 0x8a, 0xfc,
0x07, 0xc8, 0x07, 0xe0, 0x79, 0x9f, 0x13, 0xb1, 0x27, 0xbc, 0x63, 0x00, 0x6f, 0xd0, 0x0a, 0xfc,
0x04, 0xf9, 0x27, 0xe0, 0xff, 0xa7, 0x18, 0xb1, 0xa5, 0xff, 0x21, 0x00, 0x00, 0x01, 0x8a, 0x7c,
0xe0, 0xd9, 0x67, 0xe0, 0xff, 0xa3, 0xdd, 0xb1, 0xa4, 0x00, 0x03, 0x10, 0x00, 0x01, 0xa0, 0x0c,
0x54, 0x39, 0x67, 0xe0, 0xf5, 0xa0, 0xdd, 0xa1, 0xa4, 0x39, 0x83, 0x00, 0x00, 0x03, 0xa3, 0xec,
0xfc, 0x09, 0x61, 0xe0, 0x00, 0x00, 0x3d, 0x8e, 0x24, 0x37, 0x82, 0x00, 0x0f, 0x90, 0xe3, 0xec,
0xff, 0x81, 0xee, 0x60, 0x04, 0x00, 0x1c, 0x07, 0x24, 0xff, 0x83, 0x00, 0x40, 0x00, 0xc3, 0xec,
0xff, 0xe9, 0x2f, 0x80, 0x04, 0x20, 0x1c, 0x1f, 0x26, 0xdf, 0xe3, 0x90, 0x0f, 0x90, 0x5b, 0xec,
0xff, 0xf9, 0x23, 0xe0, 0x04, 0x60, 0x1c, 0x1e, 0x24, 0xf6, 0x80, 0x00, 0x00, 0x00, 0x5d, 0xec,
0xff, 0xf9, 0x20, 0xe0, 0x04, 0x38, 0x1c, 0x24, 0xa6, 0x88, 0xc0, 0x00, 0x0a, 0x00, 0x48, 0xec,
0xff, 0xf9, 0xe0, 0x20, 0x00, 0x0e, 0x1c, 0x01, 0x26, 0x0c, 0x61, 0xe4, 0x20, 0x00, 0x5e, 0xec,
0xff, 0xf9, 0x00, 0x87, 0x80, 0x78, 0x1c, 0x00, 0x06, 0x00, 0x01, 0x20, 0x00, 0x00, 0x5f, 0xec,
0xff, 0xf9, 0x00, 0x89, 0x9f, 0xff, 0x1c, 0x00, 0x20, 0x81, 0x00, 0x00, 0x29, 0x10, 0x5f, 0x6c,
0xff, 0xf9, 0x20, 0x88, 0x1f, 0xf6, 0xcc, 0x7c, 0x60, 0x00, 0x25, 0xfd, 0x02, 0x00, 0x5f, 0xec,
0xff, 0xf9, 0xe0, 0x88, 0x9f, 0xff, 0x9c, 0x3e, 0xe3, 0x7f, 0xa4, 0xbc, 0x00, 0x00, 0x03, 0xec,
0xff, 0xf9, 0x20, 0x88, 0xdf, 0xff, 0xcc, 0x84, 0x03, 0x56, 0xa7, 0xec, 0x88, 0x00, 0x1f, 0xec,
0x1f, 0xf9, 0x27, 0x08, 0xc0, 0x1f, 0xe1, 0xf4, 0x00, 0x14, 0xe7, 0xec, 0x00, 0x00, 0x1f, 0xac,
0x20, 0xf9, 0x20, 0x48, 0xcf, 0x40, 0xd8, 0x36, 0xc4, 0x7f, 0xa7, 0x9e, 0x00, 0x40, 0x03, 0xac,
0x28, 0x19, 0xe0, 0x10, 0xc0, 0x00, 0x0f, 0x06, 0xe0, 0x14, 0xe7, 0x9f, 0x79, 0xe0, 0x59, 0xac,
0xa9, 0x48, 0x00, 0x00, 0x99, 0xb1, 0x00, 0x56, 0x48, 0xff, 0xa7, 0xdc, 0xf8, 0x80, 0x01, 0x2c,
0xa9, 0x48, 0x00, 0x00, 0x0e, 0x33, 0x40, 0x00, 0x08, 0x96, 0xe5, 0xfc, 0x30, 0xc0, 0x00, 0x2c,
0x29, 0x08, 0x01, 0x80, 0x9c, 0xf3, 0x40, 0x02, 0x68, 0xde, 0xe6, 0x7e, 0x47, 0x00, 0x40, 0x2c,
0x29, 0x49, 0xe1, 0x98, 0x10, 0x03, 0x42, 0x1b, 0x28, 0xf7, 0xa7, 0xfd, 0x8e, 0x00, 0x01, 0x20,
0x09, 0x49, 0x21, 0x19, 0x97, 0x73, 0x42, 0x08, 0x00, 0x94, 0xe4, 0xfd, 0x18, 0xc3, 0xf8, 0x24,
0x00, 0x01, 0x21, 0x19, 0x97, 0x73, 0x40, 0x0d, 0x80, 0xff, 0xa4, 0xd8, 0x00, 0x00, 0x08, 0x24,
0x78, 0x09, 0x21, 0x19, 0x96, 0x63, 0x40, 0x04, 0x04, 0x94, 0xe6, 0x10, 0x00, 0x03, 0xf8, 0x24,
0x27, 0x41, 0x01, 0x19, 0x92, 0x73, 0x40, 0x0b, 0x06, 0xff, 0xa0, 0x00, 0x00, 0x00, 0x48, 0x24,
0x8d, 0x78, 0x01, 0x19, 0x9e, 0x33, 0x40, 0x08, 0x00, 0x94, 0xc7, 0xbe, 0x1e, 0x02, 0x48, 0x24,
0x99, 0xc8, 0x00, 0x11, 0x8e, 0x42, 0x40, 0x08, 0x08, 0xfe, 0x80, 0xf0, 0x01, 0xc3, 0xf9, 0x24,
0x10, 0xc0, 0x00, 0x00, 0x0e, 0x06, 0x00, 0x08, 0x80, 0x03, 0x8f, 0xc0, 0x01, 0xc0, 0x00, 0x24,
0x10, 0x80, 0x00, 0x00, 0x02, 0x17, 0x80, 0x08, 0x84, 0x01, 0x0c, 0x40, 0x0f, 0x40, 0x00, 0x24,
0x10, 0x80, 0x22, 0x00, 0x1e, 0x17, 0x90, 0x08, 0x05, 0xf0, 0x09, 0xff, 0xff, 0x00, 0x1c, 0x24,
0x10, 0x81, 0x00, 0x24, 0x1e, 0x17, 0xa0, 0x08, 0x00, 0x06, 0xee, 0x1f, 0xf1, 0x40, 0x0c, 0x24,
0x10, 0x81, 0x08, 0x00, 0x08, 0x17, 0xa6, 0x08, 0x00, 0x1c, 0x0a, 0x1f, 0xf0, 0xc3, 0xe6, 0x24,
0x10, 0x80, 0x18, 0x12, 0x0a, 0x37, 0x80, 0x01, 0x03, 0x3c, 0x08, 0x0f, 0x60, 0xc0, 0x00, 0x24,
0x90, 0x80, 0x19, 0x12, 0x0f, 0x76, 0xc0, 0x00, 0x01, 0x7f, 0xce, 0x10, 0xf1, 0xdf, 0xf8, 0x24,
0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf8, 0x24,
0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x08, 0x00, 0x00, 0x02, 0x00, 0x24,
0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x0e, 0x34, 0x1a, 0x04, 0x01, 0x82, 0x00, 0x04,
0x00, 0x08, 0x00, 0x00, 0x10, 0x01, 0xc0, 0x02, 0x16, 0x06, 0x0b, 0x54, 0x01, 0x8a, 0x50, 0x04,
0x4e, 0x3c, 0xc0, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x3c, 0x14, 0x08, 0xd0, 0x00, 0x02, 0x00, 0x08,
0xc0, 0x1c, 0xc3, 0x01, 0x88, 0x00, 0x01, 0x00, 0x00, 0x10, 0x00, 0x50, 0x00, 0x02, 0x00, 0x08,
0x82, 0x08, 0xb1, 0x01, 0x10, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
0x80, 0x10, 0x00, 0x00, 0x30, 0x00, 0xa0, 0x07, 0xfc, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x30, 0x20,
0x00, 0x00, 0x07, 0x88, 0x10, 0x00, 0x00, 0x07, 0xff, 0x7f, 0xf1, 0xe3, 0xe1, 0xf9, 0xe7, 0xe0,
0x00, 0x00, 0x33, 0xde, 0x00, 0x00, 0x00, 0x0c, 0x05, 0xff, 0xf7, 0x83, 0x8f, 0xfc, 0x43, 0xc4,
0x43, 0xce, 0x01, 0x97, 0x0f, 0xf0, 0x02, 0x4e, 0x7d, 0x30, 0x67, 0x80, 0x0f, 0x78, 0x00, 0xc4,
0xed, 0xff, 0x43, 0x93, 0x1f, 0xff, 0xfe, 0x1e, 0x02, 0x04, 0x47, 0x80, 0x08, 0x00, 0x00, 0x8c,
0xf0, 0xc5, 0x20, 0x37, 0x1f, 0xff, 0x87, 0x76, 0x03, 0x0f, 0xd7, 0x83, 0x00, 0x00, 0x81, 0x8c,
0xc0, 0x00, 0xe0, 0xe0, 0x1f, 0xfe, 0x07, 0xff, 0x03, 0x0f, 0x80, 0x00, 0x00, 0x00, 0x01, 0x1c,
0xc0, 0x24, 0x20, 0x00, 0x4f, 0xfc, 0x3d, 0xe0, 0x04, 0x00, 0x10, 0x00, 0x5f, 0xff, 0x83, 0x18,
0xf0, 0x18, 0x00, 0x00, 0x8e, 0x70, 0x38, 0xf8, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x10,
0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0xf8, 0xff, 0xff, 0x03, 0xff, 0x00, 0x00, 0x03, 0x1c,
0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x78, 0xf8, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x03, 0xfc,
0x00, 0x00, 0x00, 0x7f, 0xe0, 0x00, 0xf0, 0xe0, 0xff, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xf7, 0xfc,
0x00, 0x00, 0x7f, 0xfe, 0x00, 0x01, 0xf0, 0x80, 0x03, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xe7, 0xfc,
0x00, 0x37, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xf3, 0xff, 0xff, 0xcf, 0xfc,
0x07, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xc7, 0xff, 0xff, 0xcf, 0xfc,
0xff, 0xff, 0xfe, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xcf, 0xfc,
0xe0, 0xc1, 0x2e, 0x3f, 0xe0, 0x00, 0x00, 0x00, 0x7f, 0x3f, 0xf3, 0x3f, 0xbf, 0xfb, 0x5f, 0xfc,
0x3f, 0x03, 0xff, 0xff, 0xef, 0xfe, 0xff, 0xf7, 0xff, 0xff, 0xf7, 0xff, 0x7f, 0xfe, 0xdf, 0xf8,
0x00, 0x7f, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xdf, 0xfe, 0xff, 0xfe, 0xdc, 0x00,
0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xff, 0x7f, 0xfd, 0xff, 0xff, 0xc8, 0x00
};

static const uint32_t palette_cityscapesquare1BPP_UNCOMP[]=
{
    0x000000,   0xffffff
};

const Graphics_Image cityscapesquare1BPP_UNCOMP=
{
    IMAGE_FMT_1BPP_UNCOMP,
    126,
    126,
    2,
    palette_cityscapesquare1BPP_UNCOMP,
    pixel_cityscapesquare1BPP_UNCOMP,
};

static const unsigned char pixel_cityscape1BPP_UNCOMP[] =
{
0xff, 0xff, 0xbf, 0x1f, 0x42, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0x1f, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0x1f, 0xc4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0x1f, 0xc2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0x1f, 0xc8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0x1f, 0xc4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xfd, 0x1f, 0xc2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xf9, 0x3f, 0xc2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xf8, 0x9f, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0x3f, 0xf8, 0x9f, 0xc4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xfc, 0x3f, 0xfa, 0xbf, 0xd2, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xf0, 0x1f, 0xf7, 0x3f, 0xc8, 0xf8, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xe0, 0x19, 0xfc, 0x3f, 0xe0, 0xc1, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0x80, 0x19, 0xfb, 0x3e, 0xc2, 0x07, 0x9f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0x19, 0x1f, 0x37, 0x3e, 0xc2, 0xd9, 0xdf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0x3f, 0x3e, 0x34, 0x3e, 0xe0, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0x7f, 0x3e, 0x30, 0x3e, 0xc4, 0x9f, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0x7c, 0x3c, 0x71, 0x3e, 0x92, 0xf7, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xf0, 0x38, 0x7f, 0xbe, 0xca, 0x59, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xe0, 0x32, 0xff, 0x9e, 0xc0, 0x1d, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0x80, 0xb3, 0xfc, 0xbe, 0xc0, 0x9f, 0x37, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0x00, 0xe1, 0xf3, 0xbe, 0xc2, 0xe7, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0x21, 0x80, 0xf3, 0x1e, 0xc0, 0x5d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0x23, 0x03, 0xf9, 0x0e, 0xc4, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0x3f, 0x19, 0xff, 0x2e, 0xd2, 0xdf, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xf1, 0xbf, 0xff, 0xff, 0xff,
0x3f, 0x05, 0xf9, 0x33, 0xc2, 0xf5, 0xbf, 0xff, 0xff, 0xff, 0xff, 0x8f, 0xbf, 0xff, 0xff, 0xff,
0x3f, 0x29, 0xf1, 0x3b, 0xc0, 0x7d, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xbf, 0xff, 0xff, 0xff,
0x3f, 0xab, 0xff, 0x3c, 0xc4, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbf, 0xff, 0xff, 0xff,
0x2f, 0x13, 0xfd, 0x3e, 0x42, 0xdf, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbf, 0xff, 0xff, 0xff,
0x1e, 0x13, 0xfc, 0x9a, 0x48, 0x75, 0xbf, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xbf, 0xff, 0xff, 0xff,
0x3c, 0x33, 0x74, 0xbb, 0x84, 0x19, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbf, 0xff, 0xff, 0xff,
0x38, 0x7a, 0x71, 0xba, 0xc2, 0x9d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbf, 0xff, 0xff, 0xff,
0x70, 0x76, 0xf1, 0x3a, 0xd2, 0x9f, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbf, 0xff, 0xff, 0xff,
0x00, 0xe6, 0xf5, 0x3b, 0xc8, 0xb5, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0x9f, 0xff, 0xff, 0xff,
0x00, 0xbf, 0xf7, 0x3b, 0xcc, 0x99, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0x9f, 0xff, 0xff, 0xff,
0x11, 0xff, 0xff, 0x3b, 0xc6, 0x9d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x7f, 0xff, 0xff, 0xff,
0x33, 0xfb, 0xff, 0x3b, 0xc2, 0x9f, 0x3f, 0xff, 0xff, 0xff, 0xff, 0x00, 0xdf, 0xff, 0xff, 0xff,
0x73, 0xf3, 0xf9, 0x2b, 0xc3, 0x35, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xa0, 0xcf, 0xff, 0xff, 0xff,
0xf3, 0xc3, 0x77, 0x2b, 0xc9, 0x19, 0xb3, 0xff, 0xff, 0xff, 0xff, 0xb0, 0xff, 0xff, 0xff, 0xff,
0xfa, 0x86, 0x73, 0xab, 0xc4, 0x9d, 0x9f, 0xff, 0xff, 0xff, 0xfe, 0xf0, 0x4f, 0xff, 0xff, 0xff,
0xfe, 0x1e, 0xfb, 0xab, 0xc2, 0x1e, 0x3f, 0xff, 0xff, 0xff, 0xff, 0x90, 0x47, 0xff, 0xff, 0xff,
0xfc, 0x7f, 0xf3, 0xab, 0xc9, 0x34, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x90, 0xc7, 0xff, 0xff, 0xff,
0xe0, 0xff, 0xf7, 0xab, 0xcc, 0x19, 0xf7, 0xff, 0xff, 0xff, 0xfe, 0xf0, 0x77, 0xff, 0xff, 0xff,
0x83, 0xbf, 0xff, 0x2b, 0xc6, 0x19, 0xf7, 0xff, 0xff, 0xff, 0xfe, 0xc0, 0x4b, 0xff, 0xff, 0xff,
0x0f, 0x3f, 0xff, 0x2b, 0xd2, 0x1f, 0xe3, 0xbf, 0xff, 0xff, 0xfe, 0x10, 0x7b, 0xff, 0xff, 0xff,
0xb9, 0x3f, 0xfd, 0x2f, 0xc9, 0x37, 0xe1, 0x9f, 0xff, 0xff, 0xf9, 0x10, 0x4b, 0xff, 0xff, 0xff,
0xf0, 0x7f, 0xf9, 0x2f, 0xcc, 0x91, 0xe3, 0x87, 0xff, 0xff, 0xff, 0x10, 0x7b, 0xff, 0xff, 0xff,
0xc1, 0xff, 0xf8, 0x2f, 0xc2, 0x11, 0xeb, 0x93, 0xff, 0xff, 0xfd, 0x10, 0x4b, 0xff, 0xff, 0xff,
0x0f, 0xff, 0xf8, 0x2f, 0xdb, 0x11, 0xeb, 0x9f, 0xff, 0xff, 0xff, 0x10, 0x6b, 0xff, 0xff, 0xff,
0x1f, 0xff, 0xfa, 0x2f, 0xc9, 0x31, 0xe3, 0x8f, 0xff, 0xff, 0xfd, 0x10, 0x69, 0xff, 0xff, 0xff,
0x7f, 0x3f, 0xff, 0xaf, 0xc4, 0x91, 0xeb, 0x97, 0xff, 0xff, 0xff, 0x10, 0x69, 0xff, 0xff, 0xff,
0xe6, 0xbf, 0xff, 0xaf, 0xc2, 0x11, 0xfb, 0x9f, 0xff, 0xff, 0xfd, 0x10, 0x79, 0xff, 0xff, 0xff,
0x06, 0x7f, 0xfb, 0xaf, 0xd9, 0x11, 0xe3, 0x8f, 0x7f, 0xff, 0xff, 0x10, 0x69, 0xff, 0xff, 0xff,
0x0f, 0xff, 0xf9, 0x2f, 0xcc, 0xb1, 0xe3, 0x97, 0x7f, 0xf7, 0xff, 0x10, 0x79, 0xff, 0xff, 0xff,
0x7f, 0xff, 0xfb, 0x2f, 0xc6, 0x31, 0xfb, 0x9e, 0xbf, 0xf7, 0xfd, 0x10, 0x49, 0xff, 0xff, 0xff,
0xf3, 0xff, 0xf4, 0x2f, 0xd3, 0x11, 0xe3, 0x8f, 0xff, 0xf7, 0xff, 0x10, 0x79, 0xff, 0xff, 0xff,
0x87, 0xff, 0xf4, 0x2f, 0xd9, 0x91, 0xe3, 0x97, 0xbf, 0xf7, 0xfd, 0x10, 0x49, 0xff, 0xff, 0xf8,
0x0f, 0xff, 0xf5, 0x2f, 0xcc, 0xb1, 0xeb, 0x9a, 0xdf, 0xf7, 0xff, 0x10, 0x79, 0xff, 0xfc, 0x00,
0x3f, 0xff, 0xf1, 0x2f, 0xc6, 0x31, 0xeb, 0x89, 0x5f, 0xf0, 0x7f, 0x10, 0x49, 0xfe, 0x00, 0x03,
0xff, 0xff, 0xf3, 0x2f, 0xd3, 0x31, 0xe3, 0x80, 0x1f, 0xe0, 0x30, 0x00, 0x61, 0xc0, 0x3f, 0xff,
0xcf, 0xff, 0xf3, 0xaf, 0xc9, 0xb1, 0xe3, 0xeb, 0xff, 0xff, 0xff, 0xff, 0xe9, 0x87, 0xff, 0xff,
0x8f, 0xff, 0xb3, 0xaf, 0xcc, 0x21, 0xfb, 0x08, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff,
0x3f, 0xfe, 0x71, 0xaf, 0xc2, 0x11, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff,
0x7f, 0xfa, 0x71, 0xaf, 0xd1, 0x11, 0xe1, 0x83, 0xc4, 0x3d, 0x0f, 0x00, 0x40, 0x00, 0x00, 0x00,
0xff, 0xe4, 0xf1, 0x3f, 0xcc, 0x91, 0xf3, 0xa3, 0x40, 0x7d, 0x57, 0x10, 0x79, 0xff, 0xff, 0xe0,
0xff, 0xe3, 0xb1, 0x3f, 0xc6, 0x11, 0xe3, 0x83, 0xc3, 0xfd, 0x03, 0x10, 0x69, 0xff, 0xff, 0xff,
0xfc, 0xe3, 0xf9, 0x3f, 0xc3, 0x11, 0xe3, 0x82, 0xc7, 0xfd, 0x01, 0x10, 0x79, 0xff, 0xff, 0xff,
0xf4, 0xe7, 0xf9, 0x3f, 0xd1, 0x11, 0xe9, 0x83, 0x67, 0xed, 0x33, 0x10, 0x69, 0xff, 0xff, 0xff,
0xdc, 0xff, 0xfb, 0x3f, 0xcc, 0x91, 0xe8, 0x81, 0xe3, 0x7f, 0x11, 0x10, 0x79, 0xff, 0xff, 0xff,
0x46, 0x9f, 0xff, 0x3f, 0xc6, 0x11, 0xe2, 0x02, 0xc7, 0xff, 0x33, 0x10, 0x49, 0xff, 0xe0, 0x00,
0x5b, 0xf7, 0xff, 0x3f, 0xd3, 0x19, 0xe3, 0x83, 0x63, 0xfd, 0x13, 0x10, 0x79, 0xff, 0xf0, 0x00,
0x0f, 0xf7, 0xff, 0xbf, 0xd9, 0x99, 0xe8, 0x04, 0x07, 0xff, 0x33, 0x10, 0x49, 0xff, 0xff, 0x3f,
0x1b, 0xf4, 0xff, 0xbf, 0xcc, 0x19, 0xe0, 0x00, 0x07, 0xfd, 0x13, 0x10, 0x79, 0xff, 0xff, 0x3f,
0xdd, 0xd7, 0xff, 0xbf, 0xc6, 0x19, 0xe0, 0x00, 0x03, 0xff, 0x31, 0x10, 0x49, 0xff, 0xfe, 0x1f,
0xfc, 0x19, 0x3f, 0x3f, 0xd3, 0x11, 0xe0, 0x00, 0x00, 0x7c, 0x13, 0x10, 0x79, 0xff, 0xf0, 0x00,
0xc9, 0xed, 0x3f, 0x3f, 0xd8, 0x90, 0x00, 0x00, 0x00, 0x0f, 0x31, 0x10, 0x49, 0xff, 0xf7, 0xff,
0xad, 0xb5, 0xbf, 0x3f, 0xcc, 0x10, 0x00, 0x00, 0x00, 0x0d, 0x53, 0x10, 0x69, 0xff, 0xf7, 0x7f,
0xc2, 0xd5, 0x3f, 0x3f, 0xc3, 0x10, 0x00, 0x00, 0x00, 0x05, 0x33, 0x10, 0x79, 0xff, 0xf6, 0x7f,
0x36, 0xd7, 0xff, 0x3f, 0xd1, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x53, 0x10, 0x69, 0xff, 0xf6, 0x3f,
0x8e, 0xdf, 0xfe, 0x3f, 0x4c, 0x30, 0x00, 0x00, 0x00, 0x0d, 0x33, 0x10, 0x79, 0xff, 0xf4, 0x3f,
0xe6, 0xff, 0x7f, 0x3f, 0xc4, 0x30, 0x00, 0x00, 0x00, 0x0f, 0x50, 0x10, 0x69, 0xff, 0xf6, 0x3f,
0x71, 0xe5, 0x7f, 0x3f, 0xc3, 0x21, 0xe0, 0x00, 0x01, 0xfd, 0x33, 0x10, 0x79, 0xff, 0xf6, 0x3f,
0x3c, 0x9d, 0x3f, 0xbf, 0x59, 0x21, 0x00, 0x00, 0x00, 0x0f, 0x51, 0x10, 0x49, 0xff, 0xf6, 0x3f,
0x8e, 0x07, 0xfc, 0xbf, 0xcc, 0x20, 0x00, 0x00, 0x00, 0x0d, 0x33, 0x10, 0x79, 0xff, 0xf6, 0x3f,
0xe7, 0xb7, 0xfb, 0xbf, 0x46, 0x30, 0x80, 0x00, 0x00, 0x66, 0x73, 0x10, 0x49, 0xff, 0xf6, 0x3f,
0xe1, 0x8c, 0x7f, 0x3f, 0xd3, 0x20, 0x00, 0x00, 0x00, 0x05, 0x33, 0x10, 0x79, 0xff, 0xf6, 0x3f,
0x00, 0x06, 0xfb, 0x3f, 0xd9, 0xa0, 0x00, 0x00, 0x00, 0x0e, 0x73, 0x10, 0x49, 0xff, 0xf6, 0x3f,
0xc1, 0x82, 0x7f, 0x3f, 0x4c, 0x20, 0x00, 0x00, 0x00, 0x0d, 0x31, 0x10, 0x79, 0xff, 0xf6, 0x3f,
0x00, 0x63, 0xfe, 0x3f, 0xc7, 0x31, 0xf0, 0x00, 0x01, 0xbc, 0x73, 0x10, 0x49, 0xff, 0xf6, 0x3f,
0xc0, 0x0f, 0xef, 0x3f, 0x51, 0xa1, 0xc0, 0x1e, 0x00, 0x3d, 0x33, 0x10, 0x79, 0xff, 0xf6, 0x3f,
0x00, 0x0e, 0x0d, 0xbf, 0xd8, 0xa0, 0x00, 0x1f, 0x00, 0x0c, 0x73, 0x10, 0x69, 0xff, 0xf6, 0x3f,
0x00, 0x00, 0x2c, 0xff, 0xc4, 0x20, 0xe0, 0x1f, 0x01, 0x05, 0x13, 0x10, 0x69, 0xff, 0xf7, 0xff,
0x86, 0x08, 0x2c, 0x7f, 0xc3, 0x30, 0x00, 0x1f, 0x00, 0x0d, 0x30, 0x10, 0x79, 0xff, 0xf3, 0x63,
0xe6, 0x1a, 0x2c, 0x3f, 0xd1, 0xa0, 0x00, 0x1e, 0x00, 0x0f, 0x13, 0x00, 0x69, 0xff, 0xf0, 0x00,
0xf8, 0x82, 0x2d, 0x11, 0xcc, 0xa0, 0x00, 0x00, 0x00, 0x0d, 0x31, 0x10, 0x7b, 0xff, 0xf3, 0x04,
0xf8, 0x12, 0x2d, 0x82, 0x06, 0x21, 0x08, 0x00, 0x02, 0x1f, 0x13, 0x10, 0x60, 0xff, 0xf7, 0xcf,
0xe0, 0x16, 0x2c, 0x03, 0xc3, 0xb0, 0x03, 0x9f, 0xc7, 0xff, 0x23, 0x10, 0x6d, 0x7f, 0xf0, 0x00,
0xe0, 0x06, 0x2c, 0x07, 0xf0, 0xa0, 0xc3, 0x8f, 0xe7, 0xfb, 0x13, 0x10, 0x6c, 0x7f, 0xf8, 0x30,
0xc0, 0x62, 0x2c, 0xc7, 0xfc, 0x20, 0x1b, 0x9d, 0xe5, 0x7f, 0x23, 0x10, 0x03, 0x7f, 0xff, 0xff,
0x48, 0xf0, 0xac, 0xc6, 0xfe, 0x21, 0x0b, 0x9f, 0xe7, 0xf9, 0x10, 0x10, 0x4f, 0xbf, 0xff, 0xff,
0x09, 0xf0, 0x2c, 0xc5, 0x3f, 0x30, 0x83, 0x8f, 0xe7, 0xff, 0x33, 0x10, 0x7d, 0xbf, 0xff, 0xff,
0xcf, 0x38, 0xad, 0x87, 0xe7, 0xd0, 0x0b, 0x8d, 0xe7, 0xf9, 0x11, 0x10, 0x67, 0x3f, 0xff, 0xff,
0xcd, 0x9c, 0x2d, 0xa0, 0x00, 0x60, 0x5b, 0x9f, 0xc7, 0xfd, 0x33, 0x10, 0x79, 0xbf, 0xff, 0xfe,
0xcf, 0x9e, 0x0c, 0x80, 0x00, 0x30, 0x4b, 0x8f, 0xe7, 0x69, 0x13, 0x00, 0x3f, 0x1f, 0xff, 0xfe,
0xdf, 0xf0, 0x6c, 0x80, 0x97, 0x80, 0x53, 0x8f, 0xe7, 0xfd, 0x32, 0x00, 0x3b, 0xc3, 0xff, 0xff,
0x9f, 0xf0, 0x7c, 0x80, 0x13, 0xa0, 0x1f, 0x9f, 0xe7, 0xfd, 0x13, 0x00, 0x1b, 0x47, 0xff, 0xff,
0x8f, 0xfc, 0x7c, 0x80, 0x01, 0xb0, 0x1f, 0x8f, 0xe7, 0x7d, 0x31, 0x00, 0x3b, 0x53, 0xff, 0xff,
0x83, 0xfe, 0x7d, 0x80, 0x41, 0xb8, 0xc1, 0x8d, 0xe7, 0xfd, 0x13, 0x00, 0x1b, 0x57, 0xff, 0xfe,
0x83, 0xfe, 0x7c, 0x01, 0xf1, 0xbc, 0xda, 0x9f, 0xe7, 0x7d, 0x31, 0x00, 0x39, 0x53, 0xff, 0xfe,
0x8f, 0xee, 0x7c, 0x01, 0x72, 0x9e, 0x77, 0x9f, 0xe7, 0xff, 0x53, 0x00, 0x3f, 0x50, 0x1f, 0xfe,
0x8f, 0xfe, 0x7c, 0x00, 0x72, 0x9f, 0x13, 0x8d, 0xe7, 0xfd, 0x33, 0x00, 0x20, 0x07, 0xdf, 0xff,
0x86, 0xce, 0x7e, 0x00, 0xe2, 0x87, 0x8b, 0x97, 0xe5, 0x2f, 0x52, 0x00, 0x00, 0x01, 0xdf, 0xff,
0x07, 0xe6, 0x7c, 0x00, 0x06, 0x82, 0xe7, 0x9f, 0xe5, 0xfd, 0x33, 0x00, 0x02, 0x09, 0xdf, 0xff,
0x07, 0x7e, 0x7c, 0x24, 0x0a, 0x80, 0xe1, 0x8f, 0xe7, 0xfe, 0x61, 0x00, 0x61, 0x08, 0xdb, 0xff,
0x07, 0x3e, 0x7f, 0x00, 0x1c, 0x80, 0xc8, 0x97, 0xe7, 0xfd, 0x23, 0x00, 0x00, 0x00, 0xd8, 0xff,
0x07, 0xae, 0x7f, 0xc0, 0x1e, 0x80, 0x1b, 0x9f, 0xe7, 0xfc, 0x63, 0x00, 0x6b, 0x10, 0xca, 0x7f,
0x8f, 0x1a, 0x7f, 0xe0, 0x3e, 0x60, 0x1f, 0x8f, 0xe7, 0x3d, 0x23, 0x00, 0x00, 0x00, 0x4a, 0x7f,
0xfe, 0xc6, 0x7f, 0xe0, 0x79, 0x70, 0x1f, 0x9f, 0xe5, 0xfc, 0x63, 0x00, 0x6b, 0x91, 0x0a, 0xff,
0x7f, 0x7a, 0x7f, 0xe0, 0x71, 0x38, 0x09, 0x80, 0x27, 0xfc, 0x22, 0x00, 0x00, 0x00, 0x8a, 0xff,
0x07, 0xc8, 0x07, 0xe0, 0x79, 0x9f, 0x13, 0xb1, 0x27, 0xbc, 0x63, 0x00, 0x6f, 0xd0, 0x0a, 0xff,
0x04, 0xf9, 0x27, 0xe0, 0xff, 0xa7, 0x18, 0xb1, 0xa5, 0xff, 0x21, 0x00, 0x00, 0x01, 0x8a, 0x7f,
0xe0, 0xd9, 0x67, 0xe0, 0xff, 0xa3, 0xdd, 0xb1, 0xa4, 0x00, 0x03, 0x10, 0x00, 0x01, 0xa0, 0x0f,
0x54, 0x39, 0x67, 0xe0, 0xf5, 0xa0, 0xdd, 0xa1, 0xa4, 0x39, 0x83, 0x00, 0x00, 0x03, 0xa3, 0xef,
0xfc, 0x09, 0x61, 0xe0, 0x00, 0x00, 0x3d, 0x8e, 0x24, 0x37, 0x82, 0x00, 0x0f, 0x90, 0xe3, 0xee,
0xff, 0x81, 0xee, 0x60, 0x04, 0x00, 0x1c, 0x07, 0x24, 0xff, 0x83, 0x00, 0x40, 0x00, 0xc3, 0xee,
0xff, 0xe9, 0x2f, 0x80, 0x04, 0x20, 0x1c, 0x1f, 0x26, 0xdf, 0xe3, 0x90, 0x0f, 0x90, 0x5b, 0xef,
0xff, 0xf9, 0x23, 0xe0, 0x04, 0x60, 0x1c, 0x1e, 0x24, 0xf6, 0x80, 0x00, 0x00, 0x00, 0x5d, 0xef,
0xff, 0xf9, 0x20, 0xe0, 0x04, 0x38, 0x1c, 0x24, 0xa6, 0x88, 0xc0, 0x00, 0x0a, 0x00, 0x48, 0xef,
0xff, 0xf9, 0xe0, 0x20, 0x00, 0x0e, 0x1c, 0x01, 0x26, 0x0c, 0x61, 0xe4, 0x20, 0x00, 0x5e, 0xef,
0xff, 0xf9, 0x00, 0x87, 0x80, 0x78, 0x1c, 0x00, 0x06, 0x00, 0x01, 0x20, 0x00, 0x00, 0x5f, 0xef,
0xff, 0xf9, 0x00, 0x89, 0x9f, 0xff, 0x1c, 0x00, 0x20, 0x81, 0x00, 0x00, 0x29, 0x10, 0x5f, 0x6f,
0xff, 0xf9, 0x20, 0x88, 0x1f, 0xf6, 0xcc, 0x7c, 0x60, 0x00, 0x25, 0xfd, 0x02, 0x00, 0x5f, 0xef,
0xff, 0xf9, 0xe0, 0x88, 0x9f, 0xff, 0x9c, 0x3e, 0xe3, 0x7f, 0xa4, 0xbc, 0x00, 0x00, 0x03, 0xef,
0xff, 0xf9, 0x20, 0x88, 0xdf, 0xff, 0xcc, 0x84, 0x03, 0x56, 0xa7, 0xec, 0x88, 0x00, 0x1f, 0xee,
0x1f, 0xf9, 0x27, 0x08, 0xc0, 0x1f, 0xe1, 0xf4, 0x00, 0x14, 0xe7, 0xec, 0x00, 0x00, 0x1f, 0xae,
0x20, 0xf9, 0x20, 0x48, 0xcf, 0x40, 0xd8, 0x36, 0xc4, 0x7f, 0xa7, 0x9e, 0x00, 0x40, 0x03, 0xae,
0x28, 0x19, 0xe0, 0x10, 0xc0, 0x00, 0x0f, 0x06, 0xe0, 0x14, 0xe7, 0x9f, 0x79, 0xe0, 0x59, 0xae,
0xa9, 0x48, 0x00, 0x00, 0x99, 0xb1, 0x00, 0x56, 0x48, 0xff, 0xa7, 0xdc, 0xf8, 0x80, 0x01, 0x2e,
0xa9, 0x48, 0x00, 0x00, 0x0e, 0x33, 0x40, 0x00, 0x08, 0x96, 0xe5, 0xfc, 0x30, 0xc0, 0x00, 0x2e,
0x29, 0x08, 0x01, 0x80, 0x9c, 0xf3, 0x40, 0x02, 0x68, 0xde, 0xe6, 0x7e, 0x47, 0x00, 0x40, 0x2e,
0x29, 0x49, 0xe1, 0x98, 0x10, 0x03, 0x42, 0x1b, 0x28, 0xf7, 0xa7, 0xfd, 0x8e, 0x00, 0x01, 0x20,
0x09, 0x49, 0x21, 0x19, 0x97, 0x73, 0x42, 0x08, 0x00, 0x94, 0xe4, 0xfd, 0x18, 0xc3, 0xf8, 0x26,
0x00, 0x01, 0x21, 0x19, 0x97, 0x73, 0x40, 0x0d, 0x80, 0xff, 0xa4, 0xd8, 0x00, 0x00, 0x08, 0x26,
0x78, 0x09, 0x21, 0x19, 0x96, 0x63, 0x40, 0x04, 0x04, 0x94, 0xe6, 0x10, 0x00, 0x03, 0xf8, 0x26,
0x27, 0x41, 0x01, 0x19, 0x92, 0x73, 0x40, 0x0b, 0x06, 0xff, 0xa0, 0x00, 0x00, 0x00, 0x48, 0x26,
0x8d, 0x78, 0x01, 0x19, 0x9e, 0x33, 0x40, 0x08, 0x00, 0x94, 0xc7, 0xbe, 0x1e, 0x02, 0x48, 0x26,
0x99, 0xc8, 0x00, 0x11, 0x8e, 0x42, 0x40, 0x08, 0x08, 0xfe, 0x80, 0xf0, 0x01, 0xc3, 0xf9, 0x26,
0x10, 0xc0, 0x00, 0x00, 0x0e, 0x06, 0x00, 0x08, 0x80, 0x03, 0x8f, 0xc0, 0x01, 0xc0, 0x00, 0x26,
0x10, 0x80, 0x00, 0x00, 0x02, 0x17, 0x80, 0x08, 0x84, 0x01, 0x0c, 0x40, 0x0f, 0x40, 0x00, 0x26,
0x10, 0x80, 0x22, 0x00, 0x1e, 0x17, 0x90, 0x08, 0x05, 0xf0, 0x09, 0xff, 0xff, 0x00, 0x1c, 0x26,
0x10, 0x81, 0x00, 0x24, 0x1e, 0x17, 0xa0, 0x08, 0x00, 0x06, 0xee, 0x1f, 0xf1, 0x40, 0x0c, 0x26,
0x10, 0x81, 0x08, 0x00, 0x08, 0x17, 0xa6, 0x08, 0x00, 0x1c, 0x0a, 0x1f, 0xf0, 0xc3, 0xe6, 0x26,
0x10, 0x80, 0x18, 0x12, 0x0a, 0x37, 0x80, 0x01, 0x03, 0x3c, 0x08, 0x0f, 0x60, 0xc0, 0x00, 0x26,
0x90, 0x80, 0x19, 0x12, 0x0f, 0x76, 0xc0, 0x00, 0x01, 0x7f, 0xce, 0x10, 0xf1, 0xdf, 0xf8, 0x27,
0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf8, 0x27,
0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x08, 0x00, 0x00, 0x02, 0x00, 0x24,
0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x0e, 0x34, 0x1a, 0x04, 0x01, 0x82, 0x00, 0x07,
0x00, 0x08, 0x00, 0x00, 0x10, 0x01, 0xc0, 0x02, 0x16, 0x06, 0x0b, 0x54, 0x01, 0x8a, 0x50, 0x06,
0x4e, 0x3c, 0xc0, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x3c, 0x14, 0x08, 0xd0, 0x00, 0x02, 0x00, 0x08,
0xc0, 0x1c, 0xc3, 0x01, 0x88, 0x00, 0x01, 0x00, 0x00, 0x10, 0x00, 0x50, 0x00, 0x02, 0x00, 0x08,
0x82, 0x08, 0xb1, 0x01, 0x10, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11,
0x80, 0x10, 0x00, 0x00, 0x30, 0x00, 0xa0, 0x07, 0xfc, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x30, 0x21,
0x00, 0x00, 0x07, 0x88, 0x10, 0x00, 0x00, 0x07, 0xff, 0x7f, 0xf1, 0xe3, 0xe1, 0xf9, 0xe7, 0xe3,
0x00, 0x00, 0x33, 0xde, 0x00, 0x00, 0x00, 0x0c, 0x05, 0xff, 0xf7, 0x83, 0x8f, 0xfc, 0x43, 0xc7,
0x43, 0xce, 0x01, 0x97, 0x0f, 0xf0, 0x02, 0x4e, 0x7d, 0x30, 0x67, 0x80, 0x0f, 0x78, 0x00, 0xc6,
0xed, 0xff, 0x43, 0x93, 0x1f, 0xff, 0xfe, 0x1e, 0x02, 0x04, 0x47, 0x80, 0x08, 0x00, 0x00, 0x8e,
0xf0, 0xc5, 0x20, 0x37, 0x1f, 0xff, 0x87, 0x76, 0x03, 0x0f, 0xd7, 0x83, 0x00, 0x00, 0x81, 0x8c,
0xc0, 0x00, 0xe0, 0xe0, 0x1f, 0xfe, 0x07, 0xff, 0x03, 0x0f, 0x80, 0x00, 0x00, 0x00, 0x01, 0x1c,
0xc0, 0x24, 0x20, 0x00, 0x4f, 0xfc, 0x3d, 0xe0, 0x04, 0x00, 0x10, 0x00, 0x5f, 0xff, 0x83, 0x18,
0xf0, 0x18, 0x00, 0x00, 0x8e, 0x70, 0x38, 0xf8, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x10,
0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0xf8, 0xff, 0xff, 0x03, 0xff, 0x00, 0x00, 0x03, 0x1f,
0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x78, 0xf8, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x03, 0xff,
0x00, 0x00, 0x00, 0x7f, 0xe0, 0x00, 0xf0, 0xe0, 0xff, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xf7, 0xff,
0x00, 0x00, 0x7f, 0xfe, 0x00, 0x01, 0xf0, 0x80, 0x03, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xe7, 0xff,
0x00, 0x37, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xf3, 0xff, 0xff, 0xcf, 0xff,
0x07, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xc7, 0xff, 0xff, 0xcf, 0xff,
0xff, 0xff, 0xfe, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xcf, 0xff,
0xe0, 0xc1, 0x2e, 0x3f, 0xe0, 0x00, 0x00, 0x00, 0x7f, 0x3f, 0xf3, 0x3f, 0xbf, 0xfb, 0x5f, 0xff,
0x3f, 0x03, 0xff, 0xff, 0xef, 0xfe, 0xff, 0xf7, 0xff, 0xff, 0xf7, 0xff, 0x7f, 0xfe, 0xdf, 0xf8,
0x00, 0x7f, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xdf, 0xfe, 0xff, 0xfe, 0xdc, 0x00,
0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xff, 0x7f, 0xfd, 0xff, 0xff, 0xc8, 0x00
};

static const uint32_t palette_cityscape1BPP_UNCOMP[]=
{
    0x000000,   0xffffff
};

const tImage  cityscape1BPP_UNCOMP=
{
    IMAGE_FMT_1BPP_UNCOMP,
    128,
    182,
    2,
    palette_cityscape1BPP_UNCOMP,
    pixel_cityscape1BPP_UNCOMP,
};

////////////////////////////////////

void ui_draw_top_bar() {
    // Draw the top bar.
    Graphics_drawLine(&ui_gr_context, 0, 28, 295, 28);

    // Draw the battery symbol.
    // Determine the battery voltage.
    // 0.5 V is the minimum. 3.0V is the "maximum"
    // They're probably just about empty when we get to 1.8 V.
    Graphics_Rectangle rect;
    rect.xMin = 248;
    rect.xMax = 291;
    rect.yMin = 2;
    rect.yMax = 18;
    Graphics_drawRectangle(&ui_gr_context, &rect);
    rect.xMin = 291;
    rect.xMax = 295;
    rect.yMin = 6;
    rect.yMax = 13;
    Graphics_drawRectangle(&ui_gr_context, &rect);

    Graphics_setFont(&ui_gr_context, &g_sFontCmss16b);
    Graphics_setFont(&ui_gr_context, &g_sFontFixed6x8);
    char bat_text[5] = "3.0V";
    sprintf(bat_text, "%d.%dV", vbat_out_uvolts/1000000, (vbat_out_uvolts/100000) % 10);
    Graphics_drawStringCentered(&ui_gr_context, (int8_t *) bat_text, 4, 248 + (291-248)/2, 2 + (18-2)/2, 0);
    Graphics_flushBuffer(&ui_gr_context);

    // Is our agent around? Draw the agent symbol.

    // Are there any handlers around? Draw the handler symbol.


}

void ui_draw_main_menu() {
    // Clear the buffer.
    Graphics_clearDisplay(&ui_gr_context);

    ui_draw_top_bar();

    // Draw the menu itself

}

#define UI_AUTOREFRESH_TIMEOUT 6000000

void ui_task_fn(UArg a0, UArg a1) {
    UInt events;
    uint8_t light_brightness = 0;

    // TODO: move these:
    init_epd();
    ht16d_init_io();
    ht16d_init();
//    ht16d_all_one_color(255,255,255);
    ht16d_all_one_color(light_brightness,light_brightness,light_brightness);

    Event_post(ui_event_h, UI_EVENT_REFRESH);

    while (1) {
        events = Event_pend(ui_event_h, Event_Id_NONE, UI_EVENT_ALL,  UI_AUTOREFRESH_TIMEOUT);

        if (!events || events & UI_EVENT_REFRESH) {
            if (!events) {
                // Do this one as partial:
                epd_do_partial = 1;
            }
            // time to redraw the current view.
            // TODO: wrong:
            ui_draw_main_menu();
        }

        if (events & UI_EVENT_KB_PRESS) {
            // keyboard press.
            if (kb_active_key == BTN_UP) {
                if (light_brightness > 245) {
                    light_brightness = 255;
                } else {
                    light_brightness += 10;
                }
            } else if (kb_active_key == BTN_DOWN) {
                if (light_brightness < 10) {
                    light_brightness = 0;
                } else {
                    light_brightness -= 10;
                }
            }
            ht16d_all_one_color(light_brightness,light_brightness,light_brightness);
        }

        Task_yield();
    }

}

void ui_init() {
    // IO init:

    // TODO: consider enabling hysteresis?
    // The ROW SCAN sets ROWS as INPUTS, with PULL DOWNS
    //              and, COLS as OUTPUTS, HIGH.

    kb_pin_h = PIN_open(&btn_state, btn_row_scan);

    Clock_Params clockParams;
    Error_Block eb;
    Error_init(&eb);
    Clock_Params_init(&clockParams);
    clockParams.period = UI_CLOCK_TICKS;
    clockParams.startFlag = TRUE;
    kb_debounce_clock_h = Clock_create(kb_clock_swi, 2, &clockParams, &eb);

    // TODO: rename all the taskParams and such
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stack = ui_task_stack;
    taskParams.stackSize = UI_STACKSIZE;
    taskParams.priority = 1;
    Task_construct(&ui_task, ui_task_fn, &taskParams, NULL);

    Graphics_initContext(&ui_gr_context, &epd_grGraphicsDisplay, &epd_grDisplayFunctions);
    Graphics_setBackgroundColor(&ui_gr_context, GRAPHICS_COLOR_WHITE);
    Graphics_setForegroundColorTranslated(&ui_gr_context, GRAPHICS_COLOR_BLACK);
    Graphics_clearDisplay(&ui_gr_context);

    ui_event_h = Event_create(NULL, NULL);
}
