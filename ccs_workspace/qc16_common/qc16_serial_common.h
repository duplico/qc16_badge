#ifndef QC16_SERIAL_COMMON_H_
#define QC16_SERIAL_COMMON_H_

#include <stdint.h>

// Configuration
// TODO: it appears we need to up the bitrate?
#define PTX_TIME_MS 100 // TODO: shorten?
#define PRX_TIME_MS 1000

// Serial protocol details
#define QC16_CRC_SEED 0xB68F
#define SERIAL_PHY_SYNC_WORD 0xAC

#define SERIAL_OPCODE_HELO      0x01
#define SERIAL_OPCODE_ACK       0x02
#define SERIAL_OPCODE_BTN_J1    0x03
#define SERIAL_OPCODE_BTN_J2    0x04
#define SERIAL_OPCODE_BTN_J3    0x05
#define SERIAL_OPCODE_BTN_F1    0x06
#define SERIAL_OPCODE_BTN_F2    0x07
#define SERIAL_OPCODE_BTN_F3    0x08
#define SERIAL_OPCODE_PUTFILE   0x09
#define SERIAL_OPCODE_APPFILE   0x0A
#define SERIAL_OPCODE_ENDFILE   0x0B
#define SERIAL_OPCODE_SETID     0x0C
#define SERIAL_OPCODE_SETNAME   0x0D
#define SERIAL_OPCODE_DUMPQ     0x0E
#define SERIAL_OPCODE_DUMPA     0x0F
#define SERIAL_OPCODE_DISCON    0x10

#define SERIAL_ID_ANY 0xffff

// Serial LL (link-layer) state machine states:
#define SERIAL_LL_STATE_NC_PRX 0
#define SERIAL_LL_STATE_NC_PTX 1
#define SERIAL_LL_STATE_C_IDLE 2
#define SERIAL_LL_STATE_C_FILE_RX 3
#define SERIAL_LL_STATE_C_FILE_TX 4

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
void crc16_header_apply(serial_header_t *header);
uint8_t validate_header(serial_header_t *header);
uint8_t is_cbadge(uint16_t id);
uint8_t is_qbadge(uint16_t id);
uint8_t check_id_buf(uint16_t id, uint8_t *buf);
void set_id_buf(uint16_t id, uint8_t *buf);
uint8_t byte_rank(uint8_t v);
uint16_t buffer_rank(uint8_t *buf, uint8_t len);

#endif /* QC16_SERIAL_COMMON_H_ */
