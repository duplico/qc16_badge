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
#include <stdio.h>

#include <ti/grlib/grlib.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/drivers/PIN.h>

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

void ui_draw_top_bar() {
    // Draw the top bar.
    Graphics_drawLine(&ui_gr_context_landscape, 0, 28, 295, 28);

    // Draw the battery symbol.
    // Determine the battery voltage.
    // 0.5 V is the minimum. 3.0V is the "maximum"
    // They're probably just about empty when we get to 1.8 V.
    Graphics_Rectangle rect;
    rect.xMin = 248;
    rect.xMax = 291;
    rect.yMin = 2;
    rect.yMax = 18;
    Graphics_drawRectangle(&ui_gr_context_landscape, &rect);
    rect.xMin = 291;
    rect.xMax = 295;
    rect.yMin = 6;
    rect.yMax = 13;
    Graphics_drawRectangle(&ui_gr_context_landscape, &rect);

    Graphics_setFont(&ui_gr_context_landscape, &g_sFontCmss16b);
    Graphics_setFont(&ui_gr_context_landscape, &g_sFontFixed6x8);
    char bat_text[5] = "3.0V";
    sprintf(bat_text, "%d.%dV", vbat_out_uvolts/1000000, (vbat_out_uvolts/100000) % 10);
    Graphics_drawStringCentered(&ui_gr_context_landscape, (int8_t *) bat_text, 4, 248 + (291-248)/2, 2 + (18-2)/2, 0);

    Graphics_drawString(&ui_gr_context_landscape, "QBADGE", 6, 0, 0, 0);

    Graphics_flushBuffer(&ui_gr_context_landscape);

    // Is our agent around? Draw the agent symbol.

    // Are there any handlers around? Draw the handler symbol.


}

void ui_draw_main_menu() {
    // Clear the buffer.
    Graphics_clearDisplay(&ui_gr_context_landscape);

    ui_draw_top_bar();

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

void ui_task_fn(UArg a0, UArg a1) {
    UInt events;

    storage_init();

    ui_transition(UI_SCREEN_IDLE);

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

        if (kb_active_key_masked == BTN_ROT
                && ui_current != UI_SCREEN_IDLE
                && !ui_colorpicking
                && pop_events(&events, UI_EVENT_KB_PRESS)) {
            // The rotate button is pressed, and we're NOT in one of the
            //  portrait modes:
            epd_flip();
            Graphics_flushBuffer(&ui_gr_context_landscape);
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
                ui_screensaver_do(events);
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
