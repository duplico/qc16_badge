/*
 * ui.c
 *
 *  Created on: Jun 3, 2019
 *      Author: george
 */
#include <qbadge.h>
#include <queercon_drivers/epd.h>
#include <queercon_drivers/ht16d35b.h>
#include <queercon_drivers/storage.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <ti/grlib/grlib.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/drivers/PIN.h>

#include "queercon_drivers/storage.h"
#include <third_party/spiffs/spiffs.h>

#include <ui/images.h>
#include <ui/colorpicker.h>
#include "leds.h"
#include "ui/ui.h"
#include "ui/keypad.h"
#include "board.h"

Event_Handle ui_event_h;
Graphics_Context ui_gr_context_landscape;
Graphics_Context ui_gr_context_portrait;

#define UI_STACKSIZE 2048
Task_Struct ui_task;
uint8_t ui_task_stack[UI_STACKSIZE];

uint8_t ui_current = UI_SCREEN_STORY1;
uint8_t ui_colorpicking = 0;
uint8_t ui_textentry = 0;

#define TOPBAR_HEIGHT 36
#define TOPBAR_ICON_HEIGHT 24
#define TOPBAR_ICON_WIDTH 24
#define TOPBAR_TEXT_WIDTH 18
#define TOPBAR_TEXT_HEIGHT (TOPBAR_HEIGHT - TOPBAR_ICON_HEIGHT)
#define TOPBAR_SEG_WIDTH (TOPBAR_ICON_WIDTH + TOPBAR_TEXT_WIDTH)
#define TOPBAR_SEG_PAD 2
#define TOPBAR_SEG_WIDTH_PADDED (TOPBAR_ICON_WIDTH + TOPBAR_TEXT_WIDTH + TOPBAR_SEG_PAD)
#define TOPBAR_SUB_WIDTH TOPBAR_SEG_WIDTH
#define TOPBAR_SUB_HEIGHT (TOPBAR_HEIGHT - TOPBAR_ICON_HEIGHT - 1)

#define TOPBAR_HEADSUP_START (3*TOPBAR_SEG_WIDTH_PADDED)

#define BATTERY_X (EPD_HEIGHT-TOPBAR_ICON_WIDTH-1)
#define BATTERY_ANODE_WIDTH 3
#define BATTERY_ANODE_HEIGHT 6
#define BATTERY_BODY_WIDTH (TOPBAR_ICON_WIDTH-BATTERY_ANODE_WIDTH)
#define BATTERY_BODY_HEIGHT 15
#define BATTERY_BODY_Y0 ((TOPBAR_ICON_HEIGHT-BATTERY_BODY_HEIGHT)/2)
#define BATTERY_BODY_Y1 (BATTERY_BODY_Y0+BATTERY_BODY_HEIGHT)
#define BATTERY_ANODE_Y0 ((TOPBAR_ICON_HEIGHT-BATTERY_ANODE_HEIGHT)/2)

#define BATTERY_SEGMENT_PAD 1
#define BATTERY_SEGMENT_WIDTH ((BATTERY_BODY_WIDTH/3)-BATTERY_SEGMENT_PAD)

#define BATTERY_LOW_X0 BATTERY_X
#define BATTERY_MID_X0 (BATTERY_X + BATTERY_SEGMENT_WIDTH + BATTERY_SEGMENT_PAD)
#define BATTERY_HIGH_X0 (BATTERY_X + 2*(BATTERY_SEGMENT_WIDTH + BATTERY_SEGMENT_PAD))

#define VBAT_FULL_2DOT 9
#define VBAT_MID_2DOT 5
#define VBAT_LOW_2DOT 2

#define TOP_BAR_LOCKS 0
#define TOP_BAR_COINS 1
#define TOP_BAR_CAMERAS 2

// TODO: write draw_top_bar_remote_element_icons()

void ui_draw_top_bar_local_element_icons() {
    // TODO: consider making this a global or heap var that we share everywhere.
    Graphics_Rectangle rect;

    // TODO: Add a pad here, explicitly:

    for (uint8_t i=0; i<3; i++) {
        uint16_t icon_x = i*TOPBAR_SEG_WIDTH_PADDED;
        uint16_t icon_y = 0;
        uint16_t text_x = i*TOPBAR_SEG_WIDTH_PADDED+TOPBAR_ICON_WIDTH;
        uint16_t text_y = 8;
        uint16_t bar_x0 = i*TOPBAR_SEG_WIDTH_PADDED;
        uint16_t bar_y0 = TOPBAR_ICON_HEIGHT+1;
        uint16_t bar_x1 = i*TOPBAR_SEG_WIDTH_PADDED+TOPBAR_SUB_WIDTH-1;
        uint16_t bar_y1 = TOPBAR_ICON_HEIGHT+TOPBAR_SUB_HEIGHT-1;
        uint16_t number;
        int8_t bar_level = -1;
        int8_t bar_capacity = -1;
        tImage *icon_img;

        switch(i) {
        case TOP_BAR_LOCKS:
            icon_img = &locks1BPP_UNCOMP;
            bar_level = 1; // TODO: read
            bar_capacity = 2;
            text_x -= 2; // Unpad.
            number = my_conf.element_qty[i]; // TODO
            break;
        case TOP_BAR_COINS:
            icon_img = &coins1BPP_UNCOMP;
            bar_level = 2; // TODO: read
            bar_capacity = 5;
            number = my_conf.element_qty[i]; // TODO
            break;
        case TOP_BAR_CAMERAS:
            icon_img = &cameras1BPP_UNCOMP;
            bar_level = 3; // TODO: read
            bar_capacity = 4;
            number = my_conf.element_qty[i]; // TODO
            break;
        }

        Graphics_drawImage(&ui_gr_context_landscape, icon_img, icon_x, icon_y);

        if (bar_capacity > 0) {
            // The "fullness" bar, if applicable:
            rect.xMin = bar_x0;
            rect.xMax = bar_x1;
            rect.yMin = bar_y0;
            rect.yMax = bar_y1;
            Graphics_drawRectangle(&ui_gr_context_landscape, &rect);
        }
        Graphics_setFont(&ui_gr_context_landscape, &g_sFontCmss16);

        Graphics_drawString(&ui_gr_context_landscape, "123", 3, i*TOPBAR_SEG_WIDTH_PADDED+TOPBAR_ICON_WIDTH, 4, 0);
    }
}

void ui_draw_top_bar_qbadge_headsup_icons() {
    // TODO: consider making this a global or heap var that we share everywhere.
    Graphics_Rectangle rect;

    // TODO: Add a pad here, explicitly:

    for (uint8_t i=0; i<3; i++) {
        uint16_t icon_x = TOPBAR_HEADSUP_START + i*TOPBAR_SEG_WIDTH_PADDED;
        uint16_t icon_y = 0;
        tImage *icon_img;
        switch(i) {
        case 0: // agent
            icon_img = &agent1BPP_UNCOMP;
            break;
        case 1: // handlers
            icon_img = &handler1BPP_UNCOMP;
            break;
        case 2: // scan
            icon_img = &radar1BPP_UNCOMP;
            break;
        }

        Graphics_drawImage(&ui_gr_context_landscape, icon_img, icon_x, icon_y);
    }
}

void ui_draw_top_bar_battery_life() {
    Graphics_Rectangle rect;
    rect.xMin = BATTERY_X;
    rect.xMax = BATTERY_X+BATTERY_BODY_WIDTH;
    rect.yMin = BATTERY_BODY_Y0;
    rect.yMax = BATTERY_BODY_Y1;
    Graphics_drawRectangle(&ui_gr_context_landscape, &rect);

    rect.xMin = BATTERY_X+BATTERY_BODY_WIDTH;
    rect.xMax = BATTERY_X+BATTERY_BODY_WIDTH+BATTERY_ANODE_WIDTH;
    rect.yMin = BATTERY_ANODE_Y0;
    rect.yMax = BATTERY_ANODE_Y0 + BATTERY_ANODE_HEIGHT;
    Graphics_drawRectangle(&ui_gr_context_landscape, &rect);

    Graphics_setFont(&ui_gr_context_landscape, &g_sFontFixed6x8);
    char bat_text[5] = "3.0V";
    sprintf(bat_text, "%d.%dV", vbat_out_uvolts/1000000, (vbat_out_uvolts/100000) % 10);
    Graphics_drawStringCentered(&ui_gr_context_landscape, (int8_t *) bat_text, 4, BATTERY_X + TOPBAR_ICON_WIDTH/2, TOPBAR_ICON_HEIGHT + TOPBAR_TEXT_HEIGHT/2 - 1, 0);

    // Now do the graphics part:
    if (vbat_out_uvolts/1000000 > 2 || (vbat_out_uvolts/1000000 == 2 && (vbat_out_uvolts/100000) % 10 >= VBAT_FULL_2DOT)) {
        // full
        rect.xMin = BATTERY_HIGH_X0+BATTERY_SEGMENT_PAD;
        rect.xMax = BATTERY_HIGH_X0+BATTERY_SEGMENT_WIDTH-1;
        rect.yMin = BATTERY_BODY_Y0+BATTERY_SEGMENT_PAD;
        rect.yMax = BATTERY_BODY_Y1-BATTERY_SEGMENT_PAD-2;
        Graphics_fillRectangle(&ui_gr_context_landscape, &rect);
    }
    if (vbat_out_uvolts/1000000 > 2 || (vbat_out_uvolts/1000000 == 2 && (vbat_out_uvolts/100000 % 10 >= VBAT_MID_2DOT))) {
        rect.xMin = BATTERY_MID_X0+BATTERY_SEGMENT_PAD;
        rect.xMax = BATTERY_MID_X0+BATTERY_SEGMENT_WIDTH;
        rect.yMin = BATTERY_BODY_Y0+BATTERY_SEGMENT_PAD;
        rect.yMax = BATTERY_BODY_Y1-BATTERY_SEGMENT_PAD-2;
        Graphics_fillRectangle(&ui_gr_context_landscape, &rect);
    }
    if (vbat_out_uvolts/1000000 > 2 || (vbat_out_uvolts/1000000 == 2 && (vbat_out_uvolts/100000 % 10 >= VBAT_LOW_2DOT))) {
        rect.xMin = BATTERY_LOW_X0+BATTERY_SEGMENT_PAD+1;
        rect.xMax = BATTERY_LOW_X0+BATTERY_SEGMENT_WIDTH;
        rect.yMin = BATTERY_BODY_Y0+BATTERY_SEGMENT_PAD;
        rect.yMax = BATTERY_BODY_Y1-BATTERY_SEGMENT_PAD-2;
        Graphics_fillRectangle(&ui_gr_context_landscape, &rect);
    }
    if (vbat_out_uvolts/1000000 < 2 || (vbat_out_uvolts/1000000 == 2 && (vbat_out_uvolts/100000) % 10 < VBAT_LOW_2DOT)) {
        // ABOUT TO RUN OUT!!!
        Graphics_drawString(&ui_gr_context_landscape, ":-(", 3, BATTERY_X+2, BATTERY_BODY_Y0+4, 0);

    }
}

void ui_draw_top_bar() {
    // Draw the top bar.
    Graphics_drawLine(&ui_gr_context_landscape, 0, TOPBAR_HEIGHT, 295, TOPBAR_HEIGHT);

    ui_draw_top_bar_battery_life();
    ui_draw_top_bar_local_element_icons();
    ui_draw_top_bar_qbadge_headsup_icons();
    // Is our agent around? Draw the agent symbol.

    // Are there any handlers around? Draw the handler symbol.


}

void ui_draw_main_menu() {
    // Clear the buffer.
    Graphics_clearDisplay(&ui_gr_context_landscape);

    ui_draw_top_bar();

    Graphics_flushBuffer(&ui_gr_context_landscape);
    // Draw the menu itself

}

UInt pop_events(UInt *events_ptr, UInt events_to_check) {
    UInt in_events;

    in_events = (*events_ptr) & events_to_check;
    (*events_ptr) &= ~events_to_check;
    return in_events;
}

void ui_draw_screensaver() {
    Graphics_clearDisplay(&ui_gr_context_portrait);

    Graphics_setFont(&ui_gr_context_portrait, &g_sFontCmss16b);
    Graphics_drawStringCentered(
            &ui_gr_context_portrait,
            "- queercon 16 -",
            15,
            64,
            UI_IDLE_BLOCK0_HEIGHT_PX / 2,
            0
    );

    Graphics_setFont(&ui_gr_context_portrait, &g_sFontCmss48b);
    Graphics_drawStringCentered(
            &ui_gr_context_portrait,
            "UBER",
            4,
            64,
            UI_IDLE_BLOCK1_TOP_PX + (UI_IDLE_BLOCK1_HEIGHT_PX / 2),
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

    Graphics_drawImage(&ui_gr_context_portrait, &example_photo1BPP_UNCOMP, 0, UI_IDLE_PHOTO_TOP);

    Graphics_flushBuffer(&ui_gr_context_portrait);
}

void ui_transition(uint8_t destination) {
    if (ui_current == destination) {
        return; // Nothing to do.
    }

    ui_current = destination;
    Event_post(ui_event_h, UI_EVENT_REFRESH);
}

void ui_textentry_load() {

}

void ui_textentry_do(UInt events) {

}

void ui_textentry_unload() {

}

void ui_screensaver_do(UInt events) {
    if (pop_events(&events, UI_EVENT_REFRESH)) {
        ui_draw_screensaver();
    }
}

void ui_mainmenu_do(UInt events) {
    switch(events) {
    case UI_EVENT_REFRESH:
        ui_draw_main_menu();
    case UI_EVENT_KB_PRESS:
        break;
    }
}

uint8_t conf_file_exists() {
    volatile int32_t status;
    spiffs_stat stat;
    status = SPIFFS_stat(&fs, "/qbadge/conf", &stat);
    if (status == SPIFFS_OK && stat.size == sizeof(my_conf)) {
        return 1;
    } else if (status == SPIFFS_OK) {
        status = SPIFFS_remove(&fs, "/qbadge/conf");
    }
    // TODO: Validate more?
    // We should create it if we got SPIFFS_ERR_NOT_FOUND...
    return 0;
}

void load_conf() {
    // TODO: Make sure this worked.
    spiffs_file conf_file;
    conf_file = SPIFFS_open(&fs, "/qbadge/conf", SPIFFS_O_RDONLY, 0);
    SPIFFS_read(&fs, conf_file, (uint8_t *) (&my_conf), sizeof(my_conf));
    SPIFFS_close(&fs, conf_file);
}

void write_conf() {
    // TODO: Error handling.
    spiffs_file conf_file;
    conf_file = SPIFFS_open(&fs, "/qbadge/conf", SPIFFS_O_CREAT | SPIFFS_O_WRONLY, 0);
    SPIFFS_write(&fs, conf_file, (uint8_t *) (&my_conf), sizeof(my_conf));
    SPIFFS_close(&fs, conf_file);
}

void init_conf() {
    my_conf.badge_id = QBADGE_ID_UNASSIGNED;
    memset(my_conf.element_level, 0x00, 3);
    memset(my_conf.element_level_progress, 0x00, 3);
    memset(my_conf.element_qty, 0x00, 6);
    my_conf.initialized = 0;
    write_conf();
}

void ui_task_fn(UArg a0, UArg a1) {
    UInt events;

    storage_init();

    if (conf_file_exists()) {
        load_conf();
    } else {
        init_conf();
    }


//    ui_transition(UI_SCREEN_IDLE);
    ui_transition(UI_SCREEN_MAINMENU);
    uint8_t brightness = 0x10;

    while (1) {
        events = Event_pend(ui_event_h, Event_Id_NONE, ~Event_Id_NONE,  UI_AUTOREFRESH_TIMEOUT);

        if (!events) {
            // This is a timeout.
            if (ui_colorpicking) {
                ui_colorpicking_unload();
            } else if (ui_textentry) {
                ui_textentry_unload();
            } else {
                ui_transition(UI_SCREEN_IDLE);
            }
            // If we're doing a timeout, it's because nobody is paying
            //  attention. Therefore, it's likely safe to do a full refresh.
            epd_do_partial = 0;
            continue;
        }

        // NB: This order is very important:
        if (kb_active_key_masked == BTN_F4_PICKER
                && pop_events(&events, UI_EVENT_KB_PRESS)) {
            if (ui_colorpicking) {
                ui_colorpicking_unload();
            } else {
                ui_colorpicking_load();
            }
        }

        // TODO: just have a flag that tracks whether we're portrait or landscape
        if (kb_active_key_masked == BTN_ROT
                && ui_current != UI_SCREEN_IDLE
                && !ui_colorpicking
                && pop_events(&events, UI_EVENT_KB_PRESS)) {
            // The rotate button is pressed, and we're NOT in one of the
            //  portrait modes:
            epd_flip();
            Graphics_flushBuffer(&ui_gr_context_landscape);
        } else if (kb_active_key_masked == BTN_ROT) {
            // TODO: delete this
            led_sidelight_set_level(0x10);
        }

        if (ui_colorpicking) {
            ui_colorpicking_do(events);
        } else if (ui_textentry) {
            ui_textentry_do(events);
        } else {
            // If neither of our "overlay" options are in use, then we follow
            //  a normal state flow:
            switch(ui_current) {
            case UI_SCREEN_IDLE:
                if ((events & UI_EVENT_KB_PRESS) && kb_active_key_masked == BTN_UP) {
                    brightness+=10;
                    if (brightness > HT16D_BRIGHTNESS_MAX) {
                        brightness = HT16D_BRIGHTNESS_MAX;
                    }
                    ht16d_set_global_brightness(brightness);
                } else if ((events & UI_EVENT_KB_PRESS) && kb_active_key_masked == BTN_DOWN) {
                    if (brightness < 10) {
                        brightness = 0;
                    } else {
                        brightness -= 0;
                    }
                    ht16d_set_global_brightness(brightness);
                } else if ((events & UI_EVENT_KB_PRESS) && kb_active_key_masked == BTN_OK) {
                    ui_transition(UI_SCREEN_MAINMENU);
                }
                ui_screensaver_do(events);
                break;
            case UI_SCREEN_MAINMENU:
                ui_mainmenu_do(events);
                break;
            }

        }
    }
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
