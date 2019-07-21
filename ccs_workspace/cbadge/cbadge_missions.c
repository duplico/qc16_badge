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
    for (uint8_t element_position = 0; element_position < 2; element_position++) {
        if (mission->element_types[element_position] == ELEMENT_COUNT_NONE) {
            // Not a real element. Skip.
            continue;
        }
        if (mission->element_types[element_position] < ELEMENT_KEYS) {
            // qbadge element. Skip.
            continue;
        }

        element_type reward_element = mission->element_types[element_position];
        uint8_t reward_element_id = (uint8_t) reward_element - 3;

        // If we're here, it's a real element, and applies to us, so we get
        //  to claim its rewards.

        // Ok! WE DID IT.
        // We definitely get all the resources we earned.
        // NB: We're not worrying about a rollover, because it's 32 bits,
        //     and that would be LOLtastic if that happened.
        //     (also, it would take running almost 4.3 million
        //      level 5 missions.)
        badge_conf.element_qty[reward_element_id] += mission->element_rewards[element_position];

        // Now, let's look at progress.
        // First, we'll add the level-up amount.
        // NB: This is constructed in a way that means a byte won't be able to
        //     overflow. So, it's OK to blindly add the progress reward,
        //     and then check what happened.
        badge_conf.element_level_progress[reward_element_id] += mission->element_progress[element_position];

        // Now, we can determine if this was enough to increase a level.
        if (badge_conf.element_level_progress[reward_element_id] >= exp_required_per_level[badge_conf.element_level[reward_element_id]]) {
            badge_conf.element_level[reward_element_id]++;
            if (badge_conf.element_level[reward_element_id] > 5)
                badge_conf.element_level[reward_element_id] = 5;

        }

        // Clear our current element when the mission ends:
        badge_conf.element_selected = ELEMENT_COUNT_NONE;
    }
}
