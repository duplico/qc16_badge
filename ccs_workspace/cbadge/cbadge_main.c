#include <stdint.h>

#include <msp430fr2100.h>

#include <cbadge.h>

#include "buttons.h"

uint8_t s_button = 0x00;

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
    // P1.1     B2B DIO1    (SEL 00; DIR 1)
    // P1.2     RX (alt)    (SEL 01; DIR 0)
    // P1.3     TX (alt)    (SEL 01; DIR 1)
    // P1.4     B2B DIO2    (SEL 00; DIR 0)
    // P1.5     B1          (SEL 00; DIR 0)
    // P1.6     RX (base)   (SEL 01; DIR 0)
    // P1.7     TX (base)   (SEL 01; DIR 1)
    P1DIR = 0b10001011;
    P1SEL0 = 0b11001100; // LSB
    P1SEL1 = 0b00000000; // MSB
    // P2.0     LED B       (SEL: 00; DIR 1)
    // P2.1     B2          (SEL: 00; DIR 0)
    //  ~~(There are no P.{2..5})~~
    // P2.6     B3          (SEL: 00; DIR 0)
    // P2.7     LED C       (SEL: 00; DIR 1)
    P2DIR = 0b10000001;
    P2SEL0 = 0x00;
    P2SEL1 = 0x00;
}

void init_serial() {
    // We'll start with the ALTERNATE config.
    SYSCFG3 |= USCIARMP_1;

    // Pause the UART peripheral:
    UCA0CTLW0 |= UCSWRST;
    // Source the baud rate generation from SMCLK (1 MHz)
    UCA0CTLW0 |= UCSSEL__SMCLK;
    // Configure the baud rate to 9600.
    //  (See page 589 in the family user's guide, SLAU445I)
    UCA0BR0 = 6; // 1000000/9600/16
    UCA0BR1 = 0x00;
    UCA0MCTLW = 0x2000 | UCOS16 | UCBRF_8;
    // Activate the UART:
    UCA0CTLW0 &= ~UCSWRST;
//    UCA0IE |= UCTXIE | UCRXIE; // TODO
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

    // TODO: Read P1.4 to determine whether we're free-standing, or
    //       if we're connected to a badge that's powering us. If we're
    //       powered-up, then switch that pin to an output to signal that
    //       this

     while (1)
    {
        button_poll();

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

        if (UCA0IFG & UCRXIFG) {
            // We got a message!
            LEDA_PORT_OUT ^= LEDA_PIN;
            LEDB_PORT_OUT ^= LEDB_PIN;
            LEDC_PORT_OUT ^= LEDC_PIN;
            volatile uint8_t i;
            i = UCA0RXBUF;
        }

        // Delay 16 ms:
        WDTCTL = WDT_ADLY_16;
        __bis_SR_register(LPM3_bits);
    }
}


#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void)
{
    __bic_SR_register_on_exit(LPM3_bits);
}
