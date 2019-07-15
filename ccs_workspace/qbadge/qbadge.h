/*
 * qc16.h
 *
 *  Created on: Jun 5, 2019
 *      Author: george
 */

#ifndef QBADGE_H_
#define QBADGE_H_

#include <stdint.h>
#include <qc16.h>

// App-level configuration elements:
#define UI_AUTOREFRESH_TIMEOUT 6000000
#define UI_CLOCK_MS 15
#define ADC_INTERVAL 12500 // TODO: rename
#define UI_TEXT_FONT g_sFontCmss16
#define BRIGHTNESS_LEVEL_SIDELIGHTS_THRESH 5

// Hardware/driver-level configuration:

#define SPIFFS_LOGICAL_BLOCK_SIZE    (4096)
#define SPIFFS_LOGICAL_PAGE_SIZE     (256)
#define SPIFFS_FILE_DESCRIPTOR_SIZE  (44)

extern uint_fast16_t vbat_out_uvolts;

#endif /* QBADGE_H_ */
