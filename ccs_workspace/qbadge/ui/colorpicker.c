/*
 * colorpicker.c
 *
 *  Created on: Jun 17, 2019
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

#include <ui/leds.h>
#include <ui/ui.h>
#include <ui/keypad.h>
#include <ui/images.h>
#include <board.h>

uint8_t ui_colorpicker_cursor_pos = 0;
uint8_t ui_colorpicker_cursor_anim = 0;

void ui_colorpicking_wireframe() {
    Graphics_setBackgroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_BLACK);
    Graphics_setForegroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_WHITE);

    Graphics_Rectangle rect = {
        .xMin = 0,
        .yMin = UI_PICKER_TOP,
        .xMax = EPD_WIDTH-1,
        .yMax = EPD_HEIGHT-2
    };

    // Clear the color picker menu area
    Graphics_fillRectangle(&ui_gr_context_portrait, &rect);

    Graphics_setBackgroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_WHITE);
    Graphics_setForegroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_BLACK);

    Graphics_drawLineH(&ui_gr_context_portrait, 0, EPD_WIDTH-1, UI_PICKER_TOP);
    Graphics_drawLineH(&ui_gr_context_portrait, 0, EPD_WIDTH-1, UI_PICKER_TOP+1);
    Graphics_drawLineH(&ui_gr_context_portrait, 0, EPD_WIDTH-1, UI_PICKER_TOP+2);
    Graphics_drawLineH(&ui_gr_context_portrait, 0, EPD_WIDTH-1, UI_PICKER_TOP+3);

    // We've got a basically 170-px high frame to work with here.

    Graphics_drawLineH(&ui_gr_context_portrait, 1, EPD_WIDTH-1, EPD_HEIGHT-48);
    Graphics_drawLineH(&ui_gr_context_portrait, 1, EPD_WIDTH-1, EPD_HEIGHT-16);
    for (uint8_t i=1; i<128; i+=21) {
        Graphics_drawLineV(&ui_gr_context_portrait, i, EPD_HEIGHT-16, EPD_HEIGHT-48);
        Graphics_drawLine(&ui_gr_context_portrait, i+10, EPD_HEIGHT-16, (int32_t)((25.6*(i-1))/21), EPD_HEIGHT-1);
    }

    rect = (Graphics_Rectangle){10,UI_PICKER_TOP+4, EPD_WIDTH-11,UI_PICKER_TOP+4+64};

    if (ui_colorpicker_cursor_anim) {
        Graphics_drawRectangle(&ui_gr_context_portrait, &rect);
    } else {
        // Draw an arrow over the relevant color:
        uint8_t arrow_x = (21*ui_colorpicker_cursor_pos);
        Graphics_drawLine(&ui_gr_context_portrait, arrow_x, EPD_HEIGHT-64, arrow_x+10, EPD_HEIGHT-48);
        Graphics_drawLine(&ui_gr_context_portrait, arrow_x+21, EPD_HEIGHT-64, arrow_x+11, EPD_HEIGHT-48);
    }
}

void ui_colorpicking_load() {
    ui_colorpicking = 1;

    Graphics_Rectangle rect;

    Graphics_setBackgroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_BLACK);
    Graphics_setForegroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_WHITE);

    // Fade out the background.
    for (int16_t i=0; i<UI_PICKER_TOP*2; i+=2) {
        Graphics_drawLine(&ui_gr_context_portrait, 0, i, i, 0);
    }

    // Clear the color picker "tab" area
    rect = (Graphics_Rectangle){48,UI_PICKER_TOP-32,
                                80,UI_PICKER_TOP};
    Graphics_fillRectangle(&ui_gr_context_portrait, &rect);

    Graphics_setBackgroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_WHITE);
    Graphics_setForegroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_BLACK);

    // Draw the color picker "tab" and the color picker icon.
    Graphics_drawImage(&ui_gr_context_portrait, &picker1BPP_UNCOMP, 48, UI_PICKER_TOP-32);
    Graphics_drawRectangle(&ui_gr_context_portrait, &rect);

    // This will draw everything else:
    Event_post(ui_event_h, UI_EVENT_REFRESH);
    Event_post(led_event_h, LED_EVENT_SHOW_UPCONF);
}

void ui_colorpicking_unload() {
    ui_colorpicking = 0;
    Event_post(led_event_h, LED_EVENT_HIDE_UPCONF);
    Event_post(ui_event_h, UI_EVENT_REFRESH);
    epd_do_partial = 1;

}

void ui_colorpicking_do(UInt events) {
    if (pop_events(&events, UI_EVENT_REFRESH)) {
        ui_colorpicking_wireframe();
        epd_do_partial = 1;
        Graphics_flushBuffer(&ui_gr_context_landscape);
    }

    if (pop_events(&events, UI_EVENT_KB_PRESS)) {
        switch(kb_active_key_masked) {
        case BTN_BACK:
            ui_colorpicking_unload();
            break;
        case BTN_RED:
            memcpy(&led_tail_anim_current.colors[ui_colorpicker_cursor_pos], &led_rainbow_colors[0], sizeof(rgbcolor16_t));
            Event_post(led_event_h, LED_EVENT_SHOW_UPCONF);
            break;
        case BTN_ORG:
            memcpy(&led_tail_anim_current.colors[ui_colorpicker_cursor_pos], &led_rainbow_colors[1], sizeof(rgbcolor16_t));
            Event_post(led_event_h, LED_EVENT_SHOW_UPCONF);
            break;
        case BTN_YEL:
            memcpy(&led_tail_anim_current.colors[ui_colorpicker_cursor_pos], &led_rainbow_colors[2], sizeof(rgbcolor16_t));
            Event_post(led_event_h, LED_EVENT_SHOW_UPCONF);
            break;
        case BTN_GRN:
            memcpy(&led_tail_anim_current.colors[ui_colorpicker_cursor_pos], &led_rainbow_colors[3], sizeof(rgbcolor16_t));
            Event_post(led_event_h, LED_EVENT_SHOW_UPCONF);
            break;
        case BTN_BLU:
            memcpy(&led_tail_anim_current.colors[ui_colorpicker_cursor_pos], &led_rainbow_colors[4], sizeof(rgbcolor16_t));
            Event_post(led_event_h, LED_EVENT_SHOW_UPCONF);
            break;
        case BTN_PUR:
            memcpy(&led_tail_anim_current.colors[ui_colorpicker_cursor_pos], &led_rainbow_colors[5], sizeof(rgbcolor16_t));
            Event_post(led_event_h, LED_EVENT_SHOW_UPCONF);
            break;
        case BTN_F1_LOCK:
            ht16d_all_one_color(255, 0, 0);
            break;
        case BTN_F2_COIN:
            break;
        case BTN_F3_CAMERA:
            break;
        case BTN_UP: // This is "right"
            if (ui_colorpicker_cursor_pos == 5)
                ui_colorpicker_cursor_pos = 0;
            else
                ui_colorpicker_cursor_pos++;
            Event_post(ui_event_h, UI_EVENT_REFRESH);
            break;
        case BTN_DOWN: // "left"
            if (ui_colorpicker_cursor_pos == 0)
                ui_colorpicker_cursor_pos = 5;
            else
                ui_colorpicker_cursor_pos--;
            Event_post(ui_event_h, UI_EVENT_REFRESH);
            break;
        case BTN_LEFT: // "up"
            if (!ui_colorpicker_cursor_anim) {
                ui_colorpicker_cursor_anim = 1;
                Event_post(ui_event_h, UI_EVENT_REFRESH);
            }
            break;
        case BTN_RIGHT: // "down"
            if (ui_colorpicker_cursor_anim) {
                ui_colorpicker_cursor_anim = 0;
                Event_post(ui_event_h, UI_EVENT_REFRESH);
            }
            break;
        case BTN_OK:
            break;

        }
    }
}
