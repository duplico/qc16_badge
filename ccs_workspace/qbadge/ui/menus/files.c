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
#include <ui/menus/menus.h>

#include <badge.h>
#include <board.h>
#include <ui/layout.h>

char curr_file_name[SPIFFS_OBJ_NAME_LEN+1] = {0,};

#define FILE_LOAD 0
#define FILE_DELETE 1
#define FILE_RENAME 2

void put_filename(Graphics_Context *gr, int8_t *name, uint16_t y) {
    Graphics_drawString(gr, name, SPIFFS_OBJ_NAME_LEN, FILES_LEFT_X, y, 1);
}

void put_filecursor(Graphics_Context *gr, int8_t *text, uint16_t y) {
    Graphics_drawString(gr, text, 7, FILES_LEFT_X-44, y, 1);
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

    uint8_t y=TOPBAR_HEIGHT + 6;

    Graphics_setFont(&ui_gr_context_landscape, &g_sFontFixed6x8);

    for (uint8_t i=0; i<2; i++) {
        file_num++;
        if (file_num < first_file || file_num > last_file) {
        } else {
            put_filename(&ui_gr_context_landscape, (int8_t *) dirs[i], y);
            y+=9;
            if (first_file + ui_y_cursor == file_num) {
                // This is the selected file.
                strncpy(curr_fname, dirs[i], SPIFFS_OBJ_NAME_LEN);
                put_filename(&ui_gr_context_landscape, "SEL", y);
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
            if (file_num < first_file || file_num > last_file) {
                continue;
            }
            if (ui_y_cursor == file_num) {
                // This is the selected file.
                strncpy(curr_fname, (char *) pe->name, SPIFFS_OBJ_NAME_LEN);
                put_filename(&ui_gr_context_landscape, "SEL", y);
            }
            put_filename(&ui_gr_context_landscape, (int8_t *) pe->name, y);
            y+=9;
        }
        SPIFFS_closedir(&d);
    }

    ui_y_max = file_num;

    y = TOPBAR_HEIGHT + 6 + 9*(ui_y_cursor - first_file);

    if (!strncmp("/colors/", curr_fname, SPIFFS_OBJ_NAME_LEN)) {
        // If this is a "directory heading":
        put_filecursor(&ui_gr_context_landscape, " store:", y);
    } else if (!strncmp("/photos/", curr_fname, SPIFFS_OBJ_NAME_LEN)) {
        // If this is a "directory heading":
        put_filecursor(&ui_gr_context_landscape, "   dir:", y);
    } else if (!strncmp("/photos/", curr_file_name, 8)
            && !strncmp(badge_conf.current_photo, &curr_file_name[8], QC16_PHOTO_NAME_LEN)) {
        // This is the CURRENT photo
        // We can't do anything with it.
        put_filecursor(&ui_gr_context_landscape, "in use:", y);
    } else {
        // This is any other file:

        switch(ui_x_cursor) {
        case FILE_LOAD:
            put_filecursor(&ui_gr_context_landscape, "  load:", y);
            break;
        case FILE_DELETE:
            put_filecursor(&ui_gr_context_landscape, "delete:", y);
            break;
        case FILE_RENAME:
            put_filecursor(&ui_gr_context_landscape, "rename:", y);
            break;
        }
    }
}

void ui_draw_files() {
    // Clear the buffer.
    Graphics_clearDisplay(&ui_gr_context_landscape);

    ui_draw_top_bar();
    put_all_filenames(curr_file_name);

    Graphics_flushBuffer(&ui_gr_context_landscape);
}

void ui_files_do(UInt events) {
    static uint8_t text_use = 0;
    static char *text;

    if (pop_events(&events, UI_EVENT_KB_PRESS)) {
        switch(kb_active_key_masked) {
        case KB_OK:
            if (!strncmp("/colors/", curr_file_name, SPIFFS_OBJ_NAME_LEN)) {
                // Color SAVE request
                text = malloc(QC16_PHOTO_NAME_LEN+1);
                memset(text, 0x00, QC16_PHOTO_NAME_LEN+1);
                text_use = FILES_TEXT_USE_SAVE_COLOR;
                ui_textentry_load(text, 12);
            } else if (!strncmp("/photos/", curr_file_name, SPIFFS_OBJ_NAME_LEN)) {
                // This is an "info" request for the photos.
                // TODO: Change, if we don't get the infobox working.
            } else {
                // This is a file. (colors or photos)

                if (!strncmp("/photos/", curr_file_name, 8)
                        && !strncmp(badge_conf.current_photo, &curr_file_name[8], QC16_PHOTO_NAME_LEN)) {
                    // This is the CURRENT photo
                    // We can't do anything with it.
                    break;
                }

                switch(ui_x_cursor) {
                case FILE_DELETE:
                    SPIFFS_remove(&fs, curr_file_name);
                    ui_y_cursor--;
                    Event_post(ui_event_h, UI_EVENT_REFRESH);
                    break;
                case FILE_RENAME:
                    // Allocate space for the entire path, but only expose
                    //  the file name part to textentry.
                    // NB: SPIFFS_OBJ_NAME_LEN includes the null term.
                    text = malloc(SPIFFS_OBJ_NAME_LEN);
                    strncpy(text, curr_file_name, SPIFFS_OBJ_NAME_LEN);
                    text_use = FILES_TEXT_USE_RENAME;
                    ui_textentry_load(&text[8], QC16_PHOTO_NAME_LEN);
                    Event_post(ui_event_h, UI_EVENT_REFRESH);
                    break;
                case FILE_LOAD:
                    // Load this animation:
                    if (!strncmp("/colors/", curr_file_name, 8)) {
                        load_anim_abs(curr_file_name);
                    } else {
                        // photo
                        strncpy(badge_conf.current_photo, &curr_file_name[8], QC16_PHOTO_NAME_LEN);
                    }
                    Event_post(ui_event_h, UI_EVENT_REFRESH);
                }
            }
            break;
        }
    }

    if (pop_events(&events, UI_EVENT_TEXT_CANCELED)) {
        // dooo noooooothiiiiing
        // except prevent a memory leak:
        free(text);
    }

    if (pop_events(&events, UI_EVENT_TEXT_READY)) {
        switch(text_use) {
        case FILES_TEXT_USE_RENAME:
        {
            // Rename curr_file_name to text
            SPIFFS_rename(&fs, curr_file_name, text);
            Event_post(ui_event_h, UI_EVENT_REFRESH);
            break;
        }
        case FILES_TEXT_USE_SAVE_COLOR:
            // Save current colors as text
            save_anim(text);
            break;
        }

        free(text);
    }

    if (pop_events(&events, UI_EVENT_REFRESH)) {
        ui_draw_files();
    }
}
