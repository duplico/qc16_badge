#include <stdint.h>

#include <msp430fr2100.h>

#include <cbadge.h>

#include "serial.h"
#include "buttons.h"

uint8_t s_activated = 0x00;
uint8_t s_button = 0x00;
volatile uint8_t f_serial = 0x00;
volatile uint8_t f_time_loop = 0x00;

// TODO: initialize
#pragma PERSISTENT(my_conf)
cbadge_conf_t my_conf = {
    .activated=0,
    .active=0,
    .badge_id=0,
    .initialized=0,
};

/// Initialize clock signals and the three system clocks.
/**
 ** We'll take the DCO to 8 MHz, and divide it by 8 for MCLK.
 **
 ** Our available clock sources are:
 **  VLO:     10kHz very low power low-freq
 **  REFO:    32.768kHz (typ) reference oscillator
 **  DCO:     Digitally controlled oscillator (1MHz default)
 **           Specifically, 1048576 Hz typical.
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
    // Use the defaults.
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
    P2OUT = 0x00;
}

void init_conf() {
    if (!my_conf.initialized) {
        // this is first boot.
        // TODO:
        my_conf.initialized=1;
        my_conf.badge_id=1001;
        my_conf.activated=0;
    }
    my_conf.active=0;
}

/// Perform basic initialization of the cbadge.
void init() {
    // Stop the watchdog timer.
    WDTCTL = WDTPW | WDTHOLD;

    // Set up the clock system:
    init_clocks();
    init_io();
    init_serial();
    init_conf();
    // Note: Serial alternative switching is controlled by TBRMP in SYSCFG3

    // Enable interrupt for the WDT:
    SFRIE1 |= WDTIE;
    __bis_SR_register(GIE);

//    button_calibrate();
}

int main( void )
{
    init();

    // Read P1.1 to determine whether we're free-standing, or
    //  if we're connected to a badge that's powering us. If it's LOW,
    //  then this badge is active WITHOUT another badge powering us.
    //  How cool! Switch that pin to an output to signal that
    //  this badge is now active.
    if (!(P1IN & BIT1)) {
        // We are under our own power.
        // Assert the Active Badge Signal (ABS, P1.1).
        P1DIR |= BIT1;
        P1OUT |= BIT1;
        if (!my_conf.activated) {
            my_conf.activated = 1;
            // This badge was just turned on under its own power for the first time!
            s_activated = 1;
        }
        my_conf.active = 1;
    }

    uint8_t current_button = 0;

    f_time_loop = 1;

    uint8_t pwm_level_curr = 0;
    uint8_t pwm_level_a = 10;
    uint8_t pwm_level_b = 4;
    uint8_t pwm_level_c = 1;

    while (1) {
        if (f_time_loop) {
            // Every time loop, we measure ONE of the buttons.

            // But, receiving serial messages plays havoc with our readings.
            //  So, if the serial PHY is active, we need to avoid scanning
            //  buttons.

            if (serial_phy_state == SERIAL_PHY_STATE_IDLE) {
                if (current_button >= 0xf0) {
                    // We're coming off a HOLD, so our timer readings
                    //  are going to be invalid. So, let's restart our
                    //  scanning process, and not poll a button
                    //  this loop.
                    current_button = current_button & 0x0f;
                } else {
                    // It's ok to poll a button.
                    button_poll(current_button);
                    current_button += 1;
                    if (current_button == 3)
                        current_button = 0;
                }
                button_measure_start(current_button);
            } else {
                current_button |= 0xf0;
            }

            pwm_level_curr++;

            if (pwm_level_curr == 10)
                pwm_level_curr = 0;

            if (pwm_level_a > pwm_level_curr)
                LEDA_PORT_OUT |= LEDA_PIN;
            else
                LEDA_PORT_OUT &= ~LEDA_PIN;

            if (pwm_level_b > pwm_level_curr)
                LEDB_PORT_OUT |= LEDB_PIN;
            else
                LEDB_PORT_OUT &= ~LEDB_PIN;

            if (pwm_level_c > pwm_level_curr)
                LEDC_PORT_OUT |= LEDC_PIN;
            else
                LEDC_PORT_OUT &= ~LEDC_PIN;

            // Delay another 1.9 ms:
            WDTCTL = WDT_ADLY_1_9;

            f_time_loop = 0;
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
            serial_handle_rx();

            f_serial = 0;
        }

        // TODO: If we're an ACTIVE BADGE, we should be sending our own
        //       serial HELO messages...

        if (f_serial == SERIAL_TX_DONE) {
            if (serial_header_out.opcode == SERIAL_OPCODE_ACK) {
                // We are now connected.
                LEDA_PORT_OUT ^= LEDA_PIN;
                LEDB_PORT_OUT ^= LEDB_PIN;
                LEDC_PORT_OUT ^= LEDC_PIN;
            }

            f_serial = 0;
        }

        __bis_SR_register(LPM3_bits);
    }
}

#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void)
{
    if (serial_active_ticks && serial_phy_state) {
        serial_active_ticks--;
        if (!serial_active_ticks) {
            serial_phy_state = SERIAL_PHY_STATE_IDLE;
        }
    }
    f_time_loop = 1;
    __bic_SR_register_on_exit(LPM3_bits);
}
