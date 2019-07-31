/*
 * mainmenu.c
 *
 *  Created on: Jun 26, 2019
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
#include <qc16_serial_common.h>

#include <ui/pair/pair.h>
#include "ui/ui.h"
#include "ui/graphics.h"
#include "ui/images.h"
#include "ui/keypad.h"

#include <badge.h>
#include <board.h>
#include <ui/layout.h>

void ui_draw_pair_files() {
    // Clear the buffer.
    Graphics_clearDisplay(&ui_gr_context_landscape);
    ui_draw_top_bar();
    Graphics_flushBuffer(&ui_gr_context_landscape);
}

void ui_pair_files_do(UInt events) {
    if (pop_events(&events, UI_EVENT_BACK)) {
        // Do nothing; this is the bottom of the stack when paired.
        return;
    }

    if (pop_events(&events, UI_EVENT_REFRESH)) {
        ui_draw_pair_cb();
    }

    if (pop_events(&events, UI_EVENT_KB_PRESS)) {
        switch(kb_active_key_masked) {
        case KB_OK:
            switch(ui_y_cursor) {
            case 0:
                // TODO: do handle things
                break;
            }
            break;
        }
    }
}
