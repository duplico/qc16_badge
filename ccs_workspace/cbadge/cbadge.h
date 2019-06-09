/*
 * cbadge.h
 *
 *  Created on: Jun 7, 2019
 *      Author: george
 */

#ifndef CBADGE_H_
#define CBADGE_H_

#define KEY_LVL 190 //125

#define LEDA_PORT_OUT P2OUT
#define LEDA_PIN BIT7
#define LEDB_PORT_OUT P2OUT
#define LEDB_PIN BIT0
#define LEDC_PORT_OUT P1OUT
#define LEDC_PIN BIT0

#define BUTTON_PRESS_J1 BIT0
#define BUTTON_PRESS_J2 BIT1
#define BUTTON_PRESS_J3 BIT2
#define BUTTON_RELEASE_J1 BIT4
#define BUTTON_RELEASE_J2 BIT5
#define BUTTON_RELEASE_J3 BIT6

extern uint8_t s_button;

#endif /* CBADGE_H_ */
