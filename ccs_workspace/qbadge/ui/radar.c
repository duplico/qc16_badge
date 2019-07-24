/*
 * radar.c
 *
 *  Created on: Jul 23, 2019
 *      Author: george
 */
#include <stdint.h>
#include <stdlib.h>

#include <xdc/runtime/Error.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/family/arm/cc26xx/Seconds.h>

#include <third_party/spiffs/spiffs.h>

#include <qc16.h>
#include <qbadge.h>

#include "queercon_drivers/storage.h"
#include <ui/graphics.h>
#include <ui/images.h>
#include <ui/ui.h>
#include <ui/leds.h>
#include <qc16_serial_common.h>
#include "badge.h"

// Radar constructs:
uint32_t qbadges_near[QBADGE_BITFIELD_LONGS] = {0, };
uint32_t qbadges_near_curr[QBADGE_BITFIELD_LONGS] = {0, };
uint32_t qbadges_seen[QBADGE_BITFIELD_LONGS] = {0, };
uint32_t qbadges_connected[QBADGE_BITFIELD_LONGS] = {0, };
uint32_t cbadges_connected[CBADGE_BITFIELD_LONGS] = {0, };
uint16_t qbadges_near_count=0;
uint16_t qbadges_near_count_running=0;

Clock_Handle radar_clock_h;

uint8_t handler_nearby() {
    return 0;
}

void reset_scan_cycle(UArg a0) {
    if (qbadges_near_count_running != qbadges_near_count) {
        qbadges_near_count = qbadges_near_count_running;
        Event_post(ui_event_h, UI_EVENT_HUD_UPDATE);
    }
    qbadges_near_count_running = 0;
    memset((void *) qbadges_near, 0x00, 4*QBADGE_BITFIELD_LONGS);

    process_seconds();
    Event_post(ui_event_h, UI_EVENT_DO_SAVE);
}

uint8_t badge_seen(uint16_t id) {
    // TODO: SPIFFS instead
    if (is_qbadge(id) && check_id_buf(id, (uint8_t *) qbadges_seen))
        return 1;
    return 0;
}

uint8_t badge_connected(uint16_t id) {
    // TODO: SPIFFS instead
    if (is_cbadge(id) && check_id_buf(id, (uint8_t *)cbadges_connected))
        return 1;
    if (is_qbadge(id) && check_id_buf(id, (uint8_t *)qbadges_connected))
        return 1;
    return 0;
}

uint8_t badge_near(uint16_t id) {
    if (is_qbadge(id) && check_id_buf(id, (uint8_t *) qbadges_near))
        return 1;
    return 0;
}

uint8_t badge_near_curr(uint16_t id) {
    if (is_qbadge(id) && check_id_buf(id, (uint8_t *) qbadges_near_curr))
        return 1;
    return 0;
}

// TODO: Also pass the payload:
void set_badge_seen(uint16_t id, char *name) {
    if (!is_qbadge(id) || id == QBADGE_ID_MAX_UNASSIGNED)
        return;

    // If we're here, it's a qbadge.

    if (badge_near_curr(id)) {
        // Already marked this cycle.
        return;
    }

    // Mark this badge as "nearby"
    if (!badge_near(id) && id != badge_conf.badge_id) {
        set_id_buf(id, (uint8_t *) qbadges_near);
        qbadges_near_count_running++;
        if (qbadges_near_count_running > qbadges_near_count) {
            qbadges_near_count = qbadges_near_count_running;
            Event_post(ui_event_h, UI_EVENT_HUD_UPDATE);
        }
    }

    // Let's update its details/name in our records.

    if (id == QBADGE_ID_MAX_UNASSIGNED || id == CBADGE_ID_MAX_UNASSIGNED)
            return;
    if (badge_seen(id)) {
        return;
    }

    set_id_buf(id, (uint8_t *)qbadges_seen);
    badge_conf.stats.qbadges_seen_count++;

    Event_post(ui_event_h, UI_EVENT_DO_SAVE);
    return;
}

uint8_t set_badge_connected(uint16_t id, char *handle) {
    if (!is_qbadge(id) && !is_cbadge(id))
        return 0;

    if (id == QBADGE_ID_MAX_UNASSIGNED || id == CBADGE_ID_MAX_UNASSIGNED)
        return 0;

    if (badge_connected(id)) {
        return 0;
    }

    if (is_qbadge(id)) {
        set_id_buf(id, (uint8_t *)qbadges_connected);
        badge_conf.stats.qbadges_connected_count++;
    } else if (is_cbadge(id)) {
        set_id_buf(id, (uint8_t *)cbadges_connected);
        badge_conf.stats.cbadges_connected_count++;
    }

    Event_post(ui_event_h, UI_EVENT_DO_SAVE);
    return 1;
}

void radar_init() {
    Clock_Params clockParams;
    Error_Block eb;
    Error_init(&eb);

    Clock_Params_init(&clockParams);
    clockParams.period = SCAN_PERIOD_SECONDS * 100000;
    clockParams.startFlag = TRUE;
    radar_clock_h = Clock_create(reset_scan_cycle, clockParams.period, &clockParams, &eb);
}
