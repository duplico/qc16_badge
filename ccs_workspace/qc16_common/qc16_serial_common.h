#ifndef QC16_SERIAL_COMMON_H_
#define QC16_SERIAL_COMMON_H_

#include <stdint.h>

// Configuration
#define PTX_TIME_MS 1000
#define PRX_TIME_MS 1000

// Serial protocol details
#define QC16_CRC_SEED 0xB68F
#define SERIAL_PHY_SYNC_WORD 0xAC

#define SERIAL_OPCODE_HELO      0x01
#define SERIAL_OPCODE_ACK       0x02
#define SERIAL_OPCODE_BTN_J1    0x03
#define SERIAL_OPCODE_BTN_J2    0x04
#define SERIAL_OPCODE_BTN_J3    0x05

#define SERIAL_ID_ANY 0xffff

// Serial LL (link-layer) state machine states:
// TODO: rename to SERIAL_LL_...
#define SERIAL_MODE_NC_PRX 0
#define SERIAL_MODE_NC_PTX 1
#define SERIAL_MODE_C_IDLE 2

// Shared struct and functions:

typedef struct {
    __packed uint8_t opcode;
    __packed uint8_t payload_len;
    uint16_t from_id;
    uint16_t to_id;
    uint16_t crc16_payload;
    uint16_t crc16_header;
} serial_header_t;

uint16_t crc16_buf(volatile uint8_t *sbuf, uint8_t len);
uint16_t crc_build(uint8_t data, uint8_t start_over);

#endif /* QC16_SERIAL_COMMON_H_ */
