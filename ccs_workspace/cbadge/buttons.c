/*
 * buttons.c
 *
 *  Created on: Jun 7, 2019
 *      Author: george
 */

#include <stdint.h>

#include <msp430.h>

#define NUM_SEN     3                       // Defines number of sensors
#define KEY_LVL     600                     // Defines threshold for a key press
/*Set to ~ half the max delta expected*/

// Global variables for sensing
unsigned int base_cnt[NUM_SEN] = { 1230, 1230, 1230};
unsigned int meas_cnt[NUM_SEN];
int delta_cnt[NUM_SEN];
unsigned char key_press[NUM_SEN];
char key_pressed;
const unsigned char electrode_bit[NUM_SEN]={BIT5, BIT1, BIT6,};

/// Measure all the inputs.
void measure_count(void)
{
    unsigned char i;

    // Configure the timer: (There's only Timer B0)
    // We use Timer B to read the captouch input.
    // Use INCLK, and count in continuous 16-bit mode.
    TB0CTL = TBSSEL__INCLK + MC__CONTINUOUS;                   // INCLK, cont mode

    // Configure the CCR (CCR1 on TB0):
    //  Capture on both rising and falling edges of the clock (CM_3)
    //  Wire the capture signal to GND.
    //  Capture mode. (CAP)
    TB0CCTL1 = CM__BOTH + CCIS__GND + CAP__CAPTURE;

    for (i = 0; i<NUM_SEN; i++)
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

        // Clear the timer:
        TB0CTL |= TBCLR;

        // Delay 0.5 ms using the WDT.
        WDTCTL = WDT_MDLY_0_5;
        __bis_SR_register(LPM0_bits+GIE);

        // Toggle the capture signal, to capture the current counter,
        //  copying it into TB0CCR1.
        TB0CCTL1 ^= CCIS0;                        // Create SW capture of CCR1
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
    for (i = 0; i<NUM_SEN; i++)
        base_cnt[i] = meas_cnt[i];

    // Do that 15 more times, to get a really crappy average for some reason.
    // TODO: We should consider persisting this average.
    for(i=15; i>0; i--) {
        measure_count();
        for (uint8_t j = 0; j<NUM_SEN; j++)
            base_cnt[j] = (meas_cnt[j]+base_cnt[j])/2;
    }
}

void button_poll() {
    uint8_t j = KEY_LVL;
    key_pressed = 0;

    // Measure all buttons:
    measure_count();

    // For each button...
    for (uint8_t i = 0; i<NUM_SEN; i++)
    {
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
        if (delta_cnt[i] > j) {
            // Indicate that this key is pressed.
            key_press[i] = 1;
            // Increase our detection threshold to this detection level:
            j = delta_cnt[i];
            // Mark this as the pressed-est key:
            key_pressed = i+1;
        } else {
            key_press[i] = 0;
        }
    }

    // TODO: Was this actually needed?
    // If no key is pressed, gradually lower the baseline.
//    if (!key_pressed) {
//        for (uint8_t i = 0; i<NUM_SEN; i++)
//            base_cnt[i] = base_cnt[i] - 1;
//    }
}
