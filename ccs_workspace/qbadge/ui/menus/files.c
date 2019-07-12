/*
 * files.c
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

#include "ui/ui.h"
#include "ui/graphics.h"
#include "ui/images.h"
#include "ui/keypad.h"
#include <ui/overlays/overlays.h>

#include <badge.h>
#include <board.h>
#include <ui/layout.h>

char curr_file_name[SPIFFS_OBJ_NAME_LEN+1] = "";

void put_filename(Graphics_Context *gr, int8_t *name, uint16_t y) {
    Graphics_drawString(gr, name, SPIFFS_OBJ_NAME_LEN, FILES_LEFT_X, y, 1);
}

void put_all_filenames(char *curr_fname) {
    int32_t first_file = 0;
    int32_t last_file = 0;
    int32_t file_num = -1;

    first_file = ui_y_cursor - 5;
    if (first_file < 0) {
        first_file = 0;
    }
    last_file = first_file + 9;

    char dirs[][9] = {
                      "/colors/",
                      "/photos/",
    };

    uint8_t y=TOPBAR_HEIGHT + 3;

    Graphics_setFont(&ui_gr_context_landscape, &g_sFontFixed6x8);

    // TODO:
    for (uint8_t i=0; i<2; i++) {
        file_num++;
        // TODO: rewrite:
        if (file_num < first_file) {
        } else if (file_num > last_file) {
            break;
        } else {
            put_filename(&ui_gr_context_landscape, (int8_t *) dirs[i], y);
            y+=9;
            if (first_file + ui_y_cursor == file_num) {
                // This is the selected file.
                strncpy(curr_fname, dirs[i], SPIFFS_OBJ_NAME_LEN);
            }
        }

        spiffs_DIR d;
        struct spiffs_dirent e;
        struct spiffs_dirent *pe = &e;
        SPIFFS_opendir(&fs, "/", &d);
        while ((pe = SPIFFS_readdir(&d, pe)) &&
                (y+7 < EPD_WIDTH)) {
            if (strncmp(dirs[i], (char *) pe->name, 8)) {
                continue;
            }
            if (!strncmp("/colors/.current", (char *) pe->name, 16)) {
                continue;
            }
            file_num++;
            if (file_num < first_file) {
                continue;
            } else if (file_num > last_file) {
                break;
            } else if (first_file + ui_y_cursor == file_num) {
                // This is the selected file.
                strncpy(curr_fname, (char *) pe->name, SPIFFS_OBJ_NAME_LEN);
            }
            put_filename(&ui_gr_context_landscape, (int8_t *) pe->name, y);
            y+=9;
        }
        SPIFFS_closedir(&d);
    }

    y = TOPBAR_HEIGHT + 3 + 9*(ui_y_cursor - first_file);

    if (!strncmp("/colors/", curr_fname, SPIFFS_OBJ_NAME_LEN) || !strncmp("/photos/", curr_fname, SPIFFS_OBJ_NAME_LEN)) {
        // If this is a "directory heading":
        Graphics_drawString(&ui_gr_context_landscape, "store:", 6, 0, y, 1);
    } else {
        switch(ui_x_cursor) {
        case 0:
            Graphics_drawString(&ui_gr_context_landscape, " load:", 6, 0, y, 1);
            break;
        case 1:
            Graphics_drawString(&ui_gr_context_landscape, "delet:", 6, 0, y, 1);
            break;
        case 2:
            Graphics_drawString(&ui_gr_context_landscape, "renam:", 6, 0, y, 1);
            break;
        }
    }

}

void ui_draw_files() {
    // Clear the buffer.
    Graphics_clearDisplay(&ui_gr_context_landscape);

    ui_draw_top_bar();
    // TODO: draw the files we actually want...
    // TODO: allow to scroll
    put_all_filenames(curr_file_name);

    Graphics_flushBuffer(&ui_gr_context_landscape);
}

void ui_files_do(UInt events) {
    switch(events) {
    case UI_EVENT_REFRESH:
        ui_draw_files();
        break;
    case UI_EVENT_KB_PRESS:
        switch(kb_active_key_masked) {
        case BTN_OK:
            if (!strncmp("/colors/", curr_file_name, SPIFFS_OBJ_NAME_LEN)) {
                // Color SAVE request
                ui_textentry_load(curr_file_name, 12);
            } else if (!strncmp("/photos/", curr_file_name, SPIFFS_OBJ_NAME_LEN)) {
                // Photo SAVE request (wat)
                // TODO: nothing doing...
            } else if (!strncmp("/colors/", curr_file_name, 8)) {

            } else if (!strncmp("/photos/", curr_file_name, 8)) {
            }
            break;
        }
        break;
    }
}
