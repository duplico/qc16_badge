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

    qc16gr_drawImage(&ui_gr_context_portrait, &img_human_plate, 0, 0);

    Graphics_setFont(&ui_gr_context_portrait, &g_sFontCmtt28);
    Graphics_drawStringCentered(
            &ui_gr_context_portrait,
            "DUPLiCO",
            QC16_BADGE_NAME_LEN,
            64,
            UI_IDLE_HUD_Y-19-5,
            0
    );

    ui_draw_hud(&ui_gr_context_portrait, 1, 0, UI_IDLE_HUD_Y);

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
        break;
    case UI_SCREEN_PAIR_CB_INFO:
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

void ui_scan_do(UInt events) {
    if (pop_events(&events, UI_EVENT_BACK)) {
        ui_transition(UI_SCREEN_MAINMENU);
        return;
    }
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

    // Create and start the LED task; start the tail animation clock.
    led_init();

    // Now, let's look at the POST results.
    Graphics_clearDisplay(&ui_gr_context_landscape);
    Graphics_setFont(&ui_gr_context_landscape, &g_sFontfixed10x20);
    uint8_t y = 10;
    char disp_text[33] = {0x00,};

    sprintf(disp_text, "POST error count: %d", post_errors);
    Graphics_drawString(&ui_gr_context_landscape, disp_text, 99, 5, y, 1);
    y += 21;

    if (post_status_leds) {
        sprintf(disp_text, "  LED error: %d", post_status_leds);
        Graphics_drawString(&ui_gr_context_landscape, disp_text, 99, 5, y, 1);
        y += 21;
    }

    if (post_status_leds) {
        sprintf(disp_text, "  SPIFFS error: %d", post_status_spiffs);
        Graphics_drawString(&ui_gr_context_landscape, disp_text, 99, 5, y, 1);
        y += 21;
    }

    Graphics_flushBuffer(&ui_gr_context_landscape);

    ht16d_all_one_color(50, 50, 50);

    while (1) {
        Task_yield();
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
}
