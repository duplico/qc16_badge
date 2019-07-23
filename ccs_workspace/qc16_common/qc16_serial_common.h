#ifndef QC16_SERIAL_COMMON_H_
#define QC16_SERIAL_COMMON_H_

#include <stdint.h>
#include <qc16.h>

// Configuration
#define PTX_TIME_MS 100
#define PRX_TIME_MS 1000
#define SERIAL_C_DIO_POLL_MS 10

// Serial protocol details
#define QC16_CRC_SEED 0xB68F
#define SERIAL_PHY_SYNC_WORD 0xAC

#define SERIAL_OPCODE_HELO      0x01
#define SERIAL_OPCODE_ACK       0x02
#define SERIAL_OPCODE_ELEMENT   0x03
#define SERIAL_OPCODE_PUTFILE   0x09
#define SERIAL_OPCODE_APPFILE   0x0A
#define SERIAL_OPCODE_ENDFILE   0x0B
#define SERIAL_OPCODE_SETID     0x0C
#define SERIAL_OPCODE_SETNAME   0x0D
#define SERIAL_OPCODE_DUMPQ     0x0E
#define SERIAL_OPCODE_DUMPA     0x0F
#define SERIAL_OPCODE_DISCON    0x10
#define SERIAL_OPCODE_SETTYPE   0x11
#define SERIAL_OPCODE_PAIR      0x12
#define SERIAL_OPCODE_GETFILE   0x13
#define SERIAL_OPCODE_GOMISSION 0x14

#define SERIAL_ID_ANY 0xffff

// Serial LL (link-layer) state machine states:
#define SERIAL_LL_STATE_NC_PRX 0
#define SERIAL_LL_STATE_NC_PTX 1
#define SERIAL_LL_STATE_C_IDLE 2
#define SERIAL_LL_STATE_C_FILE_RX 3
#define SERIAL_LL_STATE_C_FILE_TX 4
#define SERIAL_LL_STATE_C_PAIRING 5
#define SERIAL_LL_STATE_C_PAIRED 6

// Shared struct and functions:

typedef struct {
    __packed uint8_t opcode;
    __packed uint8_t payload_len;
    uint16_t from_id;
    uint16_t to_id;
    uint16_t crc16_payload;
    uint16_t crc16_header;
} serial_header_t;

typedef struct {
    __packed uint16_t badge_id;
    __packed uint8_t badge_type;
    __packed uint8_t element_level[6];
    __packed uint8_t element_level_max[6];
    __packed uint8_t element_level_progress[6];
    __packed uint32_t element_qty[6];
    __packed uint32_t last_clock;
    __packed uint8_t clock_is_set;
    __packed uint8_t agent_present;
    __packed element_type element_selected;
    __packed mission_t missions[3];
    __packed uint8_t mission_assigned[3];
    __packed char handle[QC16_BADGE_NAME_LEN + 1];
} pair_payload_t;

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
