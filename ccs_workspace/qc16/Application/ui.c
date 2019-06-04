/*
 * ui.c
 *
 *  Created on: Jun 3, 2019
 *      Author: george
 */
#include <stdint.h>

#include <xdc/runtime/Error.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/drivers/PIN.h>

#include "ui.h"
#include "board.h"

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
            // TODO: signal a press
            kb_active_key = button_press;
        } else if (kb_active_key && button_press == button_press_prev) {
            // A button is released.
            // TODO: signal a release
            // No need to clear kb_active_key.
        }
        //  We should have two event signals: KB_PRESS, KB_RELEASE
        //  And then we need to store, somewhere, which key is relevant.
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
}
