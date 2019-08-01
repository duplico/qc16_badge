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
#include <post.h>

#include <ble/uble_bcast_scan.h>
#include <queercon_drivers/epd.h>
#include <queercon_drivers/ht16d35b.h>
#include <queercon_drivers/storage.h>
#include <ui/adc.h>
#include <ui/graphics.h>
#include <ui/images.h>
#include <ui/overlays/overlays.h>
#include <ui/menus/menus.h>
#include <ui/pair/pair.h>
#include <ui/leds.h>
#include <ui/ui.h>
#include <ui/keypad.h>
#include <ui/layout.h>

extern const tFont g_sFontfixed10x20;

Event_Handle ui_event_h;
Graphics_Context ui_gr_context_landscape;
Graphics_Context ui_gr_context_portrait;

#define UI_STACKSIZE 2048
Task_Struct ui_task;
uint8_t ui_task_stack[UI_STACKSIZE];

uint8_t ui_current = UI_SCREEN_STORY1;
uint8_t ui_colorpicking = 0;
uint8_t ui_textentry = 0;
uint8_t ui_textbox = 0;

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

UInt pop_events(UInt *events_ptr, UInt events_to_check) {
    UInt in_events;

    in_events = (*events_ptr) & events_to_check;
    (*events_ptr) &= ~events_to_check;
    return in_events;
}

void ui_draw_screensaver() {
    Graphics_clearDisplay(&ui_gr_context_portrait);

    ui_draw_hud(&ui_gr_context_portrait, 1, 0, UI_IDLE_HUD_Y);

    if (badge_conf.badge_type & BADGE_TYPE_UBER_MASK) {
        qc16gr_drawImage(&ui_gr_context_portrait, &img_uber_plate, 0, 0);
    } else if (badge_conf.badge_type & BADGE_TYPE_HANDLER_MASK) {
        qc16gr_drawImage(&ui_gr_context_portrait, &img_handler_plate, 0, 0);
    } else {
        qc16gr_drawImage(&ui_gr_context_portrait, &img_human_plate, 0, 0);
    }


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

    if (ui_current == UI_SCREEN_IDLE) {
        Event_post(led_event_h, LED_EVENT_SIDELIGHT_EN);
    }

    if (destination == UI_SCREEN_IDLE && led_tail_anim_current.type == LED_TAIL_ANIM_TYPE_OFF) {
        Event_post(led_event_h, LED_EVENT_SIDELIGHT_DIS);
    }

    ui_x_cursor = 0;
    ui_y_cursor = 0;

    mission_picking = 0;

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
        break;
    case UI_SCREEN_SCAN:
        ui_x_max = MAINMENU_ICON_COUNT-1;
        ui_y_max = 0;
        break;
    case UI_SCREEN_FILES:
        ui_x_max = 2;
        // NB: ui_y_max will be set in the do_ function, so no need here.
        break;
    case UI_SCREEN_PAIR_MENU:
        ui_x_max = 2;
        ui_y_max = 0;
        break;
    case UI_SCREEN_PAIR_FILE:
        ui_x_max = 0;
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

    if (events & UI_EVENT_HUD_UPDATE) {
        // Do a partial redraw with the new numbers:
        Event_post(ui_event_h, UI_EVENT_REFRESH);
        epd_do_partial = 1;
    }
}

/// Do basic menu system, returning 1 if we should skip the expected do function.
uint8_t ui_menusystem_do(UInt events) {
    if (events & UI_EVENT_KB_PRESS) {
        switch(kb_active_key_masked) {
        case KB_BACK:
            Event_post(ui_event_h, UI_EVENT_BACK);
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
    if (pop_events(&events, UI_EVENT_BACK)) {
        ui_transition(UI_SCREEN_MAINMENU);
        return;
    }
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

void ui_task_fn(UArg a0, UArg a1) {
    UInt events;

    // Read the battery.
    //  (that clock is already running.)
    //  (as is the keyboard clock)
    do {
        Task_sleep(100);
        if (vbat_out_uvolts && vbat_out_uvolts < UVOLTS_EXTPOWER) { // 50 mV
            // We're on external power.
        } else if (vbat_out_uvolts < UVOLTS_CUTOFF) {
            // Batteries are below cut-off voltage
        }
    } while (!vbat_out_uvolts);

    storage_init();

    if (post_status_spiffs == 1) {
        // Don't do our config unless SPIFFS is working.
        config_init();
        if (post_status_config == -1) {

            Graphics_clearDisplay(&ui_gr_context_landscape);
            Graphics_setFont(&ui_gr_context_landscape, &UI_FIXED_FONT);
            uint8_t y = 10;

            Graphics_drawString(&ui_gr_context_landscape, "WARNING!!!", 99, 5, y, 1);
            y += 21;

            Graphics_drawString(&ui_gr_context_landscape, "Stored config seems invalid.", 99, 5, y, 1);
            y += 21;

            Graphics_drawString(&ui_gr_context_landscape, "Press any key to regenerate.", 99, 5, y, 1);
            y += 21;

            Graphics_drawString(&ui_gr_context_landscape, "This CLEARS ALL PROGRESS!", 99, 5, y, 1);
            y += 21;

            Graphics_flushBuffer(&ui_gr_context_landscape);
            Event_pend(ui_event_h, Event_Id_NONE, UI_EVENT_KB_PRESS, BIOS_WAIT_FOREVER);

            // Invalid config.
            // TODO: do what we do for photos here:
            SPIFFS_rename(&fs, "/qbadge/conf", "/qbadge/conf.bak");
            generate_config();

            // Now that we've created and saved a config, we're going to load it.
            // This way we guarantee that all the proper start-up occurs.
            load_conf();
            post_errors--;
            post_status_config = 1;
        }
    }

    // Create and start the LED task; start the tail animation clock.
    led_init();

    while (!post_status_leds) {
        Task_sleep(10);
    }

    // Now, let's look at the POST results.
    if (post_errors) {
        Graphics_clearDisplay(&ui_gr_context_landscape);
        Graphics_setFont(&ui_gr_context_landscape, &UI_FIXED_FONT);
        uint8_t y = 10;
        char disp_text[33] = {0x00,};

        sprintf(disp_text, "POST error count: %d", post_errors);
        Graphics_drawString(&ui_gr_context_landscape, disp_text, 99, 5, y, 1);
        y += 21;

        if (post_status_leds < 1) {
            sprintf(disp_text, "  LED error: %d", post_status_leds);
            Graphics_drawString(&ui_gr_context_landscape, disp_text, 99, 5, y, 1);
            y += 21;
        }

        if (post_status_spiffs < 1) {
            sprintf(disp_text, "  SPIFFS error: %d", post_status_spiffs);
            Graphics_drawString(&ui_gr_context_landscape, disp_text, 99, 5, y, 1);
            y += 21;
            if (post_status_spiffs == -100) {
                Graphics_drawString(&ui_gr_context_landscape, "Almost full, delete files.", 99, 5, y, 1);
            }
        }

        Graphics_drawString(&ui_gr_context_landscape, "Press a key to continue.", 99, 5, y, 1);

        Graphics_flushBuffer(&ui_gr_context_landscape);
        Event_pend(ui_event_h, Event_Id_NONE, UI_EVENT_KB_PRESS, BIOS_WAIT_FOREVER);
    }

    // Create and start the BLE task:
    UBLEBcastScan_createTask();
    // Create and start the serial task.
    serial_init();

    load_anim(".current");

    ui_transition(UI_SCREEN_MAINMENU);

    while (1) {
        events = Event_pend(ui_event_h, Event_Id_NONE, ~Event_Id_NONE,  UI_AUTOREFRESH_TIMEOUT);

        if (events & UI_EVENT_REFRESH) {
            process_seconds();
            // Pop any HUD updates we get, because we're already refreshing.
            Event_pend(ui_event_h, Event_Id_NONE, UI_EVENT_HUD_UPDATE, BIOS_NO_WAIT);
            pop_events(&events, UI_EVENT_HUD_UPDATE);
        }

        // Timeouts only happen in the normal menu system & overlays:
        if (!events && ui_current < UI_SCREEN_PAIR_MENU) {
            // This is a timeout.
            if (ui_colorpicking) {
                ui_colorpicking_unload();
            } else if (ui_textentry) {
                ui_textentry_unload(0);
            } else if (ui_textbox) {
                ui_textbox_unload(0);
            } else if (ui_current == UI_SCREEN_IDLE) {
                Event_post(led_event_h, LED_EVENT_SIDELIGHT_DIS);
            } else {
                ui_transition(UI_SCREEN_IDLE);
            }
            // If we're doing a timeout, it's because nobody is paying
            //  attention. Therefore, it's likely safe to do a full refresh.
            epd_do_partial = 0;
            continue;
        }

        if (pop_events(&events, UI_EVENT_DO_SAVE)) {
            write_conf();
            if (!events) {
                continue;
            }
        }

        if (events & UI_EVENT_PAIRED) {
            Event_post(led_event_h, LED_EVENT_SIDELIGHT_EN);
            uint8_t remote_levels = 0;
            uint8_t starting_element = 0;

            // Unload everything, go to main pairing menu.
            if (ui_colorpicking) {
                ui_colorpicking_unload();
            }
            if (ui_textentry) {
                ui_textentry_unload(0);
            }
            if (ui_textbox) {
                ui_textbox_unload(0);
            }
            ui_transition(UI_SCREEN_PAIR_MENU);
            if (is_cbadge(paired_badge.badge_id)) {
                starting_element = 3;
            }

            // This is base 6 because of course it is.
            remote_levels = paired_badge.element_level[starting_element];
            remote_levels += paired_badge.element_level[starting_element+1]*6;
            remote_levels += paired_badge.element_level[starting_element+2]*36;
            set_badge_connected(paired_badge.badge_id, paired_badge.badge_type, remote_levels, paired_badge.handle);
            Event_post(led_event_h, LED_EVENT_FN_LIGHT);
            continue;
        }

        if (events & UI_EVENT_UNPAIRED) {
            // Unload everything, go to main menu.
            if (ui_colorpicking) {
                ui_colorpicking_unload();
            }
            if (ui_textentry) {
                ui_textentry_unload(0);
            }
            ui_transition(UI_SCREEN_MAINMENU);
            continue;
        }

        // NB: This order is very important:
        //     (colorpicking is not allowed during textentry)
        if (!ui_textentry && kb_active_key_masked == KB_F4_PICKER
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
            if (ui_current == UI_SCREEN_MISSIONS || ui_current == UI_SCREEN_PAIR_MENU) {
                epd_do_partial = 1;
                Event_post(ui_event_h, UI_EVENT_REFRESH);
            }
            Event_post(led_event_h, LED_EVENT_FN_LIGHT);

            // This correctly does nothing if we're not paired:
            serial_update_element();
        }

        if (ui_colorpicking) {
            ui_colorpicking_do(events);
        } else if (ui_textentry) {
            ui_textentry_do(events);
        } else if (ui_textbox) {
            ui_textbox_do(events);
        } else {
            // If neither of our "overlay" options are in use, then we follow
            //  a normal state flow:
            if (ui_current == UI_SCREEN_IDLE) {
                if ((events & UI_EVENT_KB_PRESS) && kb_active_key_masked == KB_OK) {
                    ui_transition(UI_SCREEN_MAINMENU);
                }
                ui_screensaver_do(events);
            } else if (ui_current >= UI_SCREEN_MAINMENU && ui_current <= UI_SCREEN_MENUSYSTEM_END) {
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

                case UI_SCREEN_PAIR_MENU:
                    ui_pair_menu_do(events);
                    break;
                case UI_SCREEN_PAIR_FILE:
                    ui_pair_files_do(events);
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
    adc_init();

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
}
