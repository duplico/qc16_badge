/*
 * game.c
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
#include <queercon_drivers/qbadge_serial.h>
#include <ui/graphics.h>
#include <ui/images.h>
#include <ui/ui.h>
#include <ui/leds.h>
#include <qc16_serial_common.h>
#include "badge.h"

char handler_name_missionpicking[QC16_BADGE_NAME_LEN+1] = {0,};

uint8_t mission_avail_levels[6] = {0,};

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

/// Generate and return a new mission. NB: GATE ON mission_getting_possible().
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

        // TODO:

        new_mission.element_types[0] = (element_type) (rand() % 3);

        new_mission.element_levels[0] = rand() % 6;

        if (!(rand() % 3)) {
            // A second element is needed.
            new_mission.element_types[1] = (element_type) (rand() % 6);
            new_mission.element_levels[1] = rand() % (new_mission.element_levels[0]+1);
        }

        strncpy(handler_name_missionpicking, handler_near_handle, QC16_BADGE_NAME_LEN);
        handler_name_missionpicking[QC16_BADGE_NAME_LEN] = 0x00;

    } else {
        // vhandler mission.
        badge_conf.vhandler_return_time = Seconds_get() + VHANDLER_COOLDOWN_MIN_SECONDS + rand() % (VHANDLER_COOLDOWN_MAX_SECONDS - VHANDLER_COOLDOWN_MIN_SECONDS);
        badge_conf.vhandler_present = 0;
        // The vhandler always hands out a primary element of qtype.
        new_mission.element_types[0] = (element_type) (rand() % 3);

        // The vhandler has a max level it can assign:
        new_mission.element_levels[0] = rand() % (VHANDLER_MAX_LEVEL-1);

        if (1 || !(rand() % VHANDLER_SECOND_ELEMENT_ONE_IN)) { // TODO
            // There's a chance to assign a second element.
            // A second element is needed.
            new_mission.element_levels[1] = rand() % (VHANDLER_MAX_LEVEL-1);
            if (rand() % 3) {
                new_mission.element_types[1] = (element_type) ((uint8_t)new_mission.element_types[0]+3);
            } else {
                new_mission.element_types[1] = (element_type) (rand() % 6);
            }
        }
        strncpy(handler_name_missionpicking, "vhandler", QC16_BADGE_NAME_LEN);
        handler_name_missionpicking[QC16_BADGE_NAME_LEN] = 0x00;
    }

    // TODO: cap the secondary element at mine+1??? or something???

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

    // Determine the mission's duration:
    new_mission.duration_seconds = MISSION_DURATION_MIN_SECONDS + rand() % (MISSION_DURATION_MAX_SECONDS - MISSION_DURATION_MIN_SECONDS);

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
    if (!badge_conf.agent_present) {
        return 0;
    }

    return mission_levels_qualify_for_element_id(mission, element_position, badge_conf.element_level, badge_conf.element_selected, 0);
}

/// Determines whether the paired badge qualifies for a mission's element.
/**
 ** This is safe to use even when we're not paired, because it returns
 ** false if we're not paired (which should do all the right things
 ** in an unpaired situation).
 **/
uint8_t mission_remote_qualified_for_element_id(mission_t *mission, uint8_t element_position) {
    if (!badge_paired || !paired_badge.agent_present) {
        return 0;
    }

    return mission_levels_qualify_for_element_id(mission, element_position, &paired_badge.element_level[3], paired_badge.element_selected, is_cbadge(paired_badge.badge_id));
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
    // If this is a 1-element mission, obviously #0 is the only one we can do.
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
        &paired_badge.element_level[3],
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

/// Is a mission complete-able?
uint8_t mission_possible(mission_t *mission) {
    if (!badge_conf.agent_present) {
        return 0; // can't do a mission without the agent.
    }

    if (!badge_paired) {
        // Local mission.
        // Is this not a local-only mission?
        if (mission->element_types[1] != ELEMENT_COUNT_NONE) {
            return 0;
        }

        return mission_local_qualified_for_element_id(mission, 0);
    } else {
        if (!paired_badge.agent_present) {
            return 0;
        }

        // Can't do single-element missions while paired.
        if (mission->element_types[1] == ELEMENT_COUNT_NONE) {
            return 0;
        }

        uint8_t local_index = mission_element_id_we_fill(mission);
        uint8_t remote_index = mission_element_id_remote_fills(mission);

        if (local_index == remote_index) {
            return 0;
        }

        if (mission_local_qualified_for_element_id(mission, local_index)
                && mission_remote_qualified_for_element_id(mission, remote_index)) {
            return 1;
        }
    }
    return 0;
}

void mission_begin_by_id(uint8_t mission_id) {
    badge_conf.agent_mission_id = mission_id;
    badge_conf.agent_present = 0;
    badge_conf.agent_return_time = Seconds_get() + badge_conf.missions[mission_id].duration_seconds;
    Event_post(ui_event_h, UI_EVENT_DO_SAVE);
}

/// Attempt to start a mission, returning 1 for success and 0 for failure.
uint8_t mission_begin() {
    // NB: For now, we ACCEPT the RACE CONDITION that the following could
    //     be executed simultaneously on each badge with different resulting
    //     missions:

    for (uint8_t i=0; i<6; i++) {
        mission_t *mission;
        if (i < 3) {
            if (!badge_conf.mission_assigned[i])
                continue;
            mission = &badge_conf.missions[i];
        } else {
            if (!badge_paired || !paired_badge.mission_assigned[i])
                continue;
            mission = &paired_badge.missions[i];
        }

        if (mission_possible(mission)) {
            // This is our mission!

            // Is it remote?
            if (i > 2) {
                // Copy it into our 4th mission slot:
                memcpy(&badge_conf.missions[3], mission, sizeof(mission_t));
                badge_conf.mission_assigned[3] = 1;
                mission_begin_by_id(3);
            } else {
                mission_begin_by_id(i);
            }

            if (badge_paired) {
                // Now, signal that we're starting it.
                serial_mission_go(i, mission);
            }
            return 1;
        }
    }

    // No mission was available.
    return 0;
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
