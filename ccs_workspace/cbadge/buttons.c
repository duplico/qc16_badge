/*
 * buttons.c
 *
 *  Created on: Jun 7, 2019
 *      Author: george
 */

#include <stdint.h>

#include <msp430fr2111.h>

#include "cbadge.h"

// Global variables for sensing
unsigned int base_cnt[3];
unsigned int meas_cnt[3];
int delta_cnt[3];

void button_measure_start(uint8_t button_id) {
    // Configure the timer: (There's only Timer B0)
    // We use Timer B to read the captouch input.
    // Use INCLK, and count in continuous 16-bit mode.
    TB0CTL = TBSSEL__INCLK + MC__CONTINUOUS;

    // Configure the CCR (CCR1 on TB0):
    //  Capture on both rising and falling edges of the clock (CM_3)
    //  Wire the capture signal to GND.
    //  Capture mode. (CAP)
    TB0CCTL1 = CM__BOTH + CCIS__GND + CAP__CAPTURE;
    // Select the correct button:

    switch(button_id) {
    case 0:
        // Button 1: P1.5
        CAPTIOCTL |= CAPTIOPOSEL_1 +CAPTIOPISEL_5+CAPTIOEN;
        break;
    case 1:
        // Button 2: P2.1
        CAPTIOCTL |= CAPTIOPOSEL_2+CAPTIOPISEL_1+CAPTIOEN;
        break;
    case 2:
        // Button 3: P2.6
        CAPTIOCTL |= CAPTIOPOSEL_2+CAPTIOPISEL_6+CAPTIOEN;
        break;
    }

    // Zero the captouch timer:
    TB0CTL |= TBCLR;

    // The timer is now ticking.
}

// Call button_measure_start() first, then after a fixed amount of time, call:
void button_poll(uint8_t button_id) {
    // Track the previous state of the buttons. The upper nibble will
    //  be the PREVIOUS state of each button, and the lower nibble
    //  will store the CURRENT state of each button. BIT3 and BIT7,
    //  the highest bit of each nibble, are unused.
    static uint8_t button_state = 0;

    // Toggle the capture signal, to capture the current counter,
    //  copying it into TB0CCR1.
    TB0CCTL1 ^= CCIS0;
    meas_cnt[button_id] = TB0CCR1;

    // Turn off captouch:
    CAPTIOCTL = 0;

    uint8_t button_bit = (BIT0 << button_id);
    // What's the change in our measurement?
    delta_cnt[button_id] = base_cnt[button_id] - meas_cnt[button_id];

    // We measured an INCREASE in the measurement, which is
    //  NOT a button press.
    if (delta_cnt[button_id] < 0) {
        // We should average that into our baseline.
        base_cnt[button_id] = (base_cnt[button_id]+meas_cnt[button_id]) >> 1;
        // Now, make it zero so it doesn't trigger the next if statement.
        delta_cnt[button_id] = 0;
    }

    // If the measurement DECREASED, more than our threshold, that
    //  IS a button press.
    if (delta_cnt[button_id] > KEY_LVL) {
        // If the button is not already marked as pressed...
        if (!(button_state & button_bit)) {
            button_state |= button_bit;
            // BUTTON PRESSED:
            s_button |= button_bit;
        }
    } else {
        if (button_state & button_bit) {
            button_state &= ~button_bit;
            // BUTTON RELEASED:
            s_button |= (button_bit << 4);
        }
    }

}
