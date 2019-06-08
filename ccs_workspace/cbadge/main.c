#include <stdint.h>

#include <msp430fr2100.h>

#include <cbadge.h>

#include "buttons.h"

#define LEDA_PORT_OUT P2OUT
#define LEDA_PIN BIT7
#define LEDB_PORT_OUT P2OUT
#define LEDB_PIN BIT0
#define LEDC_PORT_OUT P1OUT
#define LEDC_PIN BIT0

void init_clocks() {
    // Our available clock sources are:
    // VLO:     10kHz very low power low-freq
    // REFO:    32.768kHz (typ) reference oscillator
    // DCO:     Digitally controlled oscillator (1MHz default)

    // At startup, our clocks are as follows:
    // MCLK:  Sourced by the DCO
    //        (Available: DCO, REFO, VLO)
    // SMCLK: Sourced from MCLK, with no divider
    //        (Available dividers: {1,2,4,8})
    // ACLK: Sourced from REFO
    //        (the only available internal source)
}

void init_io() {
    // Unlock the pins from high-impedance mode:
    // (AKA the MSP430FR magic make-it-work command)
    PM5CTL0 &= ~LOCKLPM5;

    // GPIO:
    // P1.0     LED A       (SEL 00; DIR 1)
    // P1.1     B2B DIO1    (SEL 00; DIR 1)
    // P1.2     TX          TODO    DIR 1
    // P1.3     RX          TODO    DIR 0
    // P1.4 B2B DIO2        (SEL 00; DIR 0)
    // P1.5 B1              (SEL 00; DIR 0)
    // P1.6 RX              TODO    DIR 0
    // P1.7 TX              TODO    DIR 1
    P1DIR = 0b10000111;
    P1SEL0 = 0b00000000; // LSB
    P1SEL1 = 0b00000000; // MSB
    // P2.0 LED B           (SEL: 00; DIR 1)
    // P2.1 B2              (SEL: 00; DIR 0)
    //  ~~(There are no P.{2..5})~~
    // P2.6 B3              (SEL: 00; DIR 0)
    // P2.7 LED C           (SEL: 00; DIR 1)
    P2DIR = 0b10000001;
    P2SEL0 = 0x00;
    P2SEL1 = 0x00;
}





void init() {
    // Stop the watchdog timer.
    WDTCTL = WDTPW | WDTHOLD;

    // Set up the clock system:
    init_clocks();
    init_io();

    // Enable interrupt for the WDT:
    SFRIE1 |= WDTIE;
    __bis_SR_register(GIE);

    button_calibrate();
}

void pulse_LED(void)
{
    switch (key_pressed) {
    case 1:
        LEDA_PORT_OUT ^= LEDA_PIN;
        break;
    case 2:
        LEDB_PORT_OUT ^= LEDB_PIN;
        break;
    case 3:
        LEDC_PORT_OUT ^= LEDC_PIN;
        break;
    }
}

int main( void )
{
    init();

    while (1)
    {
        button_poll();
        // Delay 16 ms:
        WDTCTL = WDT_ADLY_16;
        pulse_LED();
        __bis_SR_register(LPM3_bits);
    }
}

/* Watchdog Timer interrupt service routine*/
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(WDT_VECTOR))) watchdog_timer (void)
#else
#error Compiler not supported!
#endif
{
    __bic_SR_register_on_exit(LPM3_bits);     // Exit LPM3 on reti
}
