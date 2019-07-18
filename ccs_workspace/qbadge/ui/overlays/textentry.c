/*
 * textentry.c
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

#include <ui/ui.h>
#include <ui/graphics.h>
#include <ui/images.h>
#include <ui/keypad.h>

#include <badge.h>
#include <board.h>
#include <ui/layout.h>

uint8_t textentry_len;
char *textentry_dest;
char *textentry_buf;
uint8_t textentry_cursor = 0;

void ui_textentry_load(char *dest, uint8_t len) {
    if (len > 40) {
        len = 40; // TODO: const/define for this
    }
    textentry_len = len;
    textentry_buf = malloc(len);
    textentry_dest = dest;
    textentry_cursor = 0;
    memset(textentry_buf, 0x00, len);
    strncpy(textentry_buf, textentry_dest, textentry_len);
    ui_textentry = 1;

    // Fade out everything.
    fadeRectangle_xy(&ui_gr_context_landscape, 0, 0, 295, 127); // TODO: consts

    Event_post(ui_event_h, UI_EVENT_REFRESH);
}

void ui_textentry_unload(uint8_t save) {
    ui_textentry = 0;
    if (save && textentry_buf[0] && textentry_dest) {
        // If we're supposed to copy this back to the destination,
        //  and if text was actually entered, then save it.
        strncpy(textentry_dest, textentry_buf, textentry_len);
        Event_post(ui_event_h, UI_EVENT_TEXT_READY);
    } else {
        Event_post(ui_event_h, UI_EVENT_TEXT_CANCELED);
    }

    free(textentry_buf);
    // TODO: Is this not happening?
    // It's not, because of the wrong thing we're doing with switching rather than popping
    Event_post(ui_event_h, UI_EVENT_REFRESH);
}

// TODO: Declare elsewhere
extern const tFont g_sFontfixed10x20;

// TODO: Add instructions?

void ui_textentry_draw() {
    uint16_t entry_width = textentry_len * 10;
    uint16_t txt_left = 148 - (entry_width/2);
    uint16_t txt_top = 64 - 10;

    Graphics_Rectangle rect = {
        .xMin = txt_left-3, .yMin = txt_top-3,
        .xMax = txt_left + entry_width + 3, .yMax = txt_top + 22,
    };

    // TODO: Fix frame layout:
    // Draw a little frame.
    Graphics_drawRectangle(&ui_gr_context_landscape, &rect);
    rect.xMin++; rect.yMin++; rect.xMax--; rect.yMax--;
    Graphics_drawRectangle(&ui_gr_context_landscape, &rect);
    rect.xMin++; rect.yMin++; rect.xMax--; rect.yMax--;

    // Clear text area:
    Graphics_setBackgroundColor(&ui_gr_context_landscape, GRAPHICS_COLOR_BLACK);
    Graphics_setForegroundColor(&ui_gr_context_landscape, GRAPHICS_COLOR_WHITE);
    fillRectangle(&ui_gr_context_landscape, &rect);
    Graphics_setBackgroundColor(&ui_gr_context_landscape, GRAPHICS_COLOR_WHITE);
    Graphics_setForegroundColor(&ui_gr_context_landscape, GRAPHICS_COLOR_BLACK);

    // Draw the text itself:
    Graphics_setFont(&ui_gr_context_landscape, &g_sFontfixed10x20);
    Graphics_drawString(&ui_gr_context_landscape, (int8_t *)textentry_buf, textentry_len, txt_left, txt_top, 1);

    // Now invert and draw the current character:
    Graphics_setBackgroundColor(&ui_gr_context_landscape, GRAPHICS_COLOR_BLACK);
    Graphics_setForegroundColor(&ui_gr_context_landscape, GRAPHICS_COLOR_WHITE);

    Graphics_drawString(&ui_gr_context_landscape, textentry_buf[textentry_cursor] ? (int8_t *) &textentry_buf[textentry_cursor] : " ", 1, txt_left +
            Graphics_getStringWidth(&ui_gr_context_landscape, (const int8_t *) textentry_buf, textentry_cursor), txt_top, 1);

    Graphics_setBackgroundColor(&ui_gr_context_landscape, GRAPHICS_COLOR_WHITE);
    Graphics_setForegroundColor(&ui_gr_context_landscape, GRAPHICS_COLOR_BLACK);
    Graphics_flushBuffer(&ui_gr_context_landscape);
}

void ui_textentry_do(UInt events) {
    if (pop_events(&events, UI_EVENT_REFRESH)) {
        ui_textentry_draw();
        epd_do_partial = 1; // TODO: this?
    }

    if (pop_events(&events, UI_EVENT_KB_PRESS)) {
        switch(kb_active_key_masked) {
        case KB_RIGHT:
            // right cursor
            if (textentry_cursor < strlen(textentry_buf)) {
                textentry_cursor++;
                Event_post(ui_event_h, UI_EVENT_REFRESH);
            }
            break;
        case KB_LEFT:
            // right cursor
            if (textentry_cursor) {
                textentry_cursor--;
                Event_post(ui_event_h, UI_EVENT_REFRESH);
            }
            break;
        case KB_DOWN:
            if (textentry_buf[textentry_cursor] < '0') {
                // Start with A
                textentry_buf[textentry_cursor] = 'A';
            } else if (textentry_buf[textentry_cursor] < '9') {
                // It's a number, so next number:
                textentry_buf[textentry_cursor]++;
            } else if (textentry_buf[textentry_cursor] == '9') {
                // It's number 9, now use the null term:
                textentry_buf[textentry_cursor] = 0x00;
            } else if (textentry_buf[textentry_cursor] < 'Z') {
                // Capitals:
                textentry_buf[textentry_cursor]++;
            } else if (textentry_buf[textentry_cursor] == 'Z') {
                // It's a Z, so go to a:
                textentry_buf[textentry_cursor] = 'a';
            } else if (textentry_buf[textentry_cursor] == 'z') {
                textentry_buf[textentry_cursor] = '0';
            } else {
                textentry_buf[textentry_cursor] = 0x00;
            }
            Event_post(ui_event_h, UI_EVENT_REFRESH);
            break;
        case KB_UP:
            if (textentry_buf[textentry_cursor] < '0') {
                textentry_buf[textentry_cursor] = '9';
            } else if (textentry_buf[textentry_cursor] > 'a') {
                textentry_buf[textentry_cursor]--;
            } else if (textentry_buf[textentry_cursor] == 'a') {
                textentry_buf[textentry_cursor] = 'Z';
            } else if (textentry_buf[textentry_cursor] > 'A') {
                textentry_buf[textentry_cursor]--;
            } else if (textentry_buf[textentry_cursor] == 'A') {
                textentry_buf[textentry_cursor] = 0x00;
            } else if (textentry_buf[textentry_cursor] > '0') {
                textentry_buf[textentry_cursor]--;
            } else if (textentry_buf[textentry_cursor] == '0') {
                textentry_buf[textentry_cursor] = 'z';
            } else {
                textentry_buf[textentry_cursor] = 0x00;
            }
            Event_post(ui_event_h, UI_EVENT_REFRESH);
            break;
        case KB_BACK:
            ui_textentry_unload(0);
            break;
        case KB_OK:
            ui_textentry_unload(1);
            break;
        }
        // TODO:
        // Call this if needed:
        //led_tail_start_anim();
    }
}
