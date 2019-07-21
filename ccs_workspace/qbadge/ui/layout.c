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

void ui_draw_element(element_type element, uint8_t bar_level, uint8_t bar_capacity, uint32_t number, uint16_t x, uint16_t y) {
    // NB: We don't do much bounds checking, because none of the parameters
    //     meaningfully address anything.

    Graphics_Rectangle rect;

    if (element >= ELEMENT_COUNT_NONE) {
        // invalid call, trap (because something terrible may have happened)
        while (1);
    }

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

    char amt[4];
    if (number < 1000) {
        sprintf(amt, "%d", number);
    } else if (number < 100000) {
        // <100k
        sprintf(amt, "%dk", number/1000);
    } else {
        sprintf(amt, "99k");
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

void ui_draw_top_bar_remote_element_icons() {

}

void ui_draw_battery_at(Graphics_Context *gr, uint16_t x, uint16_t y) {
    Graphics_Rectangle rect;

    // TODO: Hey, just make this work::::::::::::::::::::::::::::::::

    for (uint8_t battery=0; battery<2; battery++) {
        // Draw the battery body:
        rect.xMin = x;
        rect.xMax = x+BATTERY_BODY_WIDTH;
        rect.yMin = y + (battery*(BATTERY_BODY_HEIGHT+BATTERY_BODY_VPAD));
        rect.yMax = rect.yMin+BATTERY_BODY_HEIGHT;
        Graphics_drawRectangle(gr, &rect);

        if (vbat_out_uvolts/1000000 > 2 || (vbat_out_uvolts/1000000 == 2 && (vbat_out_uvolts/100000 % 10 >= VBAT_LOW_2DOT))) {
            // Low battery segment
            rect.xMin +=2;
            rect.yMin +=2;
            rect.yMax -=2;
            rect.xMax = rect.xMin + BATTERY_SEGMENT_WIDTH - BATTERY_SEGMENT_PAD;
            fillRectangle(gr, &rect);
        }

        if (vbat_out_uvolts/1000000 > 2 || (vbat_out_uvolts/1000000 == 2 && (vbat_out_uvolts/100000 % 10 >= VBAT_MID_2DOT))) {
            // Mid battery segment
            rect.xMin = rect.xMax + BATTERY_SEGMENT_PAD;
            rect.xMax = rect.xMin + BATTERY_SEGMENT_WIDTH - BATTERY_SEGMENT_PAD;
            fillRectangle(gr, &rect);
        }

        if (vbat_out_uvolts/1000000 > 2 || (vbat_out_uvolts/1000000 == 2 && (vbat_out_uvolts/100000) % 10 >= VBAT_FULL_2DOT)) {
            // Full battery segment
            rect.xMin = rect.xMax + BATTERY_SEGMENT_PAD;
            rect.xMax = rect.xMin + BATTERY_SEGMENT_WIDTH - BATTERY_SEGMENT_PAD;
            fillRectangle(gr, &rect);
        }

        // Draw the anode:
        rect.xMin = x+BATTERY_BODY_WIDTH;
        rect.xMax = x+BATTERY_BODY_WIDTH+BATTERY_ANODE_WIDTH;
        rect.yMin = y + (battery*(BATTERY_BODY_HEIGHT+BATTERY_BODY_VPAD)) + (BATTERY_BODY_HEIGHT-BATTERY_ANODE_HEIGHT)/2;
        rect.yMax = rect.yMin + BATTERY_ANODE_HEIGHT;
        Graphics_drawRectangle(gr, &rect);
    }

    if (vbat_out_uvolts/1000000 < 2 || (vbat_out_uvolts/1000000 == 2 && (vbat_out_uvolts/100000) % 10 < VBAT_LOW_2DOT)) {
        // Very low battery warning
        Graphics_drawStringCentered(gr, "LOW!", 4, x + TOPBAR_ICON_WIDTH/2, y + TOPBAR_ICON_HEIGHT + TOPBAR_TEXT_HEIGHT/2 - 3, 0);
    } else {
        // Otherwise, voltage reading
        Graphics_setFont(gr, &g_sFontFixed6x8);
        char bat_text[5] = "3.0V";
        sprintf(bat_text, "%d.%dV", vbat_out_uvolts/1000000, (vbat_out_uvolts/100000) % 10);
        Graphics_drawStringCentered(gr, (int8_t *) bat_text, 4, x + TOPBAR_ICON_WIDTH/2, y + TOPBAR_ICON_HEIGHT + TOPBAR_TEXT_HEIGHT/2 - 3, 0);
    }
}

/// Draw the agent-present, handler-available, and radar icons.
void ui_draw_hud(Graphics_Context *gr, uint8_t agent_vertical, uint16_t x, uint16_t y) {
    char str[QC16_BADGE_NAME_LEN+1] = {0,};

    x += 1;

    if (badge_conf.agent_present) {
        // Should draw the agent icon.
        qc16gr_drawImage(gr, &img_hud_agent, x, y+4);
    }
    x += img_hud_agent.xSize + 1;


    // Give more space for the text.
    x += 5;

    if (agent_vertical) {
        if (mission_getting_possible()) {
            qc16gr_drawImage(gr, &img_hud_handler, x, y+1);
        }
        x += img_hud_handler.xSize + 1;
    } else {
        if (mission_getting_possible()) {
            qc16gr_drawImage(gr, &img_hud_handler_sideways, x, y+1);
            if (handler_nearby()) {
                // Use the handler's name
            } else {
                // vhandler
                sprintf(str, "vhandler");
            }
            Graphics_setFont(gr, &g_sFontFixed6x8);
            Graphics_drawStringCentered(
                    gr,
                    (int8_t *) str,
                    QC16_BADGE_NAME_LEN,
                    x+img_hud_handler_sideways.xSize/2,
                    y+TOPBAR_ICON_HEIGHT + TOPBAR_TEXT_HEIGHT/2 - 1,
                    0
            );
        }
        x += img_hud_handler_sideways.xSize + 1;
    }

    // Give more space for the text.
    x += 3;

    // Now, regardless, we draw the radar icon, and text.
    //  We've got about 15 px for it.

    qc16gr_drawImage(gr, &img_hud_radar, x, y);

    sprintf(str, "%d", qbadges_near_count);
    Graphics_setFont(gr, &g_sFontFixed6x8);
    Graphics_drawStringCentered(
            gr,
        (int8_t *) str,
        4,
        x+img_hud_radar.xSize/2,
        y+TOPBAR_ICON_HEIGHT + TOPBAR_TEXT_HEIGHT/2 - 1,
        0
    );

    x += img_hud_radar.xSize + 4;

    // TODO: numbers:
    ui_draw_battery_at(gr, x, y+2);
}

void ui_draw_top_bar() {
    // Draw the top bar.
    Graphics_drawLine(&ui_gr_context_landscape, 0, TOPBAR_HEIGHT, 295, TOPBAR_HEIGHT);
    Graphics_drawLine(&ui_gr_context_landscape, 0, TOPBAR_HEIGHT+1, 295, TOPBAR_HEIGHT+1);
    Graphics_Rectangle rect;
    rect.xMin = 0;
    rect.xMax = 295;
    rect.yMin = TOPBAR_HEIGHT;
    rect.yMax = TOPBAR_HEIGHT+1;

    fadeRectangle(&ui_gr_context_landscape, &rect);

    ui_draw_top_bar_local_element_icons();
    ui_draw_hud(&ui_gr_context_landscape, 0, TOPBAR_SEG_WIDTH_PADDED * 3, 0);
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
