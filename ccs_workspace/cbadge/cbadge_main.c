#include <stdint.h>

#include <msp430fr2100.h>

#include <cbadge.h>

#include "serial.h"
#include "buttons.h"

uint8_t s_button = 0x00;
volatile uint8_t f_serial = 0x00;
volatile uint8_t f_time_loop = 0x00;

// TODO: initialize
cbadge_conf_t my_conf;
cbadge_conf_t my_conf_backup;

/// Initialize clock signals and the three system clocks.
/**
 ** Currently, we're just keeping the defaults, but this is where any
 ** such initialization should be done if needed.
 **
 ** Our available clock sources are:
 **  VLO:     10kHz very low power low-freq
 **  REFO:    32.768kHz (typ) reference oscillator
 **  DCO:     Digitally controlled oscillator (1MHz default)
 **
 ** At startup, our clocks are as follows:
 **  MCLK:  Sourced by the DCO
 **         (Available: DCO, REFO, VLO)
 **  SMCLK: Sourced from MCLK, with no divider
 **         (Available dividers: {1,2,4,8})
 **  ACLK: Sourced from REFO
 **         (the only available internal source)
 */
void init_clocks() {
}

/// Apply the initial configuration of the GPIO and peripheral pins.
/**
 **
 */
void init_io() {
    // Unlock the pins from high-impedance mode:
    // (AKA the MSP430FR magic make-it-work command)
    PM5CTL0 &= ~LOCKLPM5;

    // GPIO:
    // P1.0     LED A       (SEL 00; DIR 1)
    // P1.1     B2B ABS     (SEL 00; DIR 0) (pull-down)
    // P1.2     RX (alt)    (SEL 01; DIR 0)
    // P1.3     TX (alt)    (SEL 01; DIR 1)
    // P1.4     B2B DIO2    (SEL 00; DIR 1)
    // P1.5     B1          (SEL 00; DIR 0)
    // P1.6     RX (base)   (SEL 01; DIR 0)
    // P1.7     TX (base)   (SEL 01; DIR 1)
    P1DIR = 0b10001011;
    P1SEL0 = 0b11001100; // LSB
    P1SEL1 = 0b00000000; // MSB
    P1REN = 0b00000010;
    P1OUT = 0x00;
    // P2.0     LED B       (SEL: 00; DIR 1)
    // P2.1     B2          (SEL: 00; DIR 0)
    //  ~~(There are no P.{2..5})~~
    // P2.6     B3          (SEL: 00; DIR 0)
    // P2.7     LED C       (SEL: 00; DIR 1)
    P2DIR = 0b10000001;
    P2SEL0 = 0x00;
    P2SEL1 = 0x00;
}

/// Perform basic initialization of the cbadge.
void init() {
    // Stop the watchdog timer.
    WDTCTL = WDTPW | WDTHOLD;

    // Set up the clock system:
    init_clocks();
    init_io();
    init_serial();
    // Note: Serial alternative switching is controlled by TBRMP in SYSCFG3

    // Enable interrupt for the WDT:
    SFRIE1 |= WDTIE;
    __bis_SR_register(GIE);

    button_calibrate();
}

int main( void )
{
    init();

    // Read P1.1 to determine whether we're free-standing, or
    //  if we're connected to a badge that's powering us. If we're
    //  powered-up, then switch that pin to an output to signal that
    //  this
    if (P1IN & BIT1) {
        // Assert the Active Badge Signal (ABS, P1.1).
        P1DIR |= BIT1;
        P1OUT |= BIT1;
        // TODO: Set this badge as "powered-up"
    }

    while (1) {
        // TODO: this is a mess:
        if (f_time_loop) {
            button_poll();
            // We DON'T clear f_time_loop here, because there's another section
            //  towards the bottom of our main loop that will clear it for us.
        }

        // TODO: If we're connected to a qbadge, and a button is pressed,
        //       send something about it.
        if (s_button & BUTTON_PRESS_J1) {
            LEDA_PORT_OUT ^= LEDA_PIN;
            s_button &= ~BUTTON_PRESS_J1;
        }
        if (s_button & BUTTON_PRESS_J2) {
            LEDB_PORT_OUT ^= LEDB_PIN;
            s_button &= ~BUTTON_PRESS_J2;
        }
        if (s_button & BUTTON_PRESS_J3) {
            LEDC_PORT_OUT ^= LEDC_PIN;
            s_button &= ~BUTTON_PRESS_J3;
        }
        if (s_button & BUTTON_RELEASE_J1) {
            s_button &= ~BUTTON_RELEASE_J1;
        }
        if (s_button & BUTTON_RELEASE_J2) {
            s_button &= ~BUTTON_RELEASE_J2;
        }
        if (s_button & BUTTON_RELEASE_J3) {
            s_button &= ~BUTTON_RELEASE_J3;
        }

        if (f_serial == SERIAL_RX_DONE) {
            // We got a message!
            LEDA_PORT_OUT ^= LEDA_PIN;
            LEDB_PORT_OUT ^= LEDB_PIN;
            LEDC_PORT_OUT ^= LEDC_PIN;

            serial_handle_rx();

            f_serial = 0;
        }

        // TODO: If we're an ACTIVE BADGE, we should be sending our own
        //       serial HELO messages...

        if (f_serial == SERIAL_TX_DONE) {
            f_serial = 0;
        }


        if (f_time_loop) {
            f_time_loop = 0;
        }

        // Delay 16 ms:
        WDTCTL = WDT_ADLY_16;
        __bis_SR_register(LPM3_bits);
    }
}

// TODO: Add the ability to time out serial comms.
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void)
{
    f_time_loop = 1;
    __bic_SR_register_on_exit(LPM3_bits);
}
