/*
 * qc16.h
 *
 *  Created on: Jun 5, 2019
 *      Author: george
 */

#ifndef QBADGE_H_
#define QBADGE_H_

#include <stdint.h>

#include <ti/sysbios/knl/Event.h>

#include <qc16.h>

// App-level configuration elements:
#define UI_AUTOREFRESH_TIMEOUT 6000000
#define UI_CLOCK_MS 15
#define ADC_INTERVAL 125000
#define UI_TEXT_FONT g_sFontCmss16
#define BRIGHTNESS_LEVEL_SIDELIGHTS_THRESH 1
#define TEXTENTRY_MAX_LEN 40

#define SCAN_PERIOD_SECONDS 45

#define VHANDLER_COOLDOWN_MIN_SECONDS  300
#define VHANDLER_COOLDOWN_MAX_SECONDS 9000
#define VHANDLER_MAX_LEVEL 3
#define VHANDLER_SECOND_ELEMENT_ONE_IN 3

#define MISSION_DURATION_MIN_SECONDS 600
#define MISSION_DURATION_MAX_SECONDS 1000

// UBLE stuff:
#define UBLE_EVENT_UPDATE_ADV   Event_Id_00
extern Event_Handle uble_event_h;

// Hardware/driver-level configuration:

#define SPIFFS_LOGICAL_BLOCK_SIZE    (4096)
#define SPIFFS_LOGICAL_PAGE_SIZE     (256)
#define SPIFFS_FILE_DESCRIPTOR_SIZE  (44)

extern uint_fast16_t vbat_out_uvolts;

#endif /* QBADGE_H_ */
