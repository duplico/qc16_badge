/*
 * layout.c
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

#include "ui.h"
#include "graphics.h"
#include "images.h"
#include "keypad.h"

#include <badge.h>
#include <board.h>
#include <ui/layout.h>

void ui_draw_element(element_type element, uint8_t bar_level, uint8_t bar_capacity, uint16_t number, uint16_t x, uint16_t y) {
    // TODO: consider making this a global or heap var that we share everywhere.
    Graphics_Rectangle rect;
    // TODO: assert on element
    // TODO: bounds checking

    uint16_t text_x = x+TOPBAR_ICON_WIDTH;
    uint16_t text_y = y+4;
    uint16_t bar_x0 = x;
    uint16_t bar_y0 = y+TOPBAR_ICON_HEIGHT+1;
    uint16_t bar_y1 = bar_y0+TOPBAR_PROGBAR_HEIGHT-1;
    tImage *icon_img;

    icon_img = (Graphics_Image *) image_element_icons[(uint8_t) element];

    qc16gr_drawImage(&ui_gr_context_landscape, icon_img, x, y);

    // The "fullness" bar, if applicable:
    for (uint8_t i=0; i<5; i++) {
        rect.xMin = bar_x0+2 + 7*i;
        rect.xMax = rect.xMin + 5;
        rect.yMin = bar_y0;
        rect.yMax = bar_y1;
        if (i < bar_level) {
            fillRectangle(&ui_gr_context_landscape, &rect);
        } else if (i < bar_capacity) {
            Graphics_drawRectangle(&ui_gr_context_landscape, &rect);
        } else {
            Graphics_drawRectangle(&ui_gr_context_landscape, &rect);
            fadeRectangle(&ui_gr_context_landscape, &rect);
        }
    }
    Graphics_setFont(&ui_gr_context_landscape, &UI_TEXT_FONT);

    // TODO: handle >999
    //       like: 1k, 2k, 3k...
    char amt[4];
    if (number < 999) {
        sprintf(amt, "%d", number);
    } else if (number < 100000) {
        // <100k
        sprintf(amt, "%dk", number/1000);
    } else {
        // TODO: lots!
    }
    amt[3] = 0x00;
    Graphics_drawString(&ui_gr_context_landscape, (int8_t *)amt, 3, text_x, text_y, 0);
}

void ui_draw_top_bar_local_element_icons() {
    for (uint8_t i=0; i<3; i++) {
        uint16_t icon_x = i*TOPBAR_SEG_WIDTH_PADDED;
        uint16_t icon_y = 0;
        ui_draw_element((element_type) i, badge_conf.element_level[i], badge_conf.element_level_max[i], badge_conf.element_qty[i], icon_x, icon_y);
    }
}

void ui_draw_top_bar_qbadge_headsup_icons() {
    // TODO: consider making this a global or heap var that we share everywhere.
    Graphics_Rectangle rect;

    char cnt[5]; // TODO

    // TODO: Add a pad here, explicitly:

    for (uint8_t i=0; i<3; i++) {
        uint16_t icon_x = TOPBAR_HEADSUP_START + i*TOPBAR_SEG_WIDTH_PADDED;
        uint16_t icon_y = 0;
        uint8_t fade = 0;

        const tImage *icon_img;


        switch(i) {
        case 0: // agent
            icon_img = &img_agent;
            if (!badge_conf.agent_present) {
                fade = 1;
                // TODO: progress bar for returning...?
            }
            break;
        case 1: // handlers
            icon_img = &img_handler;
            // TODO: count:
            // TODO: are any here?
            fade = 1;
            break;
        case 2: // scan
            icon_img = &img_radar;
            sprintf(cnt, "%d", qbadges_near_count);
            Graphics_setFont(&ui_gr_context_landscape, &g_sFontFixed6x8);
            Graphics_drawStringCentered(
                    &ui_gr_context_landscape,
                    (int8_t *) cnt,
                    4,
                    icon_x + TOPBAR_SEG_WIDTH/2,
                    TOPBAR_ICON_HEIGHT + TOPBAR_TEXT_HEIGHT/2 - 1,
                    0);
            // TODO: count:

            break;
        }

        // Align the icon horizontally:
        icon_x += (TOPBAR_SEG_WIDTH - icon_img->ySize) / 2;
        qc16gr_drawImage(&ui_gr_context_landscape, icon_img, icon_x, icon_y);
        if (fade) {
            rect.xMin = icon_x;
            rect.yMin = icon_y;
            rect.xMax = icon_x + icon_img->xSize;
            rect.yMax = icon_y + icon_img->ySize;
            fadeRectangle(&ui_gr_context_landscape, &rect);
        }
    }

    // Is our agent around? Draw the agent symbol.

    // Are there any handlers around? Draw the handler symbol.

}

void ui_draw_top_bar_battery_life() {
    Graphics_Rectangle rect;
    // Draw the top battery:
    rect.xMin = BATTERY_X;
    rect.xMax = BATTERY_X+BATTERY_BODY_WIDTH;
    rect.yMin = BATTERY_BODY0_Y0;
    rect.yMax = BATTERY_BODY0_Y1;
    Graphics_drawRectangle(&ui_gr_context_landscape, &rect);

    rect.xMin = BATTERY_X+BATTERY_BODY_WIDTH;
    rect.xMax = BATTERY_X+BATTERY_BODY_WIDTH+BATTERY_ANODE_WIDTH;
    rect.yMin = BATTERY_ANODE0_Y0;
    rect.yMax = BATTERY_ANODE0_Y0 + BATTERY_ANODE_HEIGHT;
    Graphics_drawRectangle(&ui_gr_context_landscape, &rect);

    // Bottom battery:
    rect.xMin = BATTERY_X;
    rect.xMax = BATTERY_X+BATTERY_BODY_WIDTH;
    rect.yMin = BATTERY_BODY1_Y0;
    rect.yMax = BATTERY_BODY1_Y1;
    Graphics_drawRectangle(&ui_gr_context_landscape, &rect);

    rect.xMin = BATTERY_X+BATTERY_BODY_WIDTH;
    rect.xMax = BATTERY_X+BATTERY_BODY_WIDTH+BATTERY_ANODE_WIDTH;
    rect.yMin = BATTERY_ANODE1_Y0;
    rect.yMax = BATTERY_ANODE1_Y0 + BATTERY_ANODE_HEIGHT;
    Graphics_drawRectangle(&ui_gr_context_landscape, &rect);

    // TODO: change this font:
    Graphics_setFont(&ui_gr_context_landscape, &g_sFontFixed6x8);
    char bat_text[5] = "3.0V";
    sprintf(bat_text, "%d.%dV", vbat_out_uvolts/1000000, (vbat_out_uvolts/100000) % 10);
    Graphics_drawStringCentered(&ui_gr_context_landscape, (int8_t *) bat_text, 4, BATTERY_X + TOPBAR_ICON_WIDTH/2, TOPBAR_ICON_HEIGHT + TOPBAR_TEXT_HEIGHT/2 - 1, 0);

    // Now do the graphics part:
    if (vbat_out_uvolts/1000000 > 2 || (vbat_out_uvolts/1000000 == 2 && (vbat_out_uvolts/100000) % 10 >= VBAT_FULL_2DOT)) {
        // full
        rect.xMin = BATTERY_HIGH_X0+BATTERY_SEGMENT_PAD;
        rect.xMax = BATTERY_HIGH_X0+BATTERY_SEGMENT_WIDTH-1;
        rect.yMin = BATTERY_BODY0_Y0+BATTERY_SEGMENT_PAD+1;
        rect.yMax = BATTERY_BODY0_Y1-BATTERY_SEGMENT_PAD-1;
        fillRectangle(&ui_gr_context_landscape, &rect);

        rect.xMin = BATTERY_HIGH_X0+BATTERY_SEGMENT_PAD;
        rect.xMax = BATTERY_HIGH_X0+BATTERY_SEGMENT_WIDTH-1;
        rect.yMin = BATTERY_BODY1_Y0+BATTERY_SEGMENT_PAD+1;
        rect.yMax = BATTERY_BODY1_Y1-BATTERY_SEGMENT_PAD-1;
        fillRectangle(&ui_gr_context_landscape, &rect);
    }
    if (vbat_out_uvolts/1000000 > 2 || (vbat_out_uvolts/1000000 == 2 && (vbat_out_uvolts/100000 % 10 >= VBAT_MID_2DOT))) {
        rect.xMin = BATTERY_MID_X0+BATTERY_SEGMENT_PAD;
        rect.xMax = BATTERY_MID_X0+BATTERY_SEGMENT_WIDTH;
        rect.yMin = BATTERY_BODY0_Y0+BATTERY_SEGMENT_PAD+1;
        rect.yMax = BATTERY_BODY0_Y1-BATTERY_SEGMENT_PAD-1;
        fillRectangle(&ui_gr_context_landscape, &rect);

        rect.xMin = BATTERY_MID_X0+BATTERY_SEGMENT_PAD;
        rect.xMax = BATTERY_MID_X0+BATTERY_SEGMENT_WIDTH;
        rect.yMin = BATTERY_BODY1_Y0+BATTERY_SEGMENT_PAD+1;
        rect.yMax = BATTERY_BODY1_Y1-BATTERY_SEGMENT_PAD-1;
        fillRectangle(&ui_gr_context_landscape, &rect);
    }
    if (vbat_out_uvolts/1000000 > 2 || (vbat_out_uvolts/1000000 == 2 && (vbat_out_uvolts/100000 % 10 >= VBAT_LOW_2DOT))) {
        rect.xMin = BATTERY_LOW_X0+BATTERY_SEGMENT_PAD+1;
        rect.xMax = BATTERY_LOW_X0+BATTERY_SEGMENT_WIDTH;
        rect.yMin = BATTERY_BODY0_Y0+BATTERY_SEGMENT_PAD+1;
        rect.yMax = BATTERY_BODY0_Y1-BATTERY_SEGMENT_PAD-1;
        fillRectangle(&ui_gr_context_landscape, &rect);

        rect.xMin = BATTERY_LOW_X0+BATTERY_SEGMENT_PAD+1;
        rect.xMax = BATTERY_LOW_X0+BATTERY_SEGMENT_WIDTH;
        rect.yMin = BATTERY_BODY1_Y0+BATTERY_SEGMENT_PAD+1;
        rect.yMax = BATTERY_BODY1_Y1-BATTERY_SEGMENT_PAD-1;
        fillRectangle(&ui_gr_context_landscape, &rect);
    }
    if (vbat_out_uvolts/1000000 < 2 || (vbat_out_uvolts/1000000 == 2 && (vbat_out_uvolts/100000) % 10 < VBAT_LOW_2DOT)) {
        // ABOUT TO RUN OUT!!!
        // TODO
//        Graphics_drawString(&ui_gr_context_landscape, ":-(", 3, BATTERY_X+2, BATTERY_BODY_Y0+4, 0);

    }
}

void ui_draw_top_bar() {
    // Draw the top bar.
    // TODO: do we prefer this?:
    Graphics_drawLine(&ui_gr_context_landscape, 0, TOPBAR_HEIGHT, 295, TOPBAR_HEIGHT);
    Graphics_drawLine(&ui_gr_context_landscape, 0, TOPBAR_HEIGHT+1, 295, TOPBAR_HEIGHT+1);
    Graphics_Rectangle rect;
    rect.xMin = 0;
    rect.xMax = 295;
    rect.yMin = TOPBAR_HEIGHT;
    rect.yMax = TOPBAR_HEIGHT+1;

    fadeRectangle(&ui_gr_context_landscape, &rect);

    ui_draw_top_bar_battery_life();
    ui_draw_top_bar_local_element_icons();
    ui_draw_top_bar_qbadge_headsup_icons();
}

void ui_draw_menu_icons(uint8_t selected_index, const Graphics_Image **icons, const char text[][MAINMENU_NAME_MAX_LEN+1], uint16_t pad, uint16_t x, uint16_t y, uint8_t len) {
    Graphics_Rectangle rect;
    rect.yMin = y;
    rect.yMax = rect.yMin + icons[0]->ySize - 1;
    for (uint8_t i=0; i<len; i++) {
        // NB: This depends on all icons being same width:
        rect.xMin = x + i*(icons[i]->xSize + pad);
        rect.xMax = rect.xMin + icons[i]->xSize - 1;

        qc16gr_drawImage(&ui_gr_context_landscape, icons[i], rect.xMin, rect.yMin);

        if (selected_index == i) {
            // make it selected
            Graphics_setFont(&ui_gr_context_landscape, &UI_TEXT_FONT);
            int32_t text_w = Graphics_getStringWidth(&ui_gr_context_landscape, (int8_t *) text[i], MAINMENU_NAME_MAX_LEN);
            int32_t text_x = rect.xMin + icons[i]->xSize/2 - text_w/2;
            if (text_x < 0) {
                text_x = 0;
            } else if (i == len-1 && text_x + text_w > rect.xMax) {
                // need to make it so that text_x + text_w == rect.xMax + 2*pad
                text_x = rect.xMax + 2*pad - text_w;
            }
            Graphics_drawString(
                    &ui_gr_context_landscape,
                    (int8_t *) text[i],
                    MAINMENU_NAME_MAX_LEN,
                    text_x,
                    rect.yMax + 1,
                    0
                );
        } else {
            fadeRectangle(&ui_gr_context_landscape, &rect);
        }
    }
}
