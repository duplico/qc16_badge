#include <stdint.h>
#include <string.h>

#include <msp430fr2111.h>

#include <qc16.h>

#include <cbadge.h>
#include <cbadge_serial.h>

#include "buttons.h"

volatile uint8_t main_loop_ticks_curr=0;
volatile uint8_t button_poll_this_ms = 0;

/// Signal indicating that the badge is self-powered for the first time.
uint8_t s_activated;
/// Signal for all captouch button events.
uint8_t s_button;
/// Signal for the serial link-layer connection event.
uint8_t s_connected;
/// Signal that this badge is paired with a qbadge.
uint8_t s_paired;
/// Signal that element ID s_level_up-1 has leveled up!
uint8_t s_level_up;
/// Signal to do the next step in our current animation.
uint8_t s_animation_step;
/// Interrupt flag for the serial PHY
volatile uint8_t f_serial_phy;
/// Interrupt flag indicating it's time to poll a captouch button.
volatile uint8_t f_button_poll;
/// Interrupt flag indicating it's time to do another PWM time step.
volatile uint8_t f_pwm_loop;
/// Interrupt flag indicating 1 ms has passed.
volatile uint8_t f_ms;

/// The brightness level of each light
uint8_t pwm_levels[3];

uint16_t animation_ms_remaining;
uint16_t animation_step_ms;
uint16_t animation_step_curr_ms;
uint8_t animation_type;
uint8_t animation_type_prev;

cbadge_conf_t badge_conf;
#pragma PERSISTENT(badge_conf_persistent)
cbadge_conf_t badge_conf_persistent = {0,};

#pragma PERSISTENT(qbadges_connected)
uint8_t qbadges_connected[BITFIELD_BYTES_QBADGE] = {0,};

/// This cbadge is currently running under its own power.
uint8_t badge_active;

pair_payload_t paired_badge;

uint8_t mining_progress[3];
uint16_t mining_ms;

/// Initialize clock signals and the three system clocks.
/**
 ** We'll take the DCO to 8 MHz, and divide it by 1 for MCLK.
 ** Then we'll divide MCLK by 1 to get 8 MHz SMCLK.
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
    //  Set to DCO/1 = 8 MHz
    CSCTL5 |= DIVM__1;

    // SMCLK (1 MHz)
    //  Derived from MCLK with divider up to /8
    //  Set to MCLK/1 = 8 MHz
    CSCTL5 |= DIVS__1;
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
    // P1.0     LED B       (SEL 00; DIR 1)
    // P1.1     B2B PTX     (SEL 00; DIR 0) (pull-down) (DIO1)
    // P1.2     RX (PTX)    (SEL 01; DIR 0)
    // P1.3     TX (PTX)    (SEL 01; DIR 1)
    // P1.4     B2B PRX     (SEL 00; DIR 0) (pull-down) (DIO2)
    // P1.5     B1          (SEL 00; DIR 0)
    // P1.6     RX (PRX)   (SEL 01; DIR 0)
    // P1.7     TX (PRX)   (SEL 01; DIR 1)
    // (PRX is the default config)
    P1DIR = 0b10001001;
    P1SEL0 = 0b11001100; // LSB
    P1SEL1 = 0b00000000; // MSB
    P1REN = 0b00010010;
    P1OUT = 0x00;
    // P2.0     LED C       (SEL: 00; DIR 1)
    // P2.1     B2          (SEL: 00; DIR 0)
    //  ~~(There are no P.{2..5})~~
    // P2.6     B3          (SEL: 00; DIR 0)
    // P2.7     LED A       (SEL: 00; DIR 1)
    P2DIR = 0b10000001;
    P2SEL0 = 0x00;
    P2SEL1 = 0x00;
    P2OUT = 0x00;
}

void write_conf() {
    __bic_SR_register(GIE);
    // Unlock FRAM access:
    SYSCFG0 = FRWPPW | PFWP_0;
    memcpy(&badge_conf_persistent, &badge_conf, sizeof(cbadge_conf_t));
    // Lock FRAM access:
    SYSCFG0 = FRWPPW | PFWP_1;
    __bis_SR_register(GIE);
}

void generate_config() {
    // We treat this like FIRST BOOT. Need to initialize the config.
    // The run time initializes badge_conf to all 0s for us.
    badge_conf.badge_id = CBADGE_ID_MAX_UNASSIGNED;
    badge_conf.initialized = 1;
    badge_conf.element_selected = ELEMENT_COUNT_NONE;
    memcpy(badge_conf.handle, "cb", 3);
    write_conf();
}

void init_conf() {
    if (!badge_conf_persistent.initialized) {
        // Need to generate a config.
        generate_config();
    } else {
        memcpy(&badge_conf, &badge_conf_persistent, sizeof(cbadge_conf_t));
        if (!badge_conf.in_service && badge_conf.badge_id != CBADGE_ID_MAX_UNASSIGNED) {
            // If we have a real ID, or we've plugged into a few qbadges,
            //  we're going to go ahead and say this is the config that
            //  we'll run with.
            badge_conf.in_service = 1;
            write_conf();
        }
    }
}

void set_display_type(uint8_t dest_type) {
    pwm_levels[0] = 0;
    pwm_levels[1] = 0;
    pwm_levels[2] = 0;
    animation_step_curr_ms = 0;
    animation_step_ms = 0; // a "non" animation
    animation_ms_remaining=3000;

    // If the destination is a temp animation, and the previous is not,
    //  then save the previous one.
    if (dest_type >= 0x10 && animation_type < 0x10) {
        // Only store the previous animation type
        //  if it's an indefinite animation.
        animation_type_prev = animation_type;
    }
    animation_type = dest_type;

    switch(animation_type) {
    case DISPLAY_OFF:
        // Nothing to do.
        break;
    case DISPLAY_NEWPAIR_ACTIVATED:
        // Scroll!
        animation_step_ms = 75;
    case DISPLAY_ON:
        pwm_levels[0] = PWM_LEVELS-1;
        pwm_levels[1] = PWM_LEVELS-1;
        pwm_levels[2] = PWM_LEVELS-1;
        break;
    case DISPLAY_ELEMENT:
        animation_step_ms = 75;
        break;
    case DISPLAY_MINING:
        animation_step_ms = 200;
        break;
    case DISPLAY_LEVELUP:
        animation_step_ms=20;
        break;
    case DISPLAY_GOMISSION:
        // Blink!
        animation_step_ms=150;
        break;
    }

    s_animation_step = 1;
}

/// Mark a badge as connected, returning 1 if this is the first time doing so.
uint8_t set_badge_connected(uint16_t id) {
    if (!is_qbadge(id)) {
        return 0;
    }

    badge_conf.stats.qbadges_connected_count++;
    // Save, and return 1 for new badge found.

    write_conf();
    set_display_type(DISPLAY_NEWPAIR_ACTIVATED);
    return 1;
}

/// Perform basic initialization of the cbadge.
void init() {
    // Stop the watchdog timer.
    WDTCTL = WDTPW | WDTCNTCL;

    init_conf();
    init_clocks();
    init_io();
    init_serial();

    // Just plugged in (or put in batteries).
    // By default the display will be off.

    animation_type = !badge_conf.in_service;

    // Set up the WDT to do our time loop.
    WDTCTL = TICK_WDT_BITS;
    // Enable interrupt for the WDT:
    SFRIE1 |= WDTIE;
    __bis_SR_register(GIE);
}

// TODO: connect animation isn't working
// TODO: test level-up

int main( void )
{
    init();

    uint8_t current_button = 0;
    uint8_t pwm_level_curr = 0;

    while (1) {
        if (s_activated) {
            set_display_type(DISPLAY_MINING);
            set_display_type(DISPLAY_NEWPAIR_ACTIVATED);
            s_activated = 0;
        }

        if (f_pwm_loop) {
            pwm_level_curr++;

            if ((1 << pwm_levels[0]) > pwm_level_curr)
                LEDC_PORT_OUT |= LEDC_PIN;
            else
                LEDC_PORT_OUT &= ~LEDC_PIN;

            if ((1 << pwm_levels[1]) > pwm_level_curr)
                LEDA_PORT_OUT |= LEDA_PIN;
            else
                LEDA_PORT_OUT &= ~LEDA_PIN;

            if ((1 << pwm_levels[2]) > pwm_level_curr)
                LEDB_PORT_OUT |= LEDB_PIN;
            else
                LEDB_PORT_OUT &= ~LEDB_PIN;

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

        if (s_button & BUTTON_PRESS_COCKTAILS_J1) { // cocktails
            if (badge_conf.element_selected == ELEMENT_COCKTAILS) {
                badge_conf.element_selected = ELEMENT_COUNT_NONE;
            } else {
                badge_conf.element_selected = ELEMENT_COCKTAILS;
            }
        }
        if (s_button & BUTTON_PRESS_FLAGS_J2) { // flags
            if (badge_conf.element_selected == ELEMENT_FLAGS) {
                badge_conf.element_selected = ELEMENT_COUNT_NONE;
            } else {
                badge_conf.element_selected = ELEMENT_FLAGS;
            }
        }
        if (s_button & BUTTON_PRESS_KEYS_J3) { // keys
            if (badge_conf.element_selected == ELEMENT_KEYS) {
                badge_conf.element_selected = ELEMENT_COUNT_NONE;
            } else {
                badge_conf.element_selected = ELEMENT_KEYS;
            }
        }

        if (s_button) {
            if (serial_ll_state == SERIAL_LL_STATE_C_PAIRED) {
                // Send our updated element.
                serial_element_update();
                set_display_type(DISPLAY_ELEMENT);
            } else if (badge_active && badge_conf.element_selected != ELEMENT_COUNT_NONE) {
                set_display_type(DISPLAY_MINING);
            } else {
                set_display_type(DISPLAY_OFF);
            }

            s_button = 0;
        }

        if (f_serial_phy == SERIAL_RX_DONE) {
            // We got a message!
            serial_phy_handle_rx();

            f_serial_phy = 0;
        }

        if (f_ms) {
            serial_ll_ms_tick();

            if (animation_step_ms) {
                // Doing an animation
                animation_step_curr_ms++;
                if (animation_step_curr_ms >= animation_step_ms) {
                    animation_step_curr_ms = 0;
                    s_animation_step = 1;
                }

                if (animation_type >= 0x10) {
                    animation_ms_remaining--;
                    if (!animation_ms_remaining) {
                        set_display_type(animation_type_prev);
                    }
                }

            }

            if (badge_active && badge_conf.element_selected != ELEMENT_COUNT_NONE) {
                mining_ms++;
                if (mining_ms >= 1000) {
                    // Been a second, time to make progress.
                    mining_progress[badge_conf.element_selected-3] += 1 + badge_conf.stats.cbadges_connected_count / 128;

                    if (mining_progress[badge_conf.element_selected-3] > 100) {
                        set_display_type(DISPLAY_LEVELUP);
                        badge_conf.element_qty[badge_conf.element_selected-3]++;
                        mining_progress[badge_conf.element_selected-3] = 0;
                        write_conf();
                    }
                    mining_ms = 0;
                }
            }

            f_ms = 0;
        }

        if (f_serial_phy == SERIAL_TX_DONE) {
            f_serial_phy = 0;
        }

        if (s_animation_step) {
            switch(animation_type) {
            case DISPLAY_LEVELUP:
            case DISPLAY_MINING:
            case DISPLAY_ELEMENT:
                if (badge_conf.element_selected - 3 > 2) {
                    break;
                }
                pwm_levels[badge_conf.element_selected-3]++;
                if (pwm_levels[badge_conf.element_selected-3] >= PWM_LEVELS) {
                    pwm_levels[badge_conf.element_selected-3] = 0;
                }
                break;
            case DISPLAY_GOMISSION:
                pwm_levels[0] = pwm_levels[0] ? 0 : PWM_LEVELS - 1;
                pwm_levels[1] = pwm_levels[0];
                pwm_levels[2] = pwm_levels[0];
                break;
            case DISPLAY_NEWPAIR_ACTIVATED:
                if (pwm_levels[0]) {
                    pwm_levels[0] = 0;
                    pwm_levels[1] = PWM_LEVELS-1;
                    pwm_levels[2] = 0;
                } else if (pwm_levels[1]) {
                    pwm_levels[0] = 0;
                    pwm_levels[1] = 0;
                    pwm_levels[2] = PWM_LEVELS-1;
                } else {
                    pwm_levels[0] = PWM_LEVELS-1;
                    pwm_levels[1] = 0;
                    pwm_levels[2] = 0;
                }
                break;
            }

            s_animation_step = 0;
        }

        if (s_connected) {
            // We are now connected.
            // This is the new idle animation:

            // Are we connected to a qbadge? (That's the one that we pair with)
            //  And, if so, do we happen to be configured as the PTX?
            if (serial_phy_mode_ptx && is_qbadge(connected_badge_id)) {
                // The PTX is the side that sends the pairing message
                set_display_type(DISPLAY_OFF);
                serial_pair();
            } else if (is_cbadge(connected_badge_id)) {
                set_display_type(DISPLAY_NEWPAIR_ACTIVATED);
            }

            s_connected = 0;
        }

        if (s_paired) {
            badge_conf.element_selected = ELEMENT_COUNT_NONE;
            s_paired = 0;
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
