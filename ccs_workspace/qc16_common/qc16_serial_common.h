#ifndef QC16_SERIAL_COMMON_H_
#define QC16_SERIAL_COMMON_H_

typedef struct {
    uint8_t opcode;
    uint8_t payload_len;
    uint16_t from_id;
    uint16_t to_id;
    uint16_t crc16;
} serial_header_t;

#define SERIAL_PHY_SYNC_WORD 0xAC

#define SERIAL_OPCODE_HELO  0x01
#define SERIAL_OPCODE_ACK   0x02

#endif /* QC16_SERIAL_COMMON_H_ */
