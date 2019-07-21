/*
 * cbadge.h
 *
 *  Created on: Jun 7, 2019
 *      Author: george
 */

#ifndef CBADGE_H_
#define CBADGE_H_

#include <qc16.h>

#define TICK_WDT_BITS WDT_MDLY_0_5
#define TICKS_PER_MS 16

#define PWM_LEVELS 4
#define PWM_CYCLES (1 << (PWM_LEVELS-1))

#define KEY_LVL (1000 * TICKS_PER_MS) // Should be about 1000 per ms

#define SERIAL_PHY_TIMEOUT_MS 10

#define SERIAL_DIO_OUT P1OUT
#define SERIAL_DIO_IN P1IN
#define SERIAL_DIO_REN P1REN
#define SERIAL_DIO_DIR P1DIR
#define SERIAL_DIO1_PTX BIT1
#define SERIAL_DIO2_PRX BIT4

#define LEDC_PORT_OUT P2OUT
#define LEDC_PIN BIT7
#define LEDA_PORT_OUT P2OUT
#define LEDA_PIN BIT0
#define LEDB_PORT_OUT P1OUT
#define LEDB_PIN BIT0

#define BUTTON_PRESS_J1 BIT0
#define BUTTON_PRESS_J2 BIT1
#define BUTTON_PRESS_J3 BIT2
#define BUTTON_RELEASE_J1 BIT4
#define BUTTON_RELEASE_J2 BIT5
#define BUTTON_RELEASE_J3 BIT6

#define SERIAL_RX_DONE 1
#define SERIAL_TX_DONE 2

extern cbadge_conf_t badge_conf;
extern uint8_t s_button;
extern uint8_t s_activated;
volatile extern uint8_t f_serial_phy;
extern uint8_t s_connected;
extern uint8_t s_paired;
extern uint8_t s_level_up;

#endif /* CBADGE_H_ */
