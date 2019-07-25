/*
 * cbadge_missions.c
 *
 *  Created on: Jul 21, 2019
 *      Author: george
 */
#include <stdint.h>

#include <msp430fr2111.h>

#include <qc16.h>

#include <cbadge.h>
#include <cbadge_serial.h>

#include "buttons.h"

mission_t current_missions[3];
uint8_t missions_assigned[3];

const uint8_t exp_required_per_level[6] = {
    EXP_PER_LEVEL0*MISSIONS_TO_LEVEL1,
    EXP_PER_LEVEL0*MISSIONS_TO_LEVEL1+EXP_PER_LEVEL1*MISSIONS_TO_LEVEL2,
    EXP_PER_LEVEL0*MISSIONS_TO_LEVEL1+EXP_PER_LEVEL1*MISSIONS_TO_LEVEL2+EXP_PER_LEVEL2*MISSIONS_TO_LEVEL3,
    EXP_PER_LEVEL0*MISSIONS_TO_LEVEL1+EXP_PER_LEVEL1*MISSIONS_TO_LEVEL2+EXP_PER_LEVEL2*MISSIONS_TO_LEVEL3+EXP_PER_LEVEL3*MISSIONS_TO_LEVEL4,
    EXP_PER_LEVEL0*MISSIONS_TO_LEVEL1+EXP_PER_LEVEL1*MISSIONS_TO_LEVEL2+EXP_PER_LEVEL2*MISSIONS_TO_LEVEL3+EXP_PER_LEVEL3*MISSIONS_TO_LEVEL4+EXP_PER_LEVEL4*MISSIONS_TO_LEVEL5,
    255, // NB: must be unreachable
};

/// Complete and receive rewards from a mission.
void complete_mission(mission_t *mission) {
    uint8_t element_position = 0xff;

    for (uint8_t i=0; i<2; i++) {
        if (mission->element_types[i] == ELEMENT_COUNT_NONE || mission->element_types[i] < 3) {
            continue; // Irrelevant to us.
        }

        if (mission->element_types[i] == badge_conf.element_selected && mission->element_levels[i] <= badge_conf.element_level[badge_conf.element_selected]) {
            element_position = i;
            break;
        }
    }

    if (badge_conf.element_selected == ELEMENT_COUNT_NONE || element_position == 0xff) {
        // bork bork
        return;
    }

    uint8_t element_selected_index = (uint8_t) badge_conf.element_selected - 3;

    // Ok! WE DID IT.
    // We definitely get all the resources we earned.
    // NB: We're not worrying about a rollover, because it's 32 bits,
    //     and that would be LOLtastic if that happened.
    //     (also, it would take running almost 4.3 million
    //      level 5 missions.)
    badge_conf.element_qty[element_selected_index] += mission->element_rewards[element_position];

    // Now, let's look at progress.
    // First, we'll add the level-up amount.
    // NB: This is constructed in a way that means a byte won't be able to
    //     overflow. So, it's OK to blindly add the progress reward,
    //     and then check what happened.
    badge_conf.element_level_progress[element_selected_index] += mission->element_progress[element_position];

    // NB: We don't need to constrain progress by level cap, because cbadges
    //     do not have level caps.

    // Now, we can determine if this was enough to increase a level.
    if (badge_conf.element_level_progress[element_selected_index] >= exp_required_per_level[badge_conf.element_level[element_selected_index]]) {
        badge_conf.element_level[element_selected_index]++;
        if (badge_conf.element_level[element_selected_index] > 5)
            badge_conf.element_level[element_selected_index] = 5;
    }

    // Clear our current element when the mission ends:
    badge_conf.element_selected = ELEMENT_COUNT_NONE;

    // Save.
    write_conf();

    // Notify our pairing partner of our updated stats:
    serial_pair();
}
