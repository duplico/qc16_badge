/*
 * cbadge.h
 *
 *  Created on: Jun 7, 2019
 *      Author: george
 */

#ifndef CBADGE_H_
#define CBADGE_H_

#define TICK_WDT_BITS WDT_MDLY_0_5
#define TICKS_PER_MS 2

#define KEY_LVL (1000 * TICKS_PER_MS) // Should be about 1000 per ms

#define SERIAL_BUFFER_LEN 32
// Currently ticks are about 2ms:
// And, at 9600 baud a header takes about 7ms to send.
#define SERIAL_TIMEOUT_TICKS 10

typedef struct {
    uint16_t badge_id;
    uint8_t active;
    uint8_t activated;
    uint8_t initialized;
} cbadge_conf_t;

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

#define SERIAL_RX_DONE 1
#define SERIAL_TX_DONE 2

extern cbadge_conf_t my_conf;
extern uint8_t s_button;
volatile extern uint8_t f_serial;
extern uint8_t s_connected;

#endif /* CBADGE_H_ */
