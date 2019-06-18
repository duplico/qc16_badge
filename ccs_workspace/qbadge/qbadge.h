/*
 * qc16.h
 *
 *  Created on: Jun 5, 2019
 *      Author: george
 */

#ifndef QBADGE_H_
#define QBADGE_H_

#include <stdint.h>

#define UI_AUTOREFRESH_TIMEOUT 6000000
#define UI_CLOCK_MS 15 // config

#define SPIFFS_LOGICAL_BLOCK_SIZE    (4096)
#define SPIFFS_LOGICAL_PAGE_SIZE     (256)
#define SPIFFS_FILE_DESCRIPTOR_SIZE  (44)

extern uint_fast16_t vbat_out_uvolts;

#endif /* QBADGE_H_ */
