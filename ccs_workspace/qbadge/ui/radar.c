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

uint16_t handler_near_id = QBADGE_ID_MAX_UNASSIGNED;
uint8_t handler_near_rssi = 0;
element_type handler_near_element = ELEMENT_COUNT_NONE;
char handler_near_handle[QC16_BADGE_NAME_LEN+1] = {0,};

uint16_t handler_near_id_curr = QBADGE_ID_MAX_UNASSIGNED;
uint8_t handler_near_rssi_curr = 0;
element_type handler_near_element_curr = ELEMENT_COUNT_NONE;
char handler_near_handle_curr[QC16_BADGE_NAME_LEN+1] = {0,};

uint8_t dcfurs_nearby = 0;
uint8_t dcfurs_nearby_curr = 0;

Clock_Handle radar_clock_h;

uint8_t handler_human_nearby() {
    if (!badge_conf.handler_allowed) {
        return 0;
    }
    return handler_near_id != QBADGE_ID_MAX_UNASSIGNED;
}

void reset_scan_cycle(UArg a0) {
    if (vbat_out_uvolts && vbat_out_uvolts < UVOLTS_EXTPOWER) { // 50 mV
        // We're on external power.
    } else if (vbat_out_uvolts && vbat_out_uvolts < UVOLTS_CUTOFF) {
        // Batteries are below cut-off voltage
        // Do nothing.
        return;
    }

    if (qbadges_near_count_running != qbadges_near_count) {
        qbadges_near_count = qbadges_near_count_running;
        Event_post(ui_event_h, UI_EVENT_HUD_UPDATE);
    }
    qbadges_near_count_running = 0;
    memcpy(qbadges_near, qbadges_near_curr, BITFIELD_BYTES_QBADGE);
    memset((void *) qbadges_near_curr, 0x00, BITFIELD_BYTES_QBADGE);

    if (handler_near_id_curr != handler_near_id) {
        Event_post(ui_event_h, UI_EVENT_HUD_UPDATE);
    }

    handler_near_id = handler_near_id_curr;
    handler_near_rssi = handler_near_rssi_curr;
    handler_near_element = handler_near_element_curr;
    strncpy(handler_near_handle, handler_near_handle_curr, QC16_BADGE_NAME_LEN);
    handler_near_handle[QC16_BADGE_NAME_LEN] = 0x00;

    handler_near_id_curr = QBADGE_ID_MAX_UNASSIGNED;
    handler_near_element_curr = ELEMENT_COUNT_NONE;

    dcfurs_nearby = dcfurs_nearby_curr;
    dcfurs_nearby_curr = 0;

    process_seconds();
    Event_post(ui_event_h, UI_EVENT_DO_SAVE);
}

uint8_t badge_seen(uint16_t id) {
    char fname[14] = {0,};
    sprintf(fname, "/badges/%d", id);

    return storage_file_exists(fname);
}

uint8_t badge_connected(uint16_t id) {
    if (vbat_out_uvolts && vbat_out_uvolts < UVOLTS_EXTPOWER) { // 50 mV
        // We're on external power.
    } else if (vbat_out_uvolts && vbat_out_uvolts < UVOLTS_CUTOFF) {
        // Batteries are below cut-off voltage
        // Do nothing.
        return 0;
    }
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

void set_badge_seen(uint16_t id, uint8_t type, uint8_t levels, char *name, uint8_t rssi) {
    if (vbat_out_uvolts && vbat_out_uvolts < UVOLTS_EXTPOWER) { // 50 mV
        // We're on external power.
    } else if (vbat_out_uvolts && vbat_out_uvolts < UVOLTS_CUTOFF) {
        // Batteries are below cut-off voltage
        // Do nothing.
        return;
    }

    if (!is_qbadge(id) || id == QBADGE_ID_MAX_UNASSIGNED)
        return;

    // If we're here, it's a qbadge.

    if (type & BADGE_TYPE_HANDLER_MASK && rssi > handler_near_rssi_curr) {
        handler_near_rssi_curr = rssi;
        handler_near_id_curr = id;
        handler_near_element_curr = (element_type) (type & BADGE_TYPE_ELEMENT_MASK);
        strncpy(handler_near_handle_curr, name, QC16_BADGE_NAME_LEN);
        handler_near_handle_curr[QC16_BADGE_NAME_LEN] = 0x00;

        if (handler_near_rssi_curr > handler_near_rssi) {
            handler_near_rssi = rssi;
            handler_near_id = id;
            handler_near_element = (element_type) (type & BADGE_TYPE_ELEMENT_MASK);
            strncpy(handler_near_handle, name, QC16_BADGE_NAME_LEN);
            handler_near_handle[QC16_BADGE_NAME_LEN] = 0x00;
            Event_post(ui_event_h, UI_EVENT_HUD_UPDATE);
        }
    }

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
        // TODO: What if it's changed, and we've already connected to it?
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

uint8_t set_badge_connected(uint16_t id, uint8_t type, uint8_t levels, char *name) {
    if (vbat_out_uvolts && vbat_out_uvolts < UVOLTS_EXTPOWER) { // 50 mV
        // We're on external power.
    } else if (vbat_out_uvolts && vbat_out_uvolts < UVOLTS_CUTOFF) {
        // Batteries are below cut-off voltage
        // Do nothing.
        return 0;
    }

    if (!is_qbadge(id) && !is_cbadge(id))
        return 0;
    if (id == QBADGE_ID_MAX_UNASSIGNED || id == CBADGE_ID_MAX_UNASSIGNED)
        return 0;

    badge_file_t badge_file = {0,};
    char fname[14] = {0,};
    sprintf(fname, "/badges/%d", id);
    uint8_t ret = 0;

    if (storage_file_exists(fname)) {
        // We've seen it before. Maybe we've even connected.
        storage_read_file(fname, (uint8_t *) &badge_file, sizeof(badge_file_t));
    } else {
        // We've never seen or connected to it before.
        // We should set it as seen, first:
        set_badge_seen(id, type, levels, name, 0);

        if (is_cbadge(id)) {
            if (id - CBADGE_ID_START > badge_conf.stats.cbadges_in_system) {
                badge_conf.stats.cbadges_in_system = id - CBADGE_ID_START + 1;
            }
        } else {
            if (id - QBADGE_ID_START > badge_conf.stats.qbadges_in_system) {
                badge_conf.stats.qbadges_in_system = id - QBADGE_ID_START +1;
            }
        }
    }

    if (!badge_file.times_connected) {
        // never connected before.
        ret = 1;
        if (is_cbadge(id)) {
            badge_conf.stats.cbadges_connected_count++;
            game_process_new_cbadge();
        } else {
            badge_conf.stats.qbadges_connected_count++;
        }
        Event_post(ui_event_h, UI_EVENT_DO_SAVE);
    }
    badge_file.times_connected++;

    if (badge_file.badge_type != type) {
        badge_file.badge_type = type;

        if (type & BADGE_TYPE_HANDLER_MASK) {
            if (is_cbadge(id)) {
                badge_conf.stats.cbadges_handler_connected_count++;
                if (badge_conf.stats.cbadges_handler_connected_count > badge_conf.stats.cbadge_handlers_in_system) {
                    badge_conf.stats.cbadge_handlers_in_system++;
                }
            } else {
                badge_conf.stats.cbadges_handler_connected_count++;
                if (badge_conf.stats.cbadges_handler_connected_count > badge_conf.stats.cbadge_handlers_in_system) {
                    badge_conf.stats.cbadge_handlers_in_system++;
                }
            }
        }

        if (type & BADGE_TYPE_UBER_MASK) {
            if (is_cbadge(id)) {
                badge_conf.stats.cbadges_uber_connected_count++;
                if (badge_conf.stats.cbadges_uber_connected_count > badge_conf.stats.cbadge_ubers_in_system) {
                    badge_conf.stats.cbadge_ubers_in_system++;
                }
            } else {
                badge_conf.stats.qbadges_uber_connected_count++;
                if (badge_conf.stats.qbadges_uber_connected_count > badge_conf.stats.qbadge_ubers_in_system) {
                    badge_conf.stats.qbadge_ubers_in_system++;
                }
            }
        }

        Event_post(ui_event_h, UI_EVENT_DO_SAVE);
    }

    badge_file.badge_id = id;
    badge_file.levels = levels;
    strncpy(badge_file.handle, name, QC16_BADGE_NAME_LEN);
    badge_file.handle[QC16_BADGE_NAME_LEN] = 0x00;

    storage_overwrite_file(fname, (uint8_t *) &badge_file, sizeof(badge_file_t));

    Event_post(ui_event_h, UI_EVENT_DO_SAVE);
    return ret;
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
