/*
 * mainmenu.c
 *
 *  Created on: Jun 26, 2019
 *      Author: george
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <ti/grlib/grlib.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/drivers/PIN.h>
#include <ti/sysbios/family/arm/cc26xx/Seconds.h>

#include <qbadge.h>

#include <queercon_drivers/epd.h>
#include <queercon_drivers/ht16d35b.h>
#include <queercon_drivers/storage.h>
#include <qc16_serial_common.h>

#include <ui/pair/pair.h>
#include "ui/ui.h"
#include "ui/graphics.h"
#include "ui/images.h"
#include "ui/keypad.h"

#include <badge.h>
#include <board.h>
#include <ui/layout.h>

const char pair_menu_text[3][MAINMENU_NAME_MAX_LEN+1] = {
    "GO!",
    "Info",
    "Files",
};

const Graphics_Image *pair_menu_icons[3] = {
    &img_pair_mission,
    &img_pair_cb,
    &img_pair_files,
};

uint8_t pairing_mission_count = 0;

void ui_draw_pair_menu_icons() {
    uint8_t menu_mask = 0x00;
    uint16_t menu_x = 5;

    if (is_qbadge(paired_badge.badge_id)) {
        menu_mask |= 0b100;
    } else {
        menu_mask |= 0b010;
    }
    if (pairing_mission_count) {
        menu_mask |= 0b001;
    } else {
        menu_x = 296/2 - img_pair_cb.xSize/2;
    }

    ui_draw_menu_icons(ui_x_cursor, menu_mask, 0, pair_menu_icons, pair_menu_text, 10, menu_x, TOPBAR_HEIGHT+8, 3);
}

void ui_draw_pair_missions() {
    pairing_mission_count = 0;
    if (!badge_conf.agent_present || !paired_badge.agent_present) {
        // can't do a mission without an agent.
        return;
    }

    uint8_t candidate_mission_id = 0;

    // TODO: Determine whether to start with my missions, or remote missions.

    Graphics_Rectangle rect;
    rect.xMin=MISSION_BOX_X;
    rect.yMin=MISSION_BOX_Y;

    while (candidate_mission_id < 6) {
        if (candidate_mission_id==3 && is_cbadge(paired_badge.badge_id)) {
            break;
        }

        mission_t *mission;

        if (candidate_mission_id > 2) {
            // Remote mission.
            if (!paired_badge.missions_accepted[candidate_mission_id-3]) {
                candidate_mission_id++;
                continue;
            }
            mission = &paired_badge.missions[candidate_mission_id-3];
        } else {
            // Local mission.
            if (!badge_conf.mission_assigned[candidate_mission_id]) {
                candidate_mission_id++;
                continue;
            }
            mission = &badge_conf.missions[candidate_mission_id];
        }

        if (mission->element_types[1] == ELEMENT_COUNT_NONE) {
            // We're only interested in dual-element missions
            candidate_mission_id++;
            continue;
        }

        // Ok, now we know it's a mission we want to draw.
        rect.xMax=rect.xMin+TOPBAR_SEG_WIDTH_PADDED*2;
        rect.yMax=rect.yMin+TOPBAR_HEIGHT;

        // Draw the mission:
        ui_put_mission_at(mission, candidate_mission_id, rect.xMin, rect.yMin);
        pairing_mission_count++;

        rect.yMin+=TOPBAR_HEIGHT+2;
        if (rect.yMin > 120) {
            // overflowed the screen
            rect.xMin = rect.xMax+3;
            rect.yMin = MISSION_BOX_Y;
        }
        candidate_mission_id++;
    }
}

void ui_draw_pair_menu() {
    // Clear the buffer.
    Graphics_clearDisplay(&ui_gr_context_landscape);

    ui_draw_top_bar();
    ui_draw_pair_missions();
    ui_draw_pair_menu_icons();

    Graphics_flushBuffer(&ui_gr_context_landscape);
}

void ui_pair_menu_do(UInt events) {
    if (pop_events(&events, UI_EVENT_BACK)) {
        // Do nothing; this is the bottom of the stack when paired.
        return;
    }

    if (pop_events(&events, UI_EVENT_REFRESH)) {
        ui_draw_pair_menu();
    }

    if (pop_events(&events, UI_EVENT_KB_PRESS)) {
        switch(kb_active_key_masked) {
        case KB_OK:
            switch(ui_x_cursor) {
            case 0:
                break;
            case 1:
                if (is_qbadge(paired_badge.badge_id)) {
                    ui_transition(UI_SCREEN_PAIR_FILE);
                } else {
                    ui_transition(UI_SCREEN_PAIR_CB_INFO);
                }
                break;
            }
            break;
        }
    }
}
