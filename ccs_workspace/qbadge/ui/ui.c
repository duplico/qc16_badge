/*
 * ui.c
 *
 *  Created on: Jun 3, 2019
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

#include <board.h>
#include <qbadge.h>
#include <badge.h>

#include <queercon_drivers/epd.h>
#include <queercon_drivers/ht16d35b.h>
#include <queercon_drivers/storage.h>
#include <ui/graphics.h>
#include <ui/images.h>
#include <ui/overlays/overlays.h>
#include <ui/menus/menus.h>
#include <ui/leds.h>
#include <ui/ui.h>
#include <ui/keypad.h>
#include <ui/layout.h>

Event_Handle ui_event_h;
Graphics_Context ui_gr_context_landscape;
Graphics_Context ui_gr_context_portrait;

#define UI_STACKSIZE 2048
Task_Struct ui_task;
uint8_t ui_task_stack[UI_STACKSIZE];

uint8_t ui_current = UI_SCREEN_STORY1;
uint8_t ui_colorpicking = 0;
uint8_t ui_textentry = 0;

uint8_t ui_x_cursor = 0;
uint8_t ui_y_cursor = 0;
uint8_t ui_x_max = 0;
uint8_t ui_y_max = 0;



uint8_t ui_is_landscape() {
    if (ui_colorpicking) {
        return 0;
    }
    if (ui_textentry) {
        return 1;
    }
    if (ui_current < LOWEST_LANDSCAPE_SCREEN) {
        return 0;
    }
    return 1;
}

void ui_draw_info() {
    // Clear the buffer.
    Graphics_clearDisplay(&ui_gr_context_landscape);

    ui_draw_top_bar();

    Graphics_flushBuffer(&ui_gr_context_landscape);
}

void ui_draw_scan() {
    // Clear the buffer.
    Graphics_clearDisplay(&ui_gr_context_landscape);

    ui_draw_top_bar();

    Graphics_flushBuffer(&ui_gr_context_landscape);
}

UInt pop_events(UInt *events_ptr, UInt events_to_check) {
    UInt in_events;

    in_events = (*events_ptr) & events_to_check;
    (*events_ptr) &= ~events_to_check;
    return in_events;
}

void ui_draw_screensaver() {
    Graphics_clearDisplay(&ui_gr_context_portrait);

    Graphics_setFont(&ui_gr_context_portrait, &UI_TEXT_FONT);
    Graphics_drawStringCentered(
            &ui_gr_context_portrait,
            "- queercon 16 -",
            15,
            64,
            UI_IDLE_BLOCK0_HEIGHT_PX / 2,
            0
    );

    Graphics_setFont(&ui_gr_context_portrait, &g_sFontCmtt28);
    Graphics_drawStringCentered(
            &ui_gr_context_portrait,
            "DUPLiCO",
            7,
            64,
            UI_IDLE_BLOCK2_TOP_PX + (UI_IDLE_BLOCK2_HEIGHT_PX / 2),
            0
    );

    Graphics_drawLineH(&ui_gr_context_portrait, 0, 128, UI_IDLE_PHOTO_TOP-2);
    Graphics_drawLineH(&ui_gr_context_portrait, 0, 128, UI_IDLE_PHOTO_TOP-1);

    char pathname[SPIFFS_OBJ_NAME_LEN] = "/photos/";
    strcpy(&pathname[8], badge_conf.current_photo);
    qc16gr_drawImageFromFile(&ui_gr_context_portrait, pathname, 0, UI_IDLE_PHOTO_TOP);


    Graphics_flushBuffer(&ui_gr_context_portrait);
}

void ui_transition(uint8_t destination) {
    if (ui_current == destination) {
        return; // Nothing to do.
    }
    ui_x_cursor = 0;
    ui_y_cursor = 0;

    switch(destination) {
    case UI_SCREEN_MAINMENU:
        ui_x_max = MAINMENU_ICON_COUNT-1;
        ui_y_max = 0;
        break;
    case UI_SCREEN_INFO:
        ui_x_max = MAINMENU_ICON_COUNT-1;
        ui_y_max = 0;
        break;
    case UI_SCREEN_MISSIONS:
        ui_x_max = 1;
        ui_y_max = 2;
        mission_picking = 0;
        break;
    case UI_SCREEN_SCAN:
        ui_x_max = MAINMENU_ICON_COUNT-1;
        ui_y_max = 0;
        break;
    case UI_SCREEN_FILES:
        ui_x_max = 2;
        // NB: ui_y_max will be set in the do_ function, so no need here.
        break;
    }

    ui_current = destination;
    Event_post(ui_event_h, UI_EVENT_REFRESH);
}

void ui_screensaver_do(UInt events) {
    if (pop_events(&events, UI_EVENT_REFRESH)) {
        ui_draw_screensaver();
    }
}

/// Do basic menu system, returning 1 if we should skip the expected do function.
uint8_t ui_menusystem_do(UInt events) {
    if (events & UI_EVENT_KB_PRESS) {
        switch(kb_active_key_masked) {
        case KB_BACK:
            if (ui_current == UI_SCREEN_MAINMENU) {
                ui_transition(UI_SCREEN_IDLE);
            } else if (ui_current == UI_SCREEN_MISSIONS && mission_picking) {
                mission_picking = 0;
                Event_post(ui_event_h, UI_EVENT_REFRESH);
            } else {
                ui_transition(UI_SCREEN_MAINMENU);
            }
            return 1;
        case KB_LEFT:
            if (ui_x_cursor == 0)
                ui_x_cursor = ui_x_max;
            else
                ui_x_cursor--;
            Event_post(ui_event_h, UI_EVENT_REFRESH);
            epd_do_partial = 1;
            break;
        case KB_RIGHT:
            if (ui_x_cursor == ui_x_max)
                ui_x_cursor = 0;
            else
                ui_x_cursor++;
            Event_post(ui_event_h, UI_EVENT_REFRESH);
            epd_do_partial = 1;
            break;
        case KB_UP:
            if (ui_y_cursor == 0)
                ui_y_cursor = ui_y_max;
            else
                ui_y_cursor--;
            Event_post(ui_event_h, UI_EVENT_REFRESH);
            epd_do_partial = 1;
            break;
        case KB_DOWN:
            if (ui_y_cursor == ui_y_max)
                ui_y_cursor = 0;
            else
                ui_y_cursor++;
            Event_post(ui_event_h, UI_EVENT_REFRESH);
            epd_do_partial = 1;
            break;
        }
    }
    if (events & UI_EVENT_HUD_UPDATE) {
        // Do a partial redraw with the new numbers:
        Event_post(ui_event_h, UI_EVENT_REFRESH);
        epd_do_partial = 1;
    }
    return 0;
}

void ui_info_do(UInt events) {
    if (pop_events(&events, UI_EVENT_REFRESH)) {
        ui_draw_info();
    }
    if (pop_events(&events, UI_EVENT_KB_PRESS)) {
        switch(kb_active_key_masked) {
        case KB_OK:
            break;
        }
    }
}

void ui_scan_do(UInt events) {
    if (pop_events(&events, UI_EVENT_REFRESH)) {
        ui_draw_scan();
    }
    if (pop_events(&events, UI_EVENT_KB_PRESS)) {
        switch(kb_active_key_masked) {
        case KB_OK:
            break;
        }
    }
}

void ui_task_fn(UArg a0, UArg a1) {
    UInt events;

    storage_init();
    config_init();

    ui_transition(UI_SCREEN_MAINMENU);

    while (1) {
        events = Event_pend(ui_event_h, Event_Id_NONE, ~Event_Id_NONE,  UI_AUTOREFRESH_TIMEOUT);

        if (!events) {
            // This is a timeout.
            if (ui_colorpicking) {
                ui_colorpicking_unload();
            } else if (ui_textentry) {
                ui_textentry_unload(0);
            } else if (ui_current == UI_SCREEN_IDLE) {
            } else {
                ui_transition(UI_SCREEN_IDLE);
            }
            // If we're doing a timeout, it's because nobody is paying
            //  attention. Therefore, it's likely safe to do a full refresh.
            epd_do_partial = 0;
            continue;
        }

        // NB: This order is very important:
        if (kb_active_key_masked == KB_F4_PICKER
                && pop_events(&events, UI_EVENT_KB_PRESS)) {
            if (ui_colorpicking) {
                ui_colorpicking_unload();
            } else {
                ui_colorpicking_load();
            }
        }

        if (kb_active_key_masked == KB_ROT
                && ui_is_landscape()
                && pop_events(&events, UI_EVENT_KB_PRESS)) {
            // The rotate button is pressed, and we're NOT in one of the
            //  portrait modes:
            epd_flip();
            Graphics_flushBuffer(&ui_gr_context_landscape);
        } else if (kb_active_key_masked == KB_ROT) {
        }

        // NB: We only process element changes if the agent is present,
        //     because if the agent is absent we're set in that element
        //     on a mission!
        if (events & UI_EVENT_KB_PRESS && badge_conf.agent_present
                && (   kb_active_key_masked == KB_F1_LOCK
                    || kb_active_key_masked == KB_F2_COIN
                    || kb_active_key_masked == KB_F3_CAMERA)) {
            // NB: Below we are intentionally NOT writing the config.
            //     This isn't a super important configuration option,
            //     so we're not going to waste cycles (CPU and flash)
            //     on saving our whole config every time the user hits
            //     a button. Instead, we'll rely on the periodic save
            //     that writes the current clock to the config.

            // Convert from a button ID to an element enum.
            element_type next_element;
            switch(kb_active_key_masked) {
            case KB_F1_LOCK:
                next_element = ELEMENT_LOCKS;
                break;
            case KB_F2_COIN:
                next_element = ELEMENT_COINS;
                break;
            case KB_F3_CAMERA:
                next_element = ELEMENT_CAMERAS;
                break;
            }

            // Allow elements to be deselected so the LED can turn off.
            if (badge_conf.element_selected == next_element) {
                badge_conf.element_selected = ELEMENT_COUNT_NONE;
            } else {
                badge_conf.element_selected = next_element;
            }

            // We need to refresh the screen if we're looking at missions,
            //  because there are UI elements there that depend on the
            //  selected element button.
            if (ui_current == UI_SCREEN_MISSIONS) {
                epd_do_partial = 1;
                Event_post(ui_event_h, UI_EVENT_REFRESH);
            }
            Event_post(led_event_h, LED_EVENT_FN_LIGHT);
        }

        if (ui_colorpicking) {
            ui_colorpicking_do(events);
        } else if (ui_textentry) {
            ui_textentry_do(events);
        } else {
            // If neither of our "overlay" options are in use, then we follow
            //  a normal state flow:
            if (ui_current == UI_SCREEN_IDLE) {
                if ((events & UI_EVENT_KB_PRESS) && kb_active_key_masked == KB_UP) {
                } else if ((events & UI_EVENT_KB_PRESS) && kb_active_key_masked == KB_DOWN) {
                } else if ((events & UI_EVENT_KB_PRESS) && kb_active_key_masked == KB_OK) {
                    ui_transition(UI_SCREEN_MAINMENU);
                }
                ui_screensaver_do(events);
            } else if (ui_current >= UI_SCREEN_MAINMENU && ui_current <= UI_SCREEN_MAINMENU_END) {
                if (ui_menusystem_do(events)) {
                    continue;
                }
                switch(ui_current) {
                case UI_SCREEN_MAINMENU:
                    ui_mainmenu_do(events);
                    break;
                case UI_SCREEN_INFO:
                    ui_info_do(events);
                    break;
                case UI_SCREEN_MISSIONS:
                    ui_missions_do(events);
                    break;
                case UI_SCREEN_SCAN:
                    ui_scan_do(events);
                    break;
                case UI_SCREEN_FILES:
                    ui_files_do(events);
                    break;
                }
            }
        }
    }
}

uint8_t post() {
    return 1;
}

void ui_init() {
    kb_init();

    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stack = ui_task_stack;
    taskParams.stackSize = UI_STACKSIZE;
    taskParams.priority = 1;
    Task_construct(&ui_task, ui_task_fn, &taskParams, NULL);

    Graphics_initContext(&ui_gr_context_landscape, &epd_gr_display_landscape, &epd_grDisplayFunctions);
    Graphics_setBackgroundColor(&ui_gr_context_landscape, GRAPHICS_COLOR_WHITE);
    Graphics_setForegroundColor(&ui_gr_context_landscape, GRAPHICS_COLOR_BLACK);
    Graphics_clearDisplay(&ui_gr_context_landscape);

    Graphics_initContext(&ui_gr_context_portrait, &epd_gr_display_portrait, &epd_grDisplayFunctions);
    Graphics_setBackgroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_WHITE);
    Graphics_setForegroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_BLACK);
    Graphics_clearDisplay(&ui_gr_context_portrait);

    ui_event_h = Event_create(NULL, NULL);
}
