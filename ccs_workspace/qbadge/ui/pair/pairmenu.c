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

const char pair_menu_text_c[2][MAINMENU_NAME_MAX_LEN+1] = {
    "GO!",
    "Info",
};

const char pair_menu_text_q[2][MAINMENU_NAME_MAX_LEN+1] = {
    "GO!",
    "Files",
};

const Graphics_Image *pair_menu_icons_c[2] = {
    &img_pair_mission,
    &img_pair_cb,
};

const Graphics_Image *pair_menu_icons_q[2] = {
    &img_pair_mission,
    &img_pair_files,
};

void ui_draw_pair_menu_icons() {
    if (is_qbadge(paired_badge.badge_id)) {
        ui_draw_menu_icons(ui_x_cursor, pair_menu_icons_q, pair_menu_text_q, 10, 5, TOPBAR_HEIGHT+8, 2);
    } else {
        ui_draw_menu_icons(ui_x_cursor, pair_menu_icons_c, pair_menu_text_c, 10, 5, TOPBAR_HEIGHT+8, 2);
    }
}

void ui_draw_pair_menu() {
    // Clear the buffer.
    Graphics_clearDisplay(&ui_gr_context_landscape);

    ui_draw_top_bar();
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
                // TODO: Attempt a mission
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
