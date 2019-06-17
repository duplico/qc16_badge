#include <stdint.h>

#include <msp430fr2111.h>

#include <cbadge.h>
#include <cbadge_serial.h>

#include "buttons.h"

volatile uint8_t main_loop_ticks_curr=0;
volatile uint8_t button_poll_this_ms = 0;

/// Signal indicating that the badge is self-powered for the first time.
uint8_t s_activated = 0x00;
/// Signal for all captouch button events.
uint8_t s_button = 0x00;
/// Signal for the serial link-layer.
uint8_t s_serial_ll = 0;
/// Interrupt flag for the serial PHY
volatile uint8_t f_serial_phy = 0x00;
/// Interrupt flag indicating it's time to poll a captouch button.
volatile uint8_t f_button_poll = 0x00;
/// Interrupt flag indicating it's time to do another PWM time step.
volatile uint8_t f_pwm_loop = 0x00;
/// Interrupt flag indicating 1 ms has passed.
volatile uint8_t f_ms = 0x00;

#pragma PERSISTENT(my_conf)
cbadge_conf_t my_conf = {
    .activated=0,
    .active=0,
    .badge_id=0,
    .initialized=0,
};

/// Initialize clock signals and the three system clocks.
/**
 ** We'll take the DCO to 8 MHz, and divide it by 2 for MCLK.
 ** Then we'll divide MCLK by 4 to get 1 MHz SMCLK.
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
    // DCO  (Digitally-controlled oscillator)
    //  Let's bring this up to 8 MHz or so.

    __bis_SR_register(SCG0);                // disable FLL
    CSCTL3 |= SELREF__XT1CLK;               // Set XT1CLK as FLL reference source
    CSCTL0 = 0;                             // clear DCO and MOD registers
    CSCTL1 &= ~(DCORSEL_7);                 // Clear DCO frequency select bits first
    CSCTL1 |= DCORSEL_3;                    // Set DCO = 8MHz
    // CSCTL feedback loop:
    CSCTL2 = FLLD_0 + 243;                  // DCODIV = /1
    __delay_cycles(3);
    __bic_SR_register(SCG0);                // enable FLL
    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)); // Poll until FLL is locked

    // SYSTEM CLOCKS
    // =============

    // MCLK (1 MHz)
    //  All sources but MODOSC are available at up to /128
    //  Set to DCO/2 = 4 MHz
    CSCTL5 |= DIVM__2;

    // SMCLK (1 MHz)
    //  Derived from MCLK with divider up to /8
    //  Set to MCLK/4 = 1 MHz
    CSCTL5 |= DIVS__4;
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
    // P1.1     B2B ABS     (SEL 00; DIR 0) (pull-down) (DIO1)
    // P1.2     RX (PTX)    (SEL 01; DIR 0)
    // P1.3     TX (PTX)    (SEL 01; DIR 1)
    // P1.4     B2B RTR     (SEL 00; DIR 1 OUT 0) (DIO2)
    // P1.5     B1          (SEL 00; DIR 0)
    // P1.6     RX (PRX)   (SEL 01; DIR 0)
    // P1.7     TX (PRX)   (SEL 01; DIR 1)
    // (PRX is the default config)
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
        my_conf.initialized=1;
        my_conf.badge_id=1001;
        my_conf.activated=0;
    }
    my_conf.active=0;
}

/// Perform basic initialization of the cbadge.
void init() {
    // Stop the watchdog timer.
    WDTCTL = WDTPW | WDTCNTCL;

    // Set up the clock system:
    init_clocks();
    init_io();
    init_serial();
    init_conf();

    // Set up the WDT to do our time loop.
    WDTCTL = TICK_WDT_BITS;
    // Enable interrupt for the WDT:
    SFRIE1 |= WDTIE;
    __bis_SR_register(GIE);
}

int main( void )
{
    init();

    uint8_t current_button = 0;

    uint8_t pwm_level_curr = 0;
    uint8_t pwm_level_a = 0;
    uint8_t pwm_level_b = 1;
    uint8_t pwm_level_c = 2;

    while (1) {
        if (f_pwm_loop) {
            pwm_level_curr++;

            if ((1 << pwm_level_a) > pwm_level_curr)
                LEDA_PORT_OUT |= LEDA_PIN;
            else
                LEDA_PORT_OUT &= ~LEDA_PIN;

            if ((1 << pwm_level_b) > pwm_level_curr)
                LEDB_PORT_OUT |= LEDB_PIN;
            else
                LEDB_PORT_OUT &= ~LEDB_PIN;

            if ((1 << pwm_level_c) > pwm_level_curr)
                LEDC_PORT_OUT |= LEDC_PIN;
            else
                LEDC_PORT_OUT &= ~LEDC_PIN;

            if (pwm_level_curr > PWM_CYCLES)
                pwm_level_curr = 0;

            f_pwm_loop = 0;
        }

        if (f_button_poll) {
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

            f_button_poll = 0;
        }

        if (s_button & BUTTON_PRESS_J1) {
            pwm_level_a++;
            if (pwm_level_a == PWM_LEVELS)
                pwm_level_a = 0;
            s_button &= ~BUTTON_PRESS_J1;
        }
        if (s_button & BUTTON_PRESS_J2) {
            pwm_level_b++;
            if (pwm_level_b == PWM_LEVELS)
                pwm_level_b = 0;
            s_button &= ~BUTTON_PRESS_J2;
        }
        if (s_button & BUTTON_PRESS_J3) {
            pwm_level_c++;
            if (pwm_level_c == PWM_LEVELS)
                pwm_level_c = 0;
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

        if (f_serial_phy == SERIAL_RX_DONE) {
            // We got a message!
            serial_phy_handle_rx();

            f_serial_phy = 0;
        }

        if (f_ms) {
            serial_ll_ms_tick();

            f_ms = 0;
        }

        if (f_serial_phy == SERIAL_TX_DONE) {
            f_serial_phy = 0;
        }

        if (s_serial_ll) {
            // We are now connected.
            pwm_level_a = !pwm_level_a;
            pwm_level_b = !pwm_level_b;
            pwm_level_c = !pwm_level_c;

            s_serial_ll = 0;
        }

        __bis_SR_register(LPM3_bits);
    }
}

#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void)
{
    main_loop_ticks_curr++;

    if (main_loop_ticks_curr == TICKS_PER_MS) {
        main_loop_ticks_curr = 0;

        if (serial_phy_timeout_counter && serial_phy_state) {
            serial_phy_timeout_counter--;
            if (!serial_phy_timeout_counter) {
                serial_phy_state = SERIAL_PHY_STATE_IDLE;
            }
        }

        // We poll the buttons every 2 ms.
        if (button_poll_this_ms)
            f_button_poll = 1;
        button_poll_this_ms = !button_poll_this_ms;
        f_ms = 1;
    }
    f_pwm_loop = 1;
    __bic_SR_register_on_exit(LPM3_bits);
}
