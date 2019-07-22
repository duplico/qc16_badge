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

// In this file lives all the state and stuff related to the game.
// Most of this is persistent.

qbadge_conf_t badge_conf;

#pragma DATA_SECTION(startup_id, ".qc16cfg")
volatile const uint16_t startup_id = QBADGE_ID_MAX_UNASSIGNED;

uint8_t badge_paired = 0;
pair_payload_t paired_badge = {0,};

// Radar constructs:

#define QBADGE_BITFIELD_LONGS 21
#define CBADGE_BITFIELD_LONGS 47
uint32_t qbadges_near[QBADGE_BITFIELD_LONGS] = {0, };
uint32_t qbadges_seen[QBADGE_BITFIELD_LONGS] = {0, };
uint32_t qbadges_connected[QBADGE_BITFIELD_LONGS] = {0, };
uint32_t cbadges_connected[CBADGE_BITFIELD_LONGS] = {0, };
uint16_t qbadges_near_count=0;
uint16_t qbadges_near_count_running=0;
uint16_t handlers_near_count=0;
uint16_t handlers_near_count_running=0;

uint8_t mission_avail_levels[6] = {0,};

Clock_Handle radar_clock_h;

uint8_t handler_nearby() {
    return 0;
}

/// Returns true if it is possible to call generate_mission().
uint8_t mission_getting_possible() {
    return handler_nearby() || badge_conf.vhandler_present;
}

const uint8_t mission_exp_per_level[6] = {
    EXP_PER_LEVEL0,
    EXP_PER_LEVEL1,
    EXP_PER_LEVEL2,
    EXP_PER_LEVEL3,
    EXP_PER_LEVEL4,
    EXP_PER_LEVEL5,
};

const uint16_t mission_reward_per_level[6][2] = {
    {1,4},      // 1-5
    {5,5},      // 5-10
    {10,40},    // 10-50
    {50,50},    // 50-100
    {100,900}   // 100-1000
};

const uint8_t exp_required_per_level[6] = {
    EXP_PER_LEVEL0*MISSIONS_TO_LEVEL1,
    EXP_PER_LEVEL0*MISSIONS_TO_LEVEL1+EXP_PER_LEVEL1*MISSIONS_TO_LEVEL2,
    EXP_PER_LEVEL0*MISSIONS_TO_LEVEL1+EXP_PER_LEVEL1*MISSIONS_TO_LEVEL2+EXP_PER_LEVEL2*MISSIONS_TO_LEVEL3,
    EXP_PER_LEVEL0*MISSIONS_TO_LEVEL1+EXP_PER_LEVEL1*MISSIONS_TO_LEVEL2+EXP_PER_LEVEL2*MISSIONS_TO_LEVEL3+EXP_PER_LEVEL3*MISSIONS_TO_LEVEL4,
    EXP_PER_LEVEL0*MISSIONS_TO_LEVEL1+EXP_PER_LEVEL1*MISSIONS_TO_LEVEL2+EXP_PER_LEVEL2*MISSIONS_TO_LEVEL3+EXP_PER_LEVEL3*MISSIONS_TO_LEVEL4+EXP_PER_LEVEL4*MISSIONS_TO_LEVEL5,
    255, // NB: must be unreachable
};

/// Generate and return a new mission.
mission_t generate_mission() {
    mission_t new_mission;
    new_mission.element_types[1] = ELEMENT_COUNT_NONE;

    // First, we assign a type and level to the mission element(s).
    // This is done based on which handler we pull the mission from.
    // That can either be a real handler, nearby, in which case we
    // select which one based on RSSI; or, it can be the virtual
    // handler, which is available to assign low-level missions at
    // a configurable time interval.

    if (handler_nearby()) {
        // Human handler.
        // We use the one with the highest RSSI
        // Mostly, it assigns missions that are on-brand for that handler.

        new_mission.element_types[0] = (element_type) (rand() % 3);

        new_mission.element_levels[0] = rand() % 6;

        if (!(rand() % 3)) {
            // A second element is needed.
            new_mission.element_types[1] = (element_type) (rand() % 6);
            new_mission.element_levels[1] = rand() % (new_mission.element_levels[0]+1);
        }
    } else if (badge_conf.vhandler_present) {
        badge_conf.vhandler_return_time = Seconds_get() + VHANDLER_COOLDOWN_MIN_SECONDS + rand() % (VHANDLER_COOLDOWN_MAX_SECONDS - VHANDLER_COOLDOWN_MIN_SECONDS);
        badge_conf.vhandler_return_time = Seconds_get() + 1; // TODO: remove
        badge_conf.vhandler_present = 0;
        // The vhandler always hands out a primary element of qtype.
        new_mission.element_types[0] = (element_type) (rand() % 3);

        // The vhandler has a max level it can assign:
        new_mission.element_levels[0] = rand() % (VHANDLER_MAX_LEVEL-1);

        if (1 || !(rand() % VHANDLER_SECOND_ELEMENT_ONE_IN)) { // TODO
            // There's a chance to assign a second element.
            // A second element is needed. We'll assign it totally randomly.
            new_mission.element_types[1] = (element_type) (rand() % 6);
        }
    }

    // Is the mission too high-level for us? If so, reduce it:
    if (new_mission.element_levels[0] > badge_conf.element_level[new_mission.element_types[0]]) {
        new_mission.element_levels[0] = badge_conf.element_level[new_mission.element_types[0]];
        new_mission.element_levels[1] = rand() % (new_mission.element_levels[0]+1);
    }

    // Now, calculate each element's rewards based on its level:
    for (uint8_t i=0; i<2; i++) {
        new_mission.element_rewards[i] = mission_reward_per_level[new_mission.element_levels[i]][0] + rand() % mission_reward_per_level[new_mission.element_levels[i]][1];
        new_mission.element_progress[i] = mission_exp_per_level[new_mission.element_levels[i]];
    }

    return new_mission;
}

uint8_t mission_levels_qualify_for_element_id(mission_t *mission, uint8_t element_position, uint8_t element_level[3], element_type element_selected, uint8_t is_cbadge) {
    if (element_position > 1) {
        // Invalid element position ID
        return 0;
    }

    if (mission->element_types[element_position] == ELEMENT_COUNT_NONE) {
        // This slot is unused.
        return 0;
    }

    if (!is_cbadge && mission->element_types[element_position] > 2) {
        // Not a qbadge element
        return 0;
    }

    if (is_cbadge && mission->element_types[element_position] < 3) {
        // Not a cbadge element
        return 0;
    }

    if (element_selected != mission->element_types[element_position]) {
        // Selected element differs from required element
        return 0;
    }

    uint8_t element_level_index = mission->element_types[element_position] % 3;

    if (mission->element_levels[element_position] <= element_level[element_level_index]) {
        // Badge is of sufficient level!
        return 1;
    }

    // Level is insufficient.
    return 0;
}

uint8_t mission_local_qualified_for_element_id(mission_t *mission, uint8_t element_position) {
    return mission_levels_qualify_for_element_id(mission, element_position, badge_conf.element_level, badge_conf.element_selected, 0);
}

/// Determines whether the paired badge qualifies for a mission's element.
/**
 ** This is safe to use even when we're not paired, because it returns
 ** false if we're not paired (which should do all the right things
 ** in an unpaired situation).
 **/
uint8_t mission_remote_qualified_for_element_id(mission_t *mission, uint8_t element_position) {
    if (!badge_paired) {
        return 0;
    }

    return mission_levels_qualify_for_element_id(mission, element_position, badge_conf.element_level, paired_badge.element_selected, is_cbadge(paired_badge.badge_id));
}

/// In the provided mission, which element ID do we fulfill?
/**
 ** This, importantly, does not verify that the mission is doable.
 ** That is, if  the mission is do-able, this will return the index of the
 ** element that we fulfill in it. But, if the mission is not do-able,
 ** then this function's return value is not necessarily meaningful.
 */
uint8_t mission_element_id_fulfilled_by(mission_t *mission, uint8_t *this_element_level, element_type this_element_selected, uint16_t this_id,
                                        uint8_t *other_element_level, element_type other_element_selected, uint16_t other_id, uint8_t paired) {
    // If this is a 1-element mission, obviously it's the only one we can do.
    if (mission->element_types[1] == ELEMENT_COUNT_NONE) {
        return 0;
    }

    // Check if "this" badge only qualifies for one of the two element slots:
    if (mission_levels_qualify_for_element_id(mission, 0, this_element_level, this_element_selected, is_cbadge(this_id))
            && !mission_levels_qualify_for_element_id(mission, 1, this_element_level, this_element_selected, is_cbadge(this_id))) {
        return 0;
    }
    if (mission_levels_qualify_for_element_id(mission, 1, this_element_level, this_element_selected, is_cbadge(this_id))
            && !mission_levels_qualify_for_element_id(mission, 0, this_element_level, this_element_selected, is_cbadge(this_id))) {
        return 1;
    }

    // Now, we either qualify for both, or we qualify for neither.
    //  The "neither" case must be checked elsewhere.

    if (!paired) {
        // If we're not paired, just pick the first one:
        return 0;
    }

    // If we're here, that means we're paired, and we qualify for both elements
    //  of this mission. That means we need to determine if our pair partner
    //  qualifies for any elements of the mission, and if so, we need to
    //  select the other one.


    if (mission_levels_qualify_for_element_id(mission, 0, other_element_level, other_element_selected, is_cbadge(other_id))
            && !mission_levels_qualify_for_element_id(mission, 1, other_element_level, other_element_selected, is_cbadge(other_id))) {
        return 0;
    }
    if (mission_levels_qualify_for_element_id(mission, 1, other_element_level, other_element_selected, is_cbadge(other_id))
            && !mission_levels_qualify_for_element_id(mission, 0, other_element_level, other_element_selected, is_cbadge(other_id))) {
        return 1;
    }

    // If we're here, both badges qualify for both elements. So we'll pick based on badge ID:
    if (this_id < other_id) {
        return 0;
    }

    return 1;
}

uint8_t mission_element_id_we_fill(mission_t *mission) {
    return mission_element_id_fulfilled_by(
        mission,
        badge_conf.element_level,
        badge_conf.element_selected,
        badge_conf.badge_id,
        paired_badge.element_level,
        paired_badge.element_selected,
        paired_badge.badge_id,
        badge_paired
    );
}

uint8_t mission_element_id_remote_fills(mission_t *mission) {
    if (!badge_paired) {
        // This is an error.
        return 0;
    }
    return mission_element_id_fulfilled_by(
        mission,
        paired_badge.element_level,
        paired_badge.element_selected,
        paired_badge.badge_id,
        badge_conf.element_level,
        badge_conf.element_selected,
        badge_conf.badge_id,
        badge_paired
    );
}

/// Can we participate in this mission?
uint8_t mission_can_participate(mission_t *mission) {
    if (!badge_conf.agent_present) {
        return 0; // can't do a mission without the agent.
    }

    return mission_local_qualified_for_element_id(mission, 0) || mission_local_qualified_for_element_id(mission, 1);
}

/// Is a local-only mission complete-able by our badge?
uint8_t mission_solo_qualifies(uint8_t mission_id) {
    if (!badge_conf.agent_present) {
        return 0; // can't do a mission without the agent.
    }

    // Is this not a local-only mission?
    if (badge_conf.missions[mission_id].element_types[1] != ELEMENT_COUNT_NONE) {
        return 0;
    }

    return mission_local_qualified_for_element_id(&badge_conf.missions[mission_id], 0);
}

void mission_begin_by_id(uint8_t mission_id) {
    badge_conf.agent_mission_id = mission_id;
    badge_conf.agent_present = 0;
    badge_conf.agent_return_time = Seconds_get() + 60;
    Event_post(ui_event_h, UI_EVENT_DO_SAVE);
    Event_post(ui_event_h, UI_EVENT_HUD_UPDATE);
}

/// Complete and receive rewards from a mission.
void complete_mission(mission_t *mission) {
    badge_conf.agent_present = 1;
    Event_post(ui_event_h, UI_EVENT_HUD_UPDATE);

    uint8_t element_position = 0;
    // NB: the primary element is always better, so if that's us then we want it:
    if (mission->element_types[0] == badge_conf.element_selected && mission->element_levels[0] <= badge_conf.element_level[badge_conf.element_selected]) {
        element_position = 0;
    } else if (mission->element_types[1] < 3) {
        element_position = 1;
    } else {
        // oops
        return;
    }

    // Ok! WE DID IT.
    // We definitely get all the resources we earned.
    // NB: We're not worrying about a rollover, because it's 32 bits,
    //     and that would be LOLtastic if that happened.
    //     (also, it would take running almost 4.3 million
    //      level 5 missions.)
    badge_conf.element_qty[badge_conf.element_selected] += mission->element_rewards[element_position];

    // Now, let's look at progress.
    // First, we'll add the level-up amount.
    // NB: This is constructed in a way that means a byte won't be able to
    //     overflow. So, it's OK to blindly add the progress reward,
    //     and then check what happened.
    badge_conf.element_level_progress[badge_conf.element_selected] += mission->element_progress[element_position];

    // First, we need to constrain our progress by our level cap.
    // NB: This depends DESPERATELY on a level cap never being 0.
    if (badge_conf.element_level_progress[badge_conf.element_selected] > exp_required_per_level[badge_conf.element_level_max[badge_conf.element_selected]-1]) {
        badge_conf.element_level_progress[badge_conf.element_selected] = exp_required_per_level[badge_conf.element_level_max[badge_conf.element_selected]-1];
    }

    // Now, we can determine if this was enough to increase a level.
    if (badge_conf.element_level_progress[badge_conf.element_selected] >= exp_required_per_level[badge_conf.element_level[badge_conf.element_selected]]) {
        badge_conf.element_level[badge_conf.element_selected]++;
        if (badge_conf.element_level[badge_conf.element_selected] > 5)
            badge_conf.element_level[badge_conf.element_selected] = 5;
    }

    // We already posted the agent-present event, so we should be good to go.

    // Clear our current element when the mission ends:
    badge_conf.element_selected = ELEMENT_COUNT_NONE;
    Event_post(led_event_h, LED_EVENT_FN_LIGHT);
}

/// Complete and receive rewards from mission id.
void complete_mission_id(uint8_t mission_id) {
    // Clear the assignment of this mission so it can be replaced:
    badge_conf.mission_assigned[mission_id] = 0;

    // Give us the rewards!
    complete_mission(&badge_conf.missions[mission_id]);
}

void reset_scan_cycle(UArg a0) {
    if (qbadges_near_count_running != qbadges_near_count) {
        Event_post(ui_event_h, UI_EVENT_HUD_UPDATE);
    }
    qbadges_near_count_running = 0;
    memset((void *) qbadges_near, 0x00, 4*QBADGE_BITFIELD_LONGS);

    if (!badge_conf.agent_present && Seconds_get() > badge_conf.agent_return_time) {
        complete_mission_id(badge_conf.agent_mission_id);
    }

    if (!badge_conf.vhandler_present && Seconds_get() > badge_conf.vhandler_return_time) {
        badge_conf.vhandler_present = 1;
        Event_post(ui_event_h, UI_EVENT_HUD_UPDATE);
    }

    Event_post(ui_event_h, UI_EVENT_DO_SAVE);
}

/// Check whether the badge_conf exists, which isn't the same as validating.
uint8_t conf_file_exists() {
    volatile int32_t status;
    spiffs_stat stat;
    status = SPIFFS_stat(&fs, "/qbadge/conf", &stat);
    if (status == SPIFFS_OK && stat.size == sizeof(badge_conf)) {
        return 1;
    } else if (status == SPIFFS_OK) {
        // wrong size:
        status = SPIFFS_remove(&fs, "/qbadge/conf");
    }
    return 0;
}

void load_conf() {
    storage_read_file("/qbadge/conf", (uint8_t *) (&badge_conf), sizeof(badge_conf));
    storage_read_file("/qbadge/conn_c", (uint8_t *) cbadges_connected, 47*4);
    storage_read_file("/qbadge/conn_q", (uint8_t *) qbadges_connected, 47*4);
    storage_read_file("/qbadge/seen_q", (uint8_t *) qbadges_seen, 47*4);
    load_anim(".current");
    // Turn on the LED for my current light:
    Event_post(led_event_h, LED_EVENT_FN_LIGHT);
    Seconds_set(badge_conf.last_clock);
    srand(badge_conf.badge_id);
}

void write_conf() {
    badge_conf.last_clock = Seconds_get();

    storage_overwrite_file("/qbadge/conf", (uint8_t *) (&badge_conf), sizeof(badge_conf));
    storage_overwrite_file("/qbadge/conn_c", (uint8_t *) cbadges_connected, CBADGE_BITFIELD_LONGS*4);
    storage_overwrite_file("/qbadge/conn_q", (uint8_t *) qbadges_connected, QBADGE_BITFIELD_LONGS*4);
    storage_overwrite_file("/qbadge/seen_q", (uint8_t *) qbadges_seen, QBADGE_BITFIELD_LONGS*4);
}

void write_anim_curr() {
    save_anim(".current");
}

void save_anim(char *name) {
    char pathname[SPIFFS_OBJ_NAME_LEN] = "/colors/";
    strncpy(&pathname[8], name, QC16_PHOTO_NAME_LEN);
    pathname[QC16_PHOTO_NAME_LEN] = 0x00;
    storage_overwrite_file(pathname, (uint8_t *) &led_tail_anim_current, sizeof(led_tail_anim_t));
}

void load_anim_abs(char *pathname) {
    storage_read_file(pathname, (uint8_t *) &led_tail_anim_current, sizeof(led_tail_anim_t));
    storage_overwrite_file("/colors/.current", (uint8_t *) &led_tail_anim_current, sizeof(led_tail_anim_t));
    led_tail_start_anim();
}

void load_anim(char *name) {
    char pathname[SPIFFS_OBJ_NAME_LEN] = "/colors/";
    strcpy(&pathname[8], name);
    load_anim_abs(pathname);
}

void save_photo(Graphics_Image *image, char *name) {
    spiffs_file fd;
    char pathname[SPIFFS_OBJ_NAME_LEN] = "/photos/";
    strcpy(&pathname[8], name);
    fd = SPIFFS_open(&fs, pathname, SPIFFS_O_CREAT | SPIFFS_O_WRONLY, 0);
    SPIFFS_write(&fs, fd, image, sizeof(Graphics_Image));
    SPIFFS_write(&fs, fd, image->pPixel, qc16gr_get_image_size(image));
    SPIFFS_close(&fs, fd);
}

void generate_config() {
    // All we start from, here, is our ID (startup_id).
    // The struct is no good. Zero it out.
    memset(&badge_conf, 0x00, sizeof(qbadge_conf_t));

    badge_conf.badge_id = startup_id;

    badge_conf.last_clock = 0;
    badge_conf.agent_present = 1;
    badge_conf.element_level_max[0] = 2;
    badge_conf.element_level_max[1] = 2;
    badge_conf.element_level_max[2] = 2;

    badge_conf.element_qty[0] = 0;
    badge_conf.element_qty[1] = 0;
    badge_conf.element_qty[2] = 0;

    badge_conf.vhandler_present=1;

    // NB: These both will save the badge_conf:
    set_badge_seen(badge_conf.badge_id, "");
    set_badge_connected(badge_conf.badge_id, "");
    srand(badge_conf.badge_id);

    badge_conf.element_selected = ELEMENT_COUNT_NONE;

    // Initialize the first photo:
    save_photo(&img_city, "Tower");
    strcpy(badge_conf.current_photo, "Tower");

    write_conf();
}

uint8_t config_is_valid() {
    return 1;
}

uint8_t badge_seen(uint16_t id) {
    if (is_qbadge(id) && check_id_buf(id, (uint8_t *) qbadges_seen))
        return 1;
    return 0;
}

uint8_t badge_connected(uint16_t id) {
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

uint8_t set_badge_seen(uint16_t id, char *name) {
    if (!is_qbadge(id) || id == QBADGE_ID_MAX_UNASSIGNED)
        return 0;

    // If we're here, it's a qbadge.

    // Mark this badge as "nearby"
    if (!badge_near(id) && id != badge_conf.badge_id) {
        set_id_buf(id, (uint8_t *) qbadges_near);
        qbadges_near_count_running++;
        if (qbadges_near_count_running > qbadges_near_count) {
            Event_post(ui_event_h, UI_EVENT_HUD_UPDATE);
        }
    }

    // Let's update its details/name in our records.

    if (id == QBADGE_ID_MAX_UNASSIGNED || id == CBADGE_ID_MAX_UNASSIGNED)
            return 0;
    if (badge_seen(id)) {
        return 0;
    }

    set_id_buf(id, (uint8_t *)qbadges_seen);
    badge_conf.qbadges_seen_count++;

    Event_post(ui_event_h, UI_EVENT_DO_SAVE);
    return 1;
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
        badge_conf.qbadges_connected_count++;
    } else if (is_cbadge(id)) {
        set_id_buf(id, (uint8_t *)cbadges_connected);
        badge_conf.cbadges_connected_count++;
    }

    Event_post(ui_event_h, UI_EVENT_DO_SAVE);
    return 1;
}

/// Validate, load, and/or generate this badge's configuration as appropriate.
void config_init() {
    if (conf_file_exists()) {
        load_conf();
    }
    // Check the stored config:
    // TODO
//    if (config_is_valid()) return;

    // If we're still here, the config source was invalid, and
    //  we must generate a new one.
    generate_config();

    // Now that we've created and saved a config, we're going to load it.
    // This way we guarantee that all the proper start-up occurs.
    load_conf();


    Clock_Params clockParams;
    Error_Block eb;
    Error_init(&eb);

    Clock_Params_init(&clockParams);
    clockParams.period = SCAN_PERIOD_SECONDS * 100000;
    clockParams.startFlag = TRUE;
    radar_clock_h = Clock_create(reset_scan_cycle, clockParams.period, &clockParams, &eb);
}
