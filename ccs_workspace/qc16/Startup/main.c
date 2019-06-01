#include <string.h>

#include <xdc/runtime/Error.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/NVS.h>
#include <ti/drivers/I2C.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/grlib/grlib.h>

#include <board.h>

#include <hal_assert.h>
#include <bcomdef.h>

#include "uble_bcast_scan.h"

/* Header files required to enable instruction fetch cache */
#include <inc/hw_memmap.h>
#include <driverlib/vims.h>

#include <third_party/spiffs/SPIFFSNVS.h>
#include <third_party/spiffs/spiffs.h>

#include "queercon/epd_driver.h"
#include "queercon/ht16d35b.h"

// TODO:
#define SPIFFS_LOGICAL_BLOCK_SIZE    (4096)
#define SPIFFS_LOGICAL_PAGE_SIZE     (256)
#define SPIFFS_FILE_DESCRIPTOR_SIZE  (44)

uint8_t spiffsWorkBuffer[SPIFFS_LOGICAL_PAGE_SIZE * 2];
uint8_t spiffsFileDescriptorCache[SPIFFS_FILE_DESCRIPTOR_SIZE * 4];
uint8_t spiffsReadWriteCache[SPIFFS_LOGICAL_PAGE_SIZE * 2];
spiffs           fs;
spiffs_config    fsConfig;
SPIFFSNVS_Data spiffsnvs;


// TODO:
//
#define EPD_STACKSIZE 2048
Task_Struct epaperTask;
Char epaperTaskStack[EPD_STACKSIZE];

/* Buffer placed in RAM to hold bytes read from non-volatile storage. */
static char buffer[64];
static const char signature[] =
    {"SimpleLink SDK Non-Volatile Storage (NVS) SPI Example."};

void epaper_spi_task_fn(UArg a0, UArg a1)
{
//    init_epd(0);

    volatile int32_t status;
//
//    ht16d_init_io();
//    ht16d_init();

////////////////////////////////////
//
//    status = SPIFFSNVS_config(&spiffsnvs, QC16_NVSSPI25X0, &fs, &fsConfig,
//                              SPIFFS_LOGICAL_BLOCK_SIZE, SPIFFS_LOGICAL_PAGE_SIZE);
//    if (status != SPIFFSNVS_STATUS_SUCCESS) {
//        while (1); // Spin forever.
//    }
//    status = SPIFFS_mount(&fs, &fsConfig, spiffsWorkBuffer,
//        spiffsFileDescriptorCache, sizeof(spiffsFileDescriptorCache),
//        spiffsReadWriteCache, sizeof(spiffsReadWriteCache), NULL);
//
//    if (status == SPIFFS_ERR_NOT_A_FS) {
//        // Needs to be formatted before mounting.
//        status = SPIFFS_format(&fs);
//        status = SPIFFS_mount(&fs, &fsConfig, spiffsWorkBuffer,
//            spiffsFileDescriptorCache, sizeof(spiffsFileDescriptorCache),
//            spiffsReadWriteCache, sizeof(spiffsReadWriteCache), NULL);
//    }
//
//    status = SPIFFS_creat(&fs, "testfile", 0);
//    spiffs_file f;
//    f = SPIFFS_open(&fs, "testfile", SPIFFS_O_CREAT + SPIFFS_O_WRONLY, 0);
//    status = SPIFFS_write(&fs, f, "test", 4);
//    status = SPIFFS_close(&fs, f);
//
//    spiffs_stat stat;
//    status = SPIFFS_stat(&fs, "testfile", &stat);
//
//    char buf[10];
//    f = SPIFFS_open(&fs, "testfile", SPIFFS_O_RDONLY, 0);
//    status = SPIFFS_read(&fs, f, (void *)buf, 4);
////////////////////////////////

//    NVS_Handle nvsHandle;
//    volatile NVS_Attrs regionAttrs;
//    NVS_Params nvsParams;
//
//    NVS_init();
//    NVS_Params_init(&nvsParams);
//    nvsHandle = NVS_open(QC16_NVSSPI25X0, &nvsParams);
//
//    /*
//     * This will populate a NVS_Attrs structure with properties specific
//     * to a NVS_Handle such as region base address, region size,
//     * and sector size.
//     */
//    NVS_getAttrs(nvsHandle, &regionAttrs);
//
//    /* Display the NVS region attributes. */
////    Display_printf(displayHandle, 0, 0, "Sector Size: 0x%x",
////            regionAttrs.sectorSize);
////    Display_printf(displayHandle, 0, 0, "Region Size: 0x%x\n",
////            regionAttrs.regionSize);
//
//    /*
//     * Copy "sizeof(signature)" bytes from the NVS region base address into
//     * buffer.
//     */
//    status = NVS_read(nvsHandle, 0, (void *) buffer, sizeof(signature));
//
//    /*
//     * Determine if the NVS region contains the signature string.
//     * Compare the string with the contents copied into buffer.
//     */
//    if (strcmp((char *) buffer, (char *) signature) == 0) {
//
//        /* Write buffer copied from flash to the console. */
////        Display_printf(displayHandle, 0, 0, "%s", buffer);
////        Display_printf(displayHandle, 0, 0, "Erasing SPI flash sector...");
//
//        /* Erase the entire flash sector. */
////        status = NVS_erase(nvsHandle, 0, regionAttrs.sectorSize);
//    }
//    else {
//
//        /* The signature was not found in the NVS region. */
////        Display_printf(displayHandle, 0, 0, "Writing signature to SPI flash...");
//
//        /*
//         * Write signature to memory. The flash sector is erased prior
//         * to performing the write operation. This is specified by
//         * NVS_WRITE_ERASE.
//         */
//        status = NVS_write(nvsHandle, 0, (void *) signature, sizeof(signature),
//            NVS_WRITE_ERASE | NVS_WRITE_POST_VERIFY);
//    }

////////////////////////////////////


//    Graphics_Context gr_context;
//    Graphics_initContext(&gr_context, &epd_grGraphicsDisplay, &epd_grDisplayFunctions);
//    Graphics_setBackgroundColor(&gr_context, GRAPHICS_COLOR_WHITE);
//    Graphics_setForegroundColorTranslated(&gr_context, GRAPHICS_COLOR_BLACK);
//    Graphics_clearDisplay(&gr_context);
//    Graphics_fillCircle(&gr_context, 128, 64, 32);
//    Graphics_drawLine(&gr_context, 0, 16, 295, 16);
//    Graphics_setFont(&gr_context, &g_sFontCm14);
//    Graphics_drawString(&gr_context, "Queercon 2019", 13, 16, 16, 0);
////    Graphics_drawString(&gr_context, buf, status, 16, 32, 0);
//    epd_flip();
//    Graphics_flushBuffer(&gr_context);

//    ht16d_all_one_color(10,10,10);

#define BTN_ROW_1 0x10
#define BTN_ROW_2 0x20
#define BTN_ROW_3 0x30
#define BTN_ROW_4 0x40
#define BTN_COL_1 0x01
#define BTN_COL_2 0x02
#define BTN_COL_3 0x03
#define BTN_COL_4 0x04
#define BTN_COL_5 0x05

#define BTN_NONE 0x00
#define BTN_UP (BTN_ROW_1 | BTN_COL_1)
#define BTN_DOWN (BTN_ROW_1 | BTN_COL_2)
#define BTN_LEFT (BTN_ROW_1 | BTN_COL_3)
#define BTN_RIGHT (BTN_ROW_1 | BTN_COL_4)
#define BTN_F1 (BTN_ROW_2 | BTN_COL_1)
#define BTN_F2 (BTN_ROW_2 | BTN_COL_2)
#define BTN_F3 (BTN_ROW_2 | BTN_COL_3)
#define BTN_F4 (BTN_ROW_2 | BTN_COL_4)
#define BTN_RED (BTN_ROW_3 | BTN_COL_1)
#define BTN_ORG (BTN_ROW_3 | BTN_COL_2)
#define BTN_YEL (BTN_ROW_3 | BTN_COL_3)
#define BTN_GRN (BTN_ROW_3 | BTN_COL_4)
#define BTN_BLU (BTN_ROW_3 | BTN_COL_5)
#define BTN_PUR (BTN_ROW_4 | BTN_COL_1)
#define BTN_ROT (BTN_ROW_4 | BTN_COL_2)
#define BTN_BACK (BTN_ROW_4 | BTN_COL_3)
#define BTN_OK (BTN_ROW_4 | BTN_COL_5)


    // TODO: consider enabling hysteresis?
    PIN_State btn_state;
    // The ROW SCAN sets ROWS as INPUTS, with PULL DOWNS
    //              and, COLS as OUTPUTS, HIGH.
    PIN_Config btn_row_scan[] = {
           QC16_PIN_KP_ROW_1 | PIN_INPUT_EN | PIN_PULLDOWN,
           QC16_PIN_KP_ROW_2 | PIN_INPUT_EN | PIN_PULLDOWN,
           QC16_PIN_KP_ROW_3 | PIN_INPUT_EN | PIN_PULLDOWN,
           QC16_PIN_KP_ROW_4 | PIN_INPUT_EN | PIN_PULLDOWN,
           QC16_PIN_KP_COL_1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH, // | PIN_OPENSOURCE,
           QC16_PIN_KP_COL_2 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH, // | PIN_OPENSOURCE,
           QC16_PIN_KP_COL_3 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH, // | PIN_OPENSOURCE,
           QC16_PIN_KP_COL_4 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH, // | PIN_OPENSOURCE,
           QC16_PIN_KP_COL_5 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH, // | PIN_OPENSOURCE,
           PIN_TERMINATE
    };

    PIN_Handle kb_pin_h;
    kb_pin_h = PIN_open(&btn_state, btn_row_scan);

    uint8_t button_press = BTN_NONE;
    uint8_t button_press_prev = BTN_NONE;
    uint8_t kb_mashed;
    while (1) {
        Task_sleep(100);
        button_press_prev = button_press;
        button_press = BTN_NONE;
        kb_mashed = 0;
        // 1 system tick is probably 10 us (0.1 ms)
        // So, 100 ticks == 10ms == 1cs
        // SET ROWS AS INPUTS, WITH PULLDOWNS
        // SET COLS AS OUTPUTS HIGH
        PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_1, 1);
        PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_2, 1);
        PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_3, 1);
        PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_4, 1);
        PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_5, 1);
        // ...
        // Are any ROWS HIGH?
        for (uint8_t r=1; r<5; r++) {
            // this is row r
            // row pin to read is r+QC16_PIN_KP_ROW_1-1
            PIN_Id pin_to_read;
            if (r < 4)
                pin_to_read = QC16_PIN_KP_ROW_1 + r - 1;
            else
                pin_to_read = QC16_PIN_KP_ROW_4;
            if (PIN_getInputValue(pin_to_read)) {
                // high => pressed
                if (button_press & 0xF0) {
                    // another row already pressed, kb is mashed.
                    kb_mashed = 1;
                    break;
                }
                button_press = r << 4; // row is the upper nibble.
                // Now, figure out which column it is. Set all columns low:
                PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_1, 0);
                PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_2, 0);
                PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_3, 0);
                PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_4, 0);
                PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_5, 0);

                // set each column high, in turn:
                for (uint8_t c=1; c<6; c++) {
                    PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_1 + c - 1, 1);
                    if (PIN_getInputValue(pin_to_read)) {
                        // If the relevant row reads high, this col,row is pressed.
                        if (button_press & 0x0F) {
                            // if multiple columns are pressed already, then
                            // we have a mashing situation on our hands.
                            kb_mashed = 1;
                        }
                        button_press |= c; // row is already in the upper nibble.
                    }
                    PIN_setOutputValue(kb_pin_h, QC16_PIN_KP_COL_1 + c - 1, 0);
                }

            }
        }

        // TODO: Check whether it's BOTH a row and col detected.
        //  If MULTIPLE, ignore due to mashing.
        if (kb_mashed) {
            // ignore.
            // do ignoring things.
        } else {
            if (button_press && button_press == button_press_prev) {
                // A button is pressed.
                Task_sleep(99);
                // TODO
                continue;
            }
        }
    }

    for (;;)
    {
        // Get the ticks since startup
        uint32_t tickStart = Clock_getTicks();
        Task_yield();
    }
}

int main()
{
    Power_init();
    if (PIN_init(qc16_pin_init_table) != PIN_SUCCESS) {
        /* Error with PIN_init */
        // TODO
        while (1);
    }
    SPI_init();
    I2C_init();
#ifdef CACHE_AS_RAM
    // retain cache during standby
    Power_setConstraint(PowerCC26XX_SB_VIMS_CACHE_RETAIN);
    Power_setConstraint(PowerCC26XX_NEED_FLASH_IN_IDLE);
#else
    // Enable iCache prefetching
    VIMSConfigure(VIMS_BASE, TRUE, TRUE);
    // Enable cache
    VIMSModeSet(VIMS_BASE, VIMS_MODE_ENABLED);
#endif //CACHE_AS_RAM

    /* Create Application task. */
    UBLEBcastScan_createTask();

    // Set up the epaper task:
    Task_Params taskParams;
    // Configure task
    Task_Params_init(&taskParams);
    taskParams.stack = epaperTaskStack;
    taskParams.stackSize = EPD_STACKSIZE;
    taskParams.priority = 1;
    Task_construct(&epaperTask, epaper_spi_task_fn, &taskParams, NULL);

    BIOS_start();     /* enable interrupts and start SYS/BIOS */

    return 0;
}

