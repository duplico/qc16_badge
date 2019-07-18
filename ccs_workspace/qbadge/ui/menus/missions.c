/*
 * missions.c
 *
 *  Created on: Jul 12, 2019
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

#include "ui/ui.h"
#include "ui/graphics.h"
#include "ui/images.h"
#include "ui/keypad.h"
#include "menus.h"

#include <badge.h>
#include <board.h>
#include <ui/layout.h>

const char mission_menu_text[2][MAINMENU_NAME_MAX_LEN+1] = {
    "Get mission",
    "Send agent",
};

uint8_t mission_picking = 0;
mission_t candidate_mission;

void ui_draw_mission_icons() {
    Graphics_Rectangle rect;

    // TODO: Put the agent icon next to a mission if it's being run.
    for (uint8_t i=0; i<3; i++) {
        rect.xMin=124;
        rect.yMin=TOPBAR_HEIGHT+3+i*(TOPBAR_HEIGHT+2);
        rect.xMax=rect.xMin+TOPBAR_SEG_WIDTH_PADDED*2;
        rect.yMax=rect.yMin+TOPBAR_HEIGHT;
        Graphics_drawRectangle(&ui_gr_context_landscape, &rect);
        mission_t mission;

        if (mission_picking && i == ui_y_cursor) {
            mission = candidate_mission;
        } else if (!badge_conf.mission_assigned[i]) {
            continue;
        } else {
            mission = badge_conf.missions[i];
        }

        ui_draw_element(mission.element_types[0], mission.element_levels[0], 5, mission.element_rewards[0], rect.xMin+2, rect.yMin+1);

        if (!badge_conf.agent_present && badge_conf.agent_mission_id == i) {
            Graphics_drawString(&ui_gr_context_landscape, "agent", 5, rect.xMax+1, rect.yMin+2, 0);
        }

        if (mission_picking && i == ui_y_cursor) {
            // If we're mission picking, and this is the relevant mission...
            // Don't fade it, but DO mark it
            Graphics_drawLine(&ui_gr_context_landscape, rect.xMax, rect.yMin/2+rect.yMax/2, rect.xMax+5, rect.yMin/2+rect.yMax/2+5);
            Graphics_drawLine(&ui_gr_context_landscape, rect.xMax, rect.yMin/2+rect.yMax/2, rect.xMax+5, rect.yMin/2+rect.yMax/2-5);
        } else if (mission_picking) {
            // If we're in mission picking mode, fade out the entire mission
            //  other than the one we're assigning.
            fadeRectangle_xy(&ui_gr_context_landscape, rect.xMin+1, rect.yMin+1, rect.xMax-1, rect.yMax-1);
        } else {
            // In a normal render, fade out the element if it's not equipped:
            // TODO: We also need to be of sufficient level.
            if (badge_conf.element_selected != mission.element_types[0]) {
                fadeRectangle_xy(&ui_gr_context_landscape, rect.xMin+2, rect.yMin+1, rect.xMin+1+TOPBAR_ICON_WIDTH, rect.yMin+1+TOPBAR_ICON_HEIGHT);
            }
        }

        // TODO: handle the second one:
        if (mission.element_types[1] != ELEMENT_COUNT_NONE) {
            ui_draw_element(mission.element_types[1], mission.element_levels[1], 5, mission.element_rewards[1], rect.xMin+2+TOPBAR_SEG_WIDTH, rect.yMin+1);
        }
    }
}

void ui_draw_mission_menu() {
    if (mission_picking) {
        ui_draw_menu_icons(3, image_missionmenu_icons, mission_menu_text, 5, 0, TOPBAR_HEIGHT+8, 2);
    } else {
        ui_draw_menu_icons(ui_x_cursor, image_missionmenu_icons, mission_menu_text, 5, 0, TOPBAR_HEIGHT+8, 2);
    }

    if (!mission_possible()) {
        // There are no handlers nearby (not even the vhandler)
        // Put an X over the mission-getting icon
        Graphics_drawLine(&ui_gr_context_landscape, 0, TOPBAR_HEIGHT+8, 0+image_missionmenu_icons[0]->xSize, TOPBAR_HEIGHT+8+image_missionmenu_icons[0]->ySize);
    }

    if (!badge_conf.agent_present) {
        // Agent is currently on a mission.
        // Put an X over the mission-doing icon
        uint16_t x=0+image_missionmenu_icons[0]->xSize+5;
        Graphics_drawLine(&ui_gr_context_landscape, x, TOPBAR_HEIGHT+8, x+image_missionmenu_icons[1]->xSize, TOPBAR_HEIGHT+8+image_missionmenu_icons[1]->ySize);
    }
}

void ui_draw_missions() {
    // Clear the buffer.
    Graphics_clearDisplay(&ui_gr_context_landscape);

    ui_draw_top_bar();
    ui_draw_mission_icons();
    ui_draw_mission_menu();

    Graphics_flushBuffer(&ui_gr_context_landscape);
}

void ui_missions_do(UInt events) {
    if (pop_events(&events, UI_EVENT_REFRESH)) {
        ui_draw_missions();
    }
    if (pop_events(&events, UI_EVENT_KB_PRESS)) {
        switch(kb_active_key_masked) {
        case KB_OK:
            if (mission_picking) {
                // TODO: don't allow it to be done on a mission currently being run.
                mission_picking = 0;
                ui_x_cursor = 0;
                // Copy the candidate mission into the config mission
                memcpy(&badge_conf.missions[ui_y_cursor], &candidate_mission, sizeof(mission_t));
                badge_conf.mission_assigned[ui_y_cursor] = 1;
                write_conf();// and save. // TODO: move
                epd_do_partial = 1;
                Event_post(ui_event_h, UI_EVENT_REFRESH);
            } else {
                if (ui_x_cursor == 0) {
                    // TODO: validate conditions better
                    if (mission_possible()) {
                        mission_picking = 1;
                        candidate_mission = generate_mission();
                        ui_y_cursor = 0;
                        epd_do_partial = 1;
                        Event_post(ui_event_h, UI_EVENT_REFRESH);
                    }
                } else {
                    // Mission
                    // TODO: validate conditions
                    // Choose & start a mission
                    for (uint8_t i=0; i<3; i++) {
                        // take the first one we qualify for
                        if (mission_qualifies(i)) {
                            mission_begin_by_id(i);
                            Event_post(ui_event_h, UI_EVENT_REFRESH);
                            break;
                        }
                    }
                }
            }
        }
    }
}
