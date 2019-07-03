/*
 * badge.c
 *
 *  Created on: Jun 29, 2019
 *      Author: george
 */
#include <stdint.h>
#include <stdlib.h>

#include <xdc/runtime/Error.h>
#include <ti/sysbios/knl/Clock.h>

#include <third_party/spiffs/spiffs.h>

#include <qc16.h>

#include "queercon_drivers/storage.h"
#include <ui/leds.h>
#include <qc16_serial_common.h>
#include "badge.h"

// In this file lives all the state and stuff related to the game.
// Most of this is persistent.

qbadge_conf_t badge_conf;

#define QBADGE_BITFIELD_LONGS 21
#define CBADGE_BITFIELD_LONGS 47
uint32_t qbadges_near[QBADGE_BITFIELD_LONGS] = {0, };
uint32_t qbadges_seen[QBADGE_BITFIELD_LONGS] = {0, };
uint32_t qbadges_connected[QBADGE_BITFIELD_LONGS] = {0, };
uint32_t cbadges_connected[CBADGE_BITFIELD_LONGS] = {0, };
uint16_t qbadges_near_count=0;
uint16_t qbadges_near_count_running=0;

Clock_Handle radar_clock_h;

#pragma DATA_SECTION(startup_id, ".qc16cfg")
volatile const uint16_t startup_id = QBADGE_ID_UNASSIGNED;

void reset_scan_cycle(UArg a0) {
    if (qbadges_near_count_running != qbadges_near_count) {
        // TODO: post event
    }
    qbadges_near_count_running = 0;
    memset((void *) qbadges_near, 0x00, 4*QBADGE_BITFIELD_LONGS);
}

uint8_t conf_file_exists() {
    // TODO: This isn't the same as the config file being valid.
    volatile int32_t status;
    spiffs_stat stat;
    status = SPIFFS_stat(&fs, "/qbadge/conf", &stat);
    if (status == SPIFFS_OK && stat.size == sizeof(badge_conf)) {
        return 1;
    } else if (status == SPIFFS_OK) {
        status = SPIFFS_remove(&fs, "/qbadge/conf");
    }
    // TODO: Validate more?
    // We should create it if we got SPIFFS_ERR_NOT_FOUND...
    return 0;
}

void load_conf() {
    // TODO: Make sure this worked.
    storage_read_file("/qbadge/conf", (uint8_t *) (&badge_conf), sizeof(badge_conf));
    storage_read_file("/qbadge/conn_c", (uint8_t *) cbadges_connected, 47*4);
    storage_read_file("/qbadge/conn_q", (uint8_t *) qbadges_connected, 47*4);
    storage_read_file("/qbadge/seen_q", (uint8_t *) qbadges_seen, 47*4);
    load_anim(".current");
    srand(badge_conf.badge_id);
}

void write_conf() {
    storage_overwrite_file("/qbadge/conf", (uint8_t *) (&badge_conf), sizeof(badge_conf));
    storage_overwrite_file("/qbadge/conn_c", (uint8_t *) cbadges_connected, CBADGE_BITFIELD_LONGS*4);
    storage_overwrite_file("/qbadge/conn_q", (uint8_t *) qbadges_connected, QBADGE_BITFIELD_LONGS*4);
    storage_overwrite_file("/qbadge/seen_q", (uint8_t *) qbadges_seen, QBADGE_BITFIELD_LONGS*4);
}

void write_anim_curr() {
    storage_overwrite_file("/colors/.current", (uint8_t *) &led_tail_anim_current, sizeof(led_tail_anim_t));
}

void save_anim(char *name) {
    char pathname[SPIFFS_OBJ_NAME_LEN] = "/colors/";
    strcpy(&pathname[8], name);
    storage_overwrite_file(pathname, (uint8_t *) &led_tail_anim_current, sizeof(led_tail_anim_t));
}

void load_anim(char *name) {
    char pathname[SPIFFS_OBJ_NAME_LEN] = "/colors/";
    strcpy(&pathname[8], name);
    storage_read_file(pathname, (uint8_t *) &led_tail_anim_current, sizeof(led_tail_anim_t));
    storage_overwrite_file("/colors/.current", (uint8_t *) &led_tail_anim_current, sizeof(led_tail_anim_t));
    led_tail_start_anim();
}

void generate_config() {
    // All we start from, here, is our ID (startup_id).
    // The struct is no good. Zero it out.
    memset(&badge_conf, 0x00, sizeof(qbadge_conf_t));

    // TODO: If we got a bad ID from the flash, we can't trust ANYTHING.

    badge_conf.badge_id = startup_id;

    // TODO: Consider writing my name to the name list in the memory?
    //       (This is probably done with set_badge_seen?)

    badge_conf.last_clock = 0;

    set_badge_seen(badge_conf.badge_id, "");
    set_badge_connected(badge_conf.badge_id, "");
    srand(badge_conf.badge_id);

    // TODO: Initialize the current animation persistence.
}

uint8_t config_is_valid() {
    return 1;
}

// TODO: Persistence for:
//  Current LED animation
//  Current photo
//  Badges seen
//  Badges connected
//  Handle?

uint8_t badge_seen(uint16_t id) {
    // TODO: protect from overrun
    if (is_qbadge(id) && check_id_buf(id, (uint8_t *) qbadges_seen))
        return 1;
    return 0;
}

uint8_t badge_connected(uint16_t id) {
    // TODO: protect from overrun
    if (is_cbadge(id) && check_id_buf(id, (uint8_t *)cbadges_connected))
        return 1;
    if (is_qbadge(id) && check_id_buf(id, (uint8_t *)qbadges_connected))
        return 1;
    return 0;
}

uint8_t badge_near(uint16_t id) {
    // TODO: protect from overrun
    if (is_qbadge(id) && check_id_buf(id, (uint8_t *) qbadges_near))
        return 1;
    return 0;
}

uint8_t set_badge_seen(uint16_t id, char *name) {
    if (!is_qbadge(id) || id == QBADGE_ID_UNASSIGNED)
        return 0;

    // If we're here, it's a qbadge.

    // Mark this badge as "nearby"
    if (!badge_near(id) && id != badge_conf.badge_id) {
        // TODO: If we're going to raise a signal on this, we'll need a second buffer.
        set_id_buf(id, (uint8_t *) qbadges_near);
        qbadges_near_count_running++;
    }

    // Let's update its name.
    // TODO:
//    memcpy(person_names[id], name, QC15_PERSON_NAME_LEN-1);
//    person_names[id][QC15_PERSON_NAME_LEN-1]=0x00;

    if (id == QBADGE_ID_UNASSIGNED || id == CBADGE_ID_UNASSIGNED)
            return 0;
    if (badge_seen(id)) {
        return 0;
    }

    set_id_buf(id, (uint8_t *)qbadges_seen);
    badge_conf.qbadges_seen_count++;

    // TODO: uber/handler/etc

    write_conf();
    return 1;
}

uint8_t set_badge_connected(uint16_t id, char *handle) {
    if (!is_qbadge(id) && !is_cbadge(id))
        return 0;

    if (id == QBADGE_ID_UNASSIGNED || id == CBADGE_ID_UNASSIGNED)
        return 0;

    if (badge_connected(id)) {
        return 0;
    }

    if (is_qbadge(id)) {
        set_id_buf(id, (uint8_t *)qbadges_connected);
        badge_conf.qbadges_connected_count++;
    } else if (is_cbadge(id)) {
        set_id_buf(id, (uint8_t *)cbadges_connected);
        badge_conf.cbadges_connected_count++;
    }

    // TODO: uber/handler/etc

    write_conf();
    return 1;
}
// TODO: rename
/// Validate, load, and/or generate this badge's configuration as appropriate.
void init_config() {
    if (conf_file_exists()) {
        load_conf();
    }
    // Check the stored config:
    if (config_is_valid()) return;

    // If we're still here, the config source was invalid, and
    //  we must generate a new one.
    generate_config();


    Clock_Params clockParams;
    Error_Block eb;
    Error_init(&eb);

    Clock_Params_init(&clockParams);
    clockParams.period = 4500000; // 45 seconds // TODO: decide, extract
    clockParams.startFlag = TRUE;
    radar_clock_h = Clock_create(reset_scan_cycle, clockParams.period, &clockParams, &eb);
}
