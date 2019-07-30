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

uint8_t textbox_len;
char *textbox_dest;
char *textbox_buf;
uint8_t textentry_cursor = 0;

extern const tFont g_sFontfixed10x20;

void ui_textentry_load(char *dest, uint8_t len) {
    if (len > TEXTENTRY_MAX_LEN) {
        len = TEXTENTRY_MAX_LEN;
    }
    textbox_len = len;
    textbox_buf = malloc(len);
    textbox_dest = dest;
    textentry_cursor = 0;
    memset(textbox_buf, 0x00, len);
    strncpy(textbox_buf, textbox_dest, textbox_len);
    ui_textentry = 1;

    // Fade out everything.
    fadeRectangle_xy(&ui_gr_context_landscape, 0, 0, EPD_HEIGHT-1, EPD_WIDTH-1);

    Event_post(ui_event_h, UI_EVENT_REFRESH);
}

void ui_textentry_unload(uint8_t save) {
    ui_textentry = 0;
    if (save && textbox_buf[0] && textbox_dest) {
        // TODO: Manage null terms.
        // If we're supposed to copy this back to the destination,
        //  and if text was actually entered, then save it.
        strncpy(textbox_dest, textbox_buf, textbox_len);
        Event_post(ui_event_h, UI_EVENT_TEXT_READY);
    } else {
        Event_post(ui_event_h, UI_EVENT_TEXT_CANCELED);
    }

    free(textbox_buf);
    Event_post(ui_event_h, UI_EVENT_REFRESH);
}

void ui_textentry_draw() {
    uint16_t entry_width = textbox_len * 10;
    uint16_t txt_left = 148 - (entry_width/2);
    uint16_t txt_top = 64 - 10;

    Graphics_Rectangle rect = {
        .xMin = txt_left-3, .yMin = txt_top-3,
        .xMax = txt_left + entry_width + 3, .yMax = txt_top + 22,
    };

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
    Graphics_setFont(&ui_gr_context_landscape, &UI_FIXED_FONT);
    Graphics_drawString(&ui_gr_context_landscape, (int8_t *)textbox_buf, textbox_len, txt_left, txt_top, 1);

    // Now invert and draw the current character:
    Graphics_setBackgroundColor(&ui_gr_context_landscape, GRAPHICS_COLOR_BLACK);
    Graphics_setForegroundColor(&ui_gr_context_landscape, GRAPHICS_COLOR_WHITE);

    Graphics_drawString(&ui_gr_context_landscape, textbox_buf[textentry_cursor] ? (int8_t *) &textbox_buf[textentry_cursor] : " ", 1, txt_left +
            Graphics_getStringWidth(&ui_gr_context_landscape, (const int8_t *) textbox_buf, textentry_cursor), txt_top, 1);

    Graphics_setBackgroundColor(&ui_gr_context_landscape, GRAPHICS_COLOR_WHITE);
    Graphics_setForegroundColor(&ui_gr_context_landscape, GRAPHICS_COLOR_BLACK);
    Graphics_flushBuffer(&ui_gr_context_landscape);
}

void ui_textentry_do(UInt events) {
    if (pop_events(&events, UI_EVENT_REFRESH)) {
        ui_textentry_draw();
        epd_do_partial = 1;
    }

    if (pop_events(&events, UI_EVENT_KB_PRESS)) {
        switch(kb_active_key_masked) {
        case KB_RIGHT:
            // right cursor
            if (textentry_cursor < strlen(textbox_buf)) {
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
            if (textbox_buf[textentry_cursor] < '0') {
                // Start with A
                textbox_buf[textentry_cursor] = 'A';
            } else if (textbox_buf[textentry_cursor] < '9') {
                // It's a number, so next number:
                textbox_buf[textentry_cursor]++;
            } else if (textbox_buf[textentry_cursor] == '9') {
                // It's number 9, now use the null term:
                textbox_buf[textentry_cursor] = 0x00;
            } else if (textbox_buf[textentry_cursor] < 'Z') {
                // Capitals:
                textbox_buf[textentry_cursor]++;
            } else if (textbox_buf[textentry_cursor] == 'Z') {
                // It's a Z, so go to a:
                textbox_buf[textentry_cursor] = 'a';
            } else if (textbox_buf[textentry_cursor] == 'z') {
                textbox_buf[textentry_cursor] = '0';
            } else {
                textbox_buf[textentry_cursor] = 0x00;
            }
            Event_post(ui_event_h, UI_EVENT_REFRESH);
            break;
        case KB_UP:
            if (textbox_buf[textentry_cursor] < '0') {
                textbox_buf[textentry_cursor] = '9';
            } else if (textbox_buf[textentry_cursor] > 'a') {
                textbox_buf[textentry_cursor]--;
            } else if (textbox_buf[textentry_cursor] == 'a') {
                textbox_buf[textentry_cursor] = 'Z';
            } else if (textbox_buf[textentry_cursor] > 'A') {
                textbox_buf[textentry_cursor]--;
            } else if (textbox_buf[textentry_cursor] == 'A') {
                textbox_buf[textentry_cursor] = 0x00;
            } else if (textbox_buf[textentry_cursor] > '0') {
                textbox_buf[textentry_cursor]--;
            } else if (textbox_buf[textentry_cursor] == '0') {
                textbox_buf[textentry_cursor] = 'z';
            } else {
                textbox_buf[textentry_cursor] = 0x00;
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
    }
}
