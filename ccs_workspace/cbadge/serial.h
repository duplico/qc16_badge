/*
 * serial.h
 *
 *  Created on: Jun 9, 2019
 *      Author: george
 */

#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdint.h>

#include "cbadge.h"
#include "qc16_serial_common.h"

// We can store all our connections for 0.3kB.

#define SERIAL_PHY_STATE_IDLE 0
#define SERIAL_PHY_STATE_RX_HEADER 1
#define SERIAL_PHY_STATE_RX_PAYLOAD 2
#define SERIAL_PHY_STATE_TX_HEADER 17
#define SERIAL_PHY_STATE_TX_PAYLOAD 18

extern volatile uint8_t serial_phy_state;
extern volatile uint8_t serial_active_ms;
extern volatile serial_header_t serial_header_in;
extern serial_header_t serial_header_out;

void serial_phy_handle_rx();
void init_serial();

#endif /* SERIAL_H_ */
