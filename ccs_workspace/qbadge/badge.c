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

#include "queercon_drivers/storage.h"
#include <ui/ui.h>
#include <ui/leds.h>
#include <qc16_serial_common.h>
#include "badge.h"

// In this file lives all the state and stuff related to the game.
// Most of this is persistent.

qbadge_conf_t badge_conf;

#pragma DATA_SECTION(startup_id, ".qc16cfg")
volatile const uint16_t startup_id = QBADGE_ID_UNASSIGNED;

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

// TODO: track closest handler by RSSI
// TODO: we need a >second-scale click

uint8_t handler_nearby() {
    // TODO: implement:
    return 0;
}

/// Returns true if it is possible to call generate_mission().
uint8_t mission_possible() {
    return handler_nearby() || badge_conf.vhandler_present;
}

#define EXP_PER_LEVEL0 1
#define EXP_PER_LEVEL1 2
#define EXP_PER_LEVEL2 4
#define EXP_PER_LEVEL3 8
#define EXP_PER_LEVEL4 16
#define EXP_PER_LEVEL5 0

#define MISSIONS_TO_LEVEL1 2
#define MISSIONS_TO_LEVEL2 4
#define MISSIONS_TO_LEVEL3 8
#define MISSIONS_TO_LEVEL4 8
#define MISSIONS_TO_LEVEL5 8

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
    // TODO: Consider doing this with pointers instead.
    // TODO: Missions with multiple elements should maybe give better rewards
    mission_t new_mission;
    new_mission.element_types[1] = ELEMENT_COUNT_NONE;

    // TODO: Check which handlers are around

    // First, we assign a type and level to the mission element(s).
    // This is done based on which handler we pull the mission from.
    // That can either be a real handler, nearby, in which case we
    // select which one based on RSSI; or, it can be the virtual
    // handler, which is available to assign low-level missions at
    // a configurable time interval.

    if (!handler_nearby()) {
        // TODO: constant config for this timing:
        // 10-20 minutes
        badge_conf.vhandler_return_time = Seconds_get() + 600 + rand() % 600;
        badge_conf.vhandler_present = 0;
        // The vhandler always hands out a primary element of qtype.
        new_mission.element_types[0] = (element_type) (rand() % 3);

        // TODO: extract constant:
        // The vhandler has a max level it can assign:
        new_mission.element_levels[0] = rand() % 3;

        if (!(rand() % 3)) {
            // There's a chance to assign a second element.
            // TODO: Extract constant
            // A second element is needed. We'll assign it totally randomly.
            new_mission.element_types[1] = (element_type) (rand() % 6);
        }
    } else {
        // Human handler. No cooldown.
        // TODO: We sure about no cooldown?
        // We use the one with the highest RSSI
        // Mostly, it assigns missions that are on-brand for that handler.

        // TODO: Base this on handlers:
        new_mission.element_types[0] = (element_type) (rand() % 3);

        // TODO: Base this on level?
        new_mission.element_levels[0] = rand() % 6;

        // TODO: higher levels should make it more likely that there's a
        //       second element.
        if (!(rand() % 3)) {
            // There's a chance to assign a second element.
            // TODO: Extract constant
            // A second element is needed. We'll assign it totally randomly.
            // TODO: actually, we should prefer to make it the complement of
            //       the primary element.
            new_mission.element_types[1] = (element_type) (rand() % 6);
            // TODO: this needs to be after the original is adjusted:
            //       maybe???
            new_mission.element_levels[1] = rand() % (new_mission.element_levels[0]+1);
        }
    }

    // Is the mission too high-level for us? If so, reduce it:
    if (new_mission.element_levels[0] > badge_conf.element_level[new_mission.element_types[0]]) {
        new_mission.element_levels[0] = badge_conf.element_level[new_mission.element_types[0]];
        // TODO: should it be possible for this to be higher?
        new_mission.element_levels[1] = rand() % (new_mission.element_levels[0]+1);
    }

    // Now, calculate each element's rewards based on its level:
    for (uint8_t i=0; i<2; i++) {
        new_mission.element_rewards[i] = mission_reward_per_level[new_mission.element_levels[i]][0] + rand() % mission_reward_per_level[new_mission.element_levels[i]][1];
        new_mission.element_progress[i] = mission_exp_per_level[new_mission.element_levels[i]];
    }

    return new_mission;
}

uint8_t mission_qualifies(uint8_t mission_id) {
    // TODO: Handle the serial version

    if (!badge_conf.agent_present) {
        return 0; // can't do a mission without the agent.
    }

    if (badge_conf.missions[mission_id].element_levels[0] <= badge_conf.element_level[badge_conf.missions[mission_id].element_types[0]] && badge_conf.element_selected == badge_conf.missions[mission_id].element_types[0]) {
        // mission is at or below our level for the element selected
        return 1;
    }

    return 0;
}

// TODO: rename:
void begin_mission_id(uint8_t mission_id) {
    badge_conf.agent_mission_id = mission_id;
    badge_conf.agent_present = 0;
    // TODO: determine mission duration
    // TODO: extract constant
    // TODO: should be higher than 1 minute (10m, say)
    badge_conf.agent_return_time = Seconds_get() + 60;
    // TODO: issue event to update agent status in UI
    // TODO: write conf
    write_conf();
    // TODO: if we only ever call write_conf from a task context, we should be ok?
}

/// Complete and receive rewards from a mission.
void complete_mission(mission_t *mission) {
    badge_conf.agent_present = 1;
    // TODO: flag the HUD thingy instead
    Event_post(ui_event_h, UI_EVENT_REFRESH);

    uint8_t element_position = 0;
    // TODO: is the following always true?
    // NB: the primary element is always better, so if that's us then we want it:
    if (mission->element_types[0] == badge_conf.element_selected && mission->element_levels[0] <= badge_conf.element_level[badge_conf.element_selected]) {
        element_position = 0;
    } else if (mission->element_types[1] < 3) {
        element_position = 1;
    } else {
        // PROBLEM
        // TODO
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
        // TODO: animation flag
        badge_conf.element_level[badge_conf.element_selected]++;
        // TODO: assert this is < 5
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

// TODO: rename to signify that this is the thing that happens at minute-ish scale:
void reset_scan_cycle(UArg a0) {
    if (qbadges_near_count_running != qbadges_near_count) {
        // TODO: post event
    }
    qbadges_near_count_running = 0;
    memset((void *) qbadges_near, 0x00, 4*QBADGE_BITFIELD_LONGS);

    // TODO: refactor this out into a "check stuff's timing" function:

    if (!badge_conf.agent_present && Seconds_get() > badge_conf.agent_return_time) {
        complete_mission_id(badge_conf.agent_mission_id);
        // TODO: post event for HUD update
    }

    if (!badge_conf.vhandler_present && Seconds_get() > badge_conf.vhandler_return_time) {
        badge_conf.vhandler_present = 1;
        // TODO: post event for HUD update
    }

    // TODO: write config (this is a swi, which is not good for that...)
    //       (unless this is the *only* place we want to ever save...
    //       Actually, I could be down for that...
    // TODO: issue event to update the count & handler status
}

uint8_t conf_file_exists() {
    // TODO: This isn't the same as the config file being valid.
    volatile int32_t status;
    spiffs_stat stat;
    status = SPIFFS_stat(&fs, "/qbadge/conf", &stat);
    if (status == SPIFFS_OK && stat.size == sizeof(badge_conf)) {
        return 1;
    } else if (status == SPIFFS_OK) {
        // wrong size:
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

    badge_conf.last_clock = 0;
    badge_conf.agent_present = 1;
    badge_conf.element_level_max[0] = 2;
    badge_conf.element_level_max[1] = 2;
    badge_conf.element_level_max[2] = 2;

    badge_conf.vhandler_present=1;

    // NB: These both will save the badge_conf:
    set_badge_seen(badge_conf.badge_id, "");
    set_badge_connected(badge_conf.badge_id, "");
    srand(badge_conf.badge_id);

    // TODO: Set the selected element

    // TODO: Initialize the current animation persistence.
    write_conf();
}

uint8_t config_is_valid() {
    return 1;
}

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
        if (qbadges_near_count_running > qbadges_near_count) {
            qbadges_near_count = qbadges_near_count_running;
            // TODO: Raise the refresh signal
        }
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
    // TODO
//    if (config_is_valid()) return;

    // If we're still here, the config source was invalid, and
    //  we must generate a new one.
    generate_config();

    // Now that we've created and saved a config, we're going to load it.
    // This way we guarantee that all the proper start-up occurs.
    load_conf(); // TODO: validate


    Clock_Params clockParams;
    Error_Block eb;
    Error_init(&eb);

    Clock_Params_init(&clockParams);
    clockParams.period = 4500000; // 45 seconds // TODO: decide, extract
    clockParams.startFlag = TRUE;
    radar_clock_h = Clock_create(reset_scan_cycle, clockParams.period, &clockParams, &eb);
}
