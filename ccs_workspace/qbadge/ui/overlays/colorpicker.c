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

#include <badge.h>
#include <ui/leds.h>
#include <ui/ui.h>
#include <ui/keypad.h>
#include <ui/images.h>
#include <ui/graphics.h>
#include <board.h>
#include "overlays.h"

uint8_t ui_colorpicker_cursor_pos = 0;
uint8_t ui_colorpicker_cursor_anim = 0;

void ui_colorpicking_wireframe() {
    // Switch to the "clearing" colors
    Graphics_setBackgroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_BLACK);
    Graphics_setForegroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_WHITE);

    Graphics_Rectangle rect = {
        .xMin = 0,
        .yMin = UI_PICKER_TOP,
        .xMax = EPD_WIDTH-1,
        .yMax = EPD_HEIGHT-1
    };

    // Clear the color picker menu area
    fillRectangle(&ui_gr_context_portrait, &rect);

    // Switch to the "drawing" colors
    Graphics_setBackgroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_WHITE);
    Graphics_setForegroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_BLACK);

    // Thick line at the top of the drawer.
    Graphics_drawLineH(&ui_gr_context_portrait, 0, EPD_WIDTH-1, UI_PICKER_TOP);
    Graphics_drawLineH(&ui_gr_context_portrait, 0, EPD_WIDTH-1, UI_PICKER_TOP+1);
    Graphics_drawLineH(&ui_gr_context_portrait, 0, EPD_WIDTH-1, UI_PICKER_TOP+2);
    Graphics_drawLineH(&ui_gr_context_portrait, 0, EPD_WIDTH-1, UI_PICKER_TOP+3);

    // Draw the boxes that hold our non-LED color representations
    Graphics_drawLineH(&ui_gr_context_portrait, 1, EPD_WIDTH-1, EPD_HEIGHT-UI_PICKER_COLORBOX_H-UI_PICKER_COLORBOX_BPAD);
    Graphics_drawLineH(&ui_gr_context_portrait, 1, EPD_WIDTH-1, EPD_HEIGHT-UI_PICKER_COLORBOX_BPAD);

    uint8_t count = led_tail_anim_color_counts[led_tail_anim_current.type];
    if (count) {
        uint16_t width = EPD_WIDTH/count;
        for (uint8_t i=0; i<count; i++) {
            if (!ui_colorpicker_cursor_anim && ui_colorpicker_cursor_pos == i) {
                // draw an arrow:
                uint8_t arrow_x = ((width*i)+(width/2)-10);
                Graphics_drawLine(&ui_gr_context_portrait, arrow_x, EPD_HEIGHT-64, arrow_x+10, EPD_HEIGHT-48);
                Graphics_drawLine(&ui_gr_context_portrait, arrow_x+21, EPD_HEIGHT-64, arrow_x+11, EPD_HEIGHT-48);
            }
            rect = (Graphics_Rectangle) {width*i, EPD_HEIGHT-16, width*(i+1), EPD_HEIGHT-48};
            Graphics_drawRectangle(&ui_gr_context_portrait, &rect);
        }
    }

    // Draw the icon for the animation type currently selected:
    // TODO: Make sure there can't be an overrun here:
    qc16gr_drawImage(&ui_gr_context_portrait, image_anim_type_buttons[led_tail_anim_current.type], 32, UI_PICKER_TOP+4);

    if (ui_colorpicker_cursor_anim) {
        // If we have the animation type selected, draw a box around its area:
        // It's approx 64x64
        // TODO: change this representation.
        rect = (Graphics_Rectangle){10,UI_PICKER_TOP+4, EPD_WIDTH-11,UI_PICKER_TOP+4+64};
        Graphics_drawRectangle(&ui_gr_context_portrait, &rect);
    }

}

void ui_colorpicking_load() {
    ui_colorpicking = 1;

    Graphics_Rectangle rect;
    rect.xMin = 0;
    rect.yMin = 0;
    rect.xMax = EPD_WIDTH-1;
    rect.yMax = UI_PICKER_TOP;

    // Fade out the background
    fadeRectangle(&ui_gr_context_portrait, &rect);

    // Clear the color picker "tab" area
    rect = (Graphics_Rectangle){64-(UI_PICKER_TAB_W/2), UI_PICKER_TOP-UI_PICKER_TAB_H,
                                64+(UI_PICKER_TAB_W/2), UI_PICKER_TOP};
    fillRectangle(&ui_gr_context_portrait, &rect);

    // Switch to the "drawing" colors
    Graphics_setBackgroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_WHITE);
    Graphics_setForegroundColor(&ui_gr_context_portrait, GRAPHICS_COLOR_BLACK);

    // Draw the color picker "tab" and the color picker icon.
    qc16gr_drawImage(&ui_gr_context_portrait, &img_picker, 64-(img_picker.xSize/2), UI_PICKER_TOP-UI_PICKER_TAB_H);
    Graphics_drawRectangle(&ui_gr_context_portrait, &rect);

    // This will draw everything else:
    Event_post(ui_event_h, UI_EVENT_REFRESH);
    Event_post(led_event_h, LED_EVENT_SHOW_UPCONF);

    // TODO: if the selected color is out of range, do this:
    ui_colorpicker_cursor_pos = 0;
}

void ui_colorpicking_unload() {
    ui_colorpicking = 0;
    Event_post(led_event_h, LED_EVENT_HIDE_UPCONF);
    Event_post(ui_event_h, UI_EVENT_REFRESH);
    epd_do_partial = 1;
    write_anim_curr();
}

void ui_colorpicking_colorbutton() {
    uint8_t color_index;

    // If the button we pressed corresponds to
    switch(kb_active_key_masked) {
    case BTN_RED:
        color_index = 0;
        break;
    case BTN_ORG:
        color_index = 1;
        break;
    case BTN_YEL:
        color_index = 2;
        break;
    case BTN_GRN:
        color_index = 3;
        break;
    case BTN_BLU:
        color_index = 4;
        break;
    case BTN_PUR:
        color_index = 5;
        break;
    }

    // TODO: This isn't exactly what we want.

    if (memcmp(&led_tail_anim_current.colors[ui_colorpicker_cursor_pos], &led_rainbow_colors[color_index], sizeof(rgbcolor16_t)) == 0) {
        memcpy(&led_tail_anim_current.colors[ui_colorpicker_cursor_pos], &led_off, sizeof(rgbcolor16_t));
    } else if (memcmp(&led_tail_anim_current.colors[ui_colorpicker_cursor_pos], &led_off, sizeof(rgbcolor16_t)) == 0) {
        memcpy(&led_tail_anim_current.colors[ui_colorpicker_cursor_pos], &led_white, sizeof(rgbcolor16_t));
    } else {
        memcpy(&led_tail_anim_current.colors[ui_colorpicker_cursor_pos], &led_rainbow_colors[color_index], sizeof(rgbcolor16_t));
    }

    Event_post(led_event_h, LED_EVENT_SHOW_UPCONF);

    if (led_tail_anim_current.type == LED_TAIL_ANIM_TYPE_ON) {
        led_tail_start_anim();
    }

}

void ui_colorpicking_do(UInt events) {
    if (pop_events(&events, UI_EVENT_REFRESH)) {
        ui_colorpicking_wireframe();
        epd_do_partial = 1;
        Graphics_flushBuffer(&ui_gr_context_landscape);
    }

    if (pop_events(&events, UI_EVENT_KB_PRESS)) {
        if (ui_colorpicker_cursor_anim) {
            // Animation selection is highlighted

            switch(kb_active_key_masked) {
            case BTN_RIGHT:
                led_tail_anim_type_next();
                ui_colorpicker_cursor_pos = 0;
                Event_post(led_event_h, LED_EVENT_SHOW_UPCONF);
                Event_post(ui_event_h, UI_EVENT_REFRESH);
                break;
            case BTN_LEFT:
                led_tail_anim_type_prev();
                ui_colorpicker_cursor_pos = 0;
                Event_post(led_event_h, LED_EVENT_SHOW_UPCONF);
                Event_post(ui_event_h, UI_EVENT_REFRESH);
                break;
            case BTN_DOWN:
                if (led_tail_anim_color_counts[led_tail_anim_current.type]) {
                    ui_colorpicker_cursor_anim = 0;
                    Event_post(ui_event_h, UI_EVENT_REFRESH);
                }
                break;
            }

        } else {
            // Color selection is highlighted.

            switch(kb_active_key_masked) {

            case BTN_RED:
            case BTN_ORG:
            case BTN_YEL:
            case BTN_GRN:
            case BTN_BLU:
            case BTN_PUR:
                // Any color button:
                ui_colorpicking_colorbutton();
                break;
            case BTN_RIGHT:
                if (ui_colorpicker_cursor_pos == led_tail_anim_color_counts[led_tail_anim_current.type])
                    ui_colorpicker_cursor_pos = 0;
                else
                    ui_colorpicker_cursor_pos++; // TODO: this can overrun if max==1
                Event_post(ui_event_h, UI_EVENT_REFRESH);
                break;
            case BTN_LEFT:
                if (ui_colorpicker_cursor_pos == 0)
                    ui_colorpicker_cursor_pos = led_tail_anim_color_counts[led_tail_anim_current.type];
                ui_colorpicker_cursor_pos--; // TODO: this can overrun if max==1
                Event_post(ui_event_h, UI_EVENT_REFRESH);
                break;
            case BTN_UP:
                ui_colorpicker_cursor_anim = 1;
                Event_post(ui_event_h, UI_EVENT_REFRESH);
                break;
            }
        }

        // Universal ones:
        switch(kb_active_key_masked) {
        case BTN_BACK:
            ui_colorpicking_unload();
            break;
        case BTN_F1_LOCK:
            break;
        case BTN_F2_COIN:
            break;
        case BTN_F3_CAMERA:
            break;
        case BTN_OK:
            break;
        }
        // TODO:
        // Call this if needed:
        //led_tail_start_anim();
    }
}
