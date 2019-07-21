#include <stdint.h>

#include <qc16.h>

#include "qc16_serial_common.h"

/// Calculate a 16-bit cyclic redundancy check on buffer sbuf of length len.
uint16_t crc16_buf(volatile uint8_t *sbuf, uint8_t len) {
    uint16_t crc=QC16_CRC_SEED;

    while(len){
        crc=(uint8_t)(crc >> 8) | (crc << 8);
        crc^=(uint8_t) *sbuf;
        crc^=(uint8_t)(crc & 0xff) >> 4;
        crc^=(crc << 8) << 4;
        crc^=((crc & 0xff) << 4) << 1;
        len--;
        sbuf++;
    }
    return crc;
}

uint16_t crc_build(uint8_t data, uint8_t start_over) {
    static uint16_t crc = QC16_CRC_SEED;
    if (start_over) {
        crc = QC16_CRC_SEED;
    }
    crc=(uint8_t)(crc >> 8) | (crc << 8);
    crc^=data;
    crc^=(uint8_t)(crc & 0xff) >> 4;
    crc^=(crc << 8) << 4;
    crc^=((crc & 0xff) << 4) << 1;
    return crc;
}

void crc16_header_apply(serial_header_t *header) {
    header->crc16_header = crc16_buf((uint8_t *) header, sizeof(serial_header_t) - sizeof(header->crc16_header));
}

uint8_t validate_header(serial_header_t *header) {
    if (crc16_buf((uint8_t *) header, sizeof(serial_header_t) - sizeof(header->crc16_header)) != header->crc16_header) {
        // Bad header CRC.
        return 0;
    }

    if (header->payload_len > SERIAL_BUFFER_LEN) {
        return 0;
    }

    return 1;
}

/// Given a standard buffer of bitfields, check whether ``id``'s bit is set.
uint8_t check_id_buf(uint16_t id, uint8_t *buf) {
    uint8_t byte;
    uint8_t bit;
    byte = id / 8;
    bit = id % 8;
    return (buf[byte] & (0x01 << bit)) ? 1 : 0;
}

/// In a standard buffer of bitfields, set ``id``'s bit.
void set_id_buf(uint16_t id, uint8_t *buf) {
    uint8_t byte;
    uint8_t bit;
    byte = id / 8;
    bit = id % 8;
    buf[byte] |= (0x01 << bit);
}

/// Counts the bits set in a byte and return the total.
/**
 ** This is the Brian Kernighan, Peter Wegner, and Derrick Lehmer way of
 ** counting bits in a bitstring. See _The C Programming Language_, 2nd Ed.,
 ** Exercise 2-9; or _CACM 3_ (1960), 322.
 */
uint8_t byte_rank(uint8_t v) {
    uint8_t c;
    for (c = 0; v; c++) {
        v &= v - 1; // clear the least significant bit set
    }
    return c;
}

/// Counts the bits set in all the bytes of a buffer and returns it.
/**
 ** This is the Brian Kernighan, Peter Wegner, and Derrick Lehmer way of
 ** counting bits in a bitstring. See _The C Programming Language_, 2nd Ed.,
 ** Exercise 2-9; or _CACM 3_ (1960), 322.
 */
uint16_t buffer_rank(uint8_t *buf, uint8_t len) {
    uint16_t count = 0;
    uint8_t c, v;
    for (uint8_t i=0; i<len; i++) {
        v = buf[i];
        for (c = 0; v; c++) {
            v &= v - 1; // clear the least significant bit set
        }
        count += c;
    }
    return count;
}

uint8_t is_qbadge(uint16_t id) {
    if (id >= QBADGE_ID_START && id <= QBADGE_ID_MAX_UNASSIGNED) {
        return 1;
    }
    return 0;
}

uint8_t is_cbadge(uint16_t id) {
    if (id >= CBADGE_ID_START && id < CBADGE_ID_START + CBADGES_IN_SYSTEM) {
        return 1;
    } else if (id == CBADGE_ID_MAX_UNASSIGNED) {
        return 1;
    }
    return 0;
}

