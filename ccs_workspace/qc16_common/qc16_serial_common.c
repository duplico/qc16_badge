#include <stdint.h>

#include "qc16_serial_common.h"

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
    return 1;
}
