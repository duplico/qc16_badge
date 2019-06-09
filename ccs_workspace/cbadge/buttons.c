/*
 * buttons.c
 *
 *  Created on: Jun 7, 2019
 *      Author: george
 */

#include <stdint.h>

#include <msp430.h>

#include "cbadge.h"

// Global variables for sensing
unsigned int base_cnt[3];
unsigned int meas_cnt[3];
int delta_cnt[3];
unsigned char key_press[3];
char key_pressed;
const unsigned char electrode_bit[3]={BIT5, BIT1, BIT6,};

/// Measure all the inputs.
void measure_count(void)
{
    // TODO: Totally refactor this.
    unsigned char i;

    // Configure the timer: (There's only Timer B0)
    // We use Timer B to read the captouch input.
    // Use INCLK, and count in continuous 16-bit mode.
    TB0CTL = TBSSEL__INCLK + MC__CONTINUOUS;

    // Configure the CCR (CCR1 on TB0):
    //  Capture on both rising and falling edges of the clock (CM_3)
    //  Wire the capture signal to GND.
    //  Capture mode. (CAP)
    TB0CCTL1 = CM__BOTH + CCIS__GND + CAP__CAPTURE;

    for (i = 0; i<3; i++)
    {
        // Select the correct button:
        switch(i) {
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

        // Delay 0.064 ms using the WDT.
        WDTCTL = WDT_MDLY_0_064;
        __bis_SR_register(LPM0_bits+GIE);

        // Toggle the capture signal, to capture the current counter,
        //  copying it into TB0CCR1.
        TB0CCTL1 ^= CCIS0;
        meas_cnt[i] = TB0CCR1;
        // Hold the WDT.
        WDTCTL = WDTPW + WDTHOLD;
        // Turn off captouch:
        CAPTIOCTL = 0;
    }
}

void button_calibrate() {
    // Read from the buttons, and place it in meas_cnt.
    uint8_t i;
    measure_count();
    // Copy that into base_cnt.
    for (i = 0; i<3; i++)
        base_cnt[i] = meas_cnt[i];

    // Do that 15 more times, to get a really crappy average for some reason.
    // TODO: We should consider persisting this average.
    for(i=15; i>0; i--) {
        measure_count();
        for (uint8_t j = 0; j<3; j++)
            base_cnt[j] = (meas_cnt[j]+base_cnt[j])/2;
    }
}

void button_poll(uint8_t button_id) {
    // Track the previous state of the buttons. The upper nibble will
    //  be the PREVIOUS state of each button, and the lower nibble
    //  will store the CURRENT state of each button. BIT3 and BIT7,
    //  the highest bit of each nibble, are unused.
    static uint8_t button_state = 0;

    // Measure all buttons:
    measure_count();

    // For each button...
    for (uint8_t i = 0; i<3; i++)
    {
        uint8_t button_bit = (BIT0 << i);
        // What's the change in our measurement?
        delta_cnt[i] = base_cnt[i] - meas_cnt[i];

        // We measured an INCREASE in the measurement, which is
        //  NOT a button press.
        if (delta_cnt[i] < 0) {
            // We should average that into our baseline.
            base_cnt[i] = (base_cnt[i]+meas_cnt[i]) >> 1;
            // Now, make it zero so it doesn't trigger the next if statement.
            delta_cnt[i] = 0;
        }

        // If the measurement DECREASED, more than our threshold, that
        //  IS a button press.
        if (delta_cnt[i] > KEY_LVL) {
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

    // TODO: Was this actually needed?
    // If no key is pressed, gradually lower the baseline.
//    if (!key_pressed) {
//        for (uint8_t i = 0; i<3; i++)
//            base_cnt[i] = base_cnt[i] - 1;
//    }
}
