/*
 * serial.h
 *
 *  Created on: Jun 9, 2019
 *      Author: george
 */

#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdint.h>

typedef struct {
    uint16_t badge_id;
} cbadge_conf_t;

// We can store all our connections for 0.3kB.

typedef struct {
    uint8_t opcode;
    uint8_t payload_len;
    uint16_t from_id;
    uint16_t to_id;
    uint16_t crc16;
} serial_header_t;

#define SERIAL_PHY_SYNC_WORD 0xAC

#define SERIAL_PHY_STATE_IDLE 0
#define SERIAL_PHY_STATE_RX_HEADER 1
#define SERIAL_PHY_STATE_RX_PAYLOAD 2
#define SERIAL_PHY_STATE_TX_HEADER 17
#define SERIAL_PHY_STATE_TX_PAYLOAD 18

#endif /* SERIAL_H_ */
