/*
 * radar.c
 *
 *  Created on: Jul 23, 2019
 *      Author: george
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

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
uint8_t qbadges_near[BITFIELD_BYTES_QBADGE] = {0, };
uint8_t qbadges_near_curr[BITFIELD_BYTES_QBADGE] = {0, };
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
    memcpy(qbadges_near, qbadges_near_curr, BITFIELD_BYTES_QBADGE);
    memset((void *) qbadges_near_curr, 0x00, BITFIELD_BYTES_QBADGE);

    process_seconds();
    Event_post(ui_event_h, UI_EVENT_DO_SAVE);
}

uint8_t badge_seen(uint16_t id) {
    char fname[14] = {0,};
    sprintf(fname, "/badges/%d", id);

    return storage_file_exists(fname);
}

uint8_t badge_connected(uint16_t id) {
    badge_file_t badge_file = {0,};

    // file name is /badges/###

    char fname[14] = {0,};
    sprintf(fname, "/badges/%d", id);

    if (storage_file_exists(fname)) {
        // We've seen it before.
        storage_read_file(fname, (uint8_t *) &badge_file, sizeof(badge_file_t));
        if (badge_file.times_connected) {
            return 1;
        }
    }

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

// TODO: name is not guaranteed to be null-termed.
void set_badge_seen(uint16_t id, uint8_t type, uint8_t levels, char *name, uint8_t rssi) {
    if (!is_qbadge(id) || id == QBADGE_ID_MAX_UNASSIGNED)
        return;

    // If we're here, it's a qbadge.

    if (badge_near_curr(id)) {
        // Already marked this cycle.
        return;
    }

    // If this badge is not currently recorded as nearby, and it's not ourself,
    //  then go ahead and count it as nearby.
    // TODO: Make sure the use of badge_near and badge_near_curr is correct:
    if (!badge_near_curr(id) && id != badge_conf.badge_id) {
        set_id_buf(id, (uint8_t *) qbadges_near_curr);
        qbadges_near_count_running++;
        if (qbadges_near_count_running > qbadges_near_count) {
            qbadges_near_count = qbadges_near_count_running;
            Event_post(ui_event_h, UI_EVENT_HUD_UPDATE);
        }
    } else {
        // Already marked. We can leave...
        return;
    }

    // If we're here, it's the first time we've seen this badge this cycle.
    //  Check whether we should update its file.

    badge_file_t badge_file = {0,};

    // Let's update its details/name in our records.
    // file name is /qbadges/###

    char fname[14] = {0,};
    sprintf(fname, "/badges/%d", id);

    uint8_t write_file = 0;

    if (storage_file_exists(fname)) {
        // We've seen it before.
        storage_read_file(fname, (uint8_t *) &badge_file, sizeof(badge_file_t));
    } else {
        // We've never seen it before.
        badge_conf.stats.qbadges_seen_count++;
        if (id > badge_conf.stats.qbadges_in_system) {
            badge_conf.stats.qbadges_in_system = id+1;
        }
        write_file = 1;
        Event_post(ui_event_h, UI_EVENT_DO_SAVE);
    }

    // TODO: Check whether it's uber or handler now, and didn't used to be.
    // TODO: How's the performance on this?


    if (badge_file.badge_type != type) {
        write_file = 1;
        badge_file.badge_type = type;
        if (type & BADGE_TYPE_HANDLER_MASK) {
            badge_conf.stats.qbadges_handler_seen_count++;
            if (badge_conf.stats.qbadges_handler_seen_count > badge_conf.stats.qbadge_handlers_in_system) {
                badge_conf.stats.qbadge_handlers_in_system++;
                Event_post(ui_event_h, UI_EVENT_DO_SAVE);
            }
        }
        if (type & BADGE_TYPE_UBER_MASK) {
            badge_conf.stats.qbadges_uber_seen_count++;
            if (badge_conf.stats.qbadges_uber_seen_count > badge_conf.stats.qbadge_ubers_in_system) {
                badge_conf.stats.qbadge_ubers_in_system++;
                Event_post(ui_event_h, UI_EVENT_DO_SAVE);
            }
        }
    }

    badge_file.badge_id = id;
    if (badge_file.levels != levels) {
        write_file = 1;
        badge_file.levels = levels;
    }

    if (strncmp(badge_file.handle, name, QC16_BADGE_NAME_LEN)) {
        write_file = 1;
        strncpy(badge_file.handle, name, QC16_BADGE_NAME_LEN);
        badge_file.handle[QC16_BADGE_NAME_LEN] = 0x00;
    }

    if (write_file) {
        storage_overwrite_file(fname, (uint8_t *) &badge_file, sizeof(badge_file_t));
    }

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

    // TODO: This is the wrong way:
//    if (is_qbadge(id)) {
//        set_id_buf(id, (uint8_t *)qbadges_connected);
//        badge_conf.stats.qbadges_connected_count++;
//    } else if (is_cbadge(id)) {
//        set_id_buf(id, (uint8_t *)cbadges_connected);
//        badge_conf.stats.cbadges_connected_count++;
//    }

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
