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
    init_epd(0);




    volatile int32_t status;

    status = SPIFFSNVS_config(&spiffsnvs, QC16_NVSSPI25X0, &fs, &fsConfig,
                              SPIFFS_LOGICAL_BLOCK_SIZE, SPIFFS_LOGICAL_PAGE_SIZE);
    if (status != SPIFFSNVS_STATUS_SUCCESS) {
        while (1); // Spin forever.
    }
    status = SPIFFS_mount(&fs, &fsConfig, spiffsWorkBuffer,
        spiffsFileDescriptorCache, sizeof(spiffsFileDescriptorCache),
        spiffsReadWriteCache, sizeof(spiffsReadWriteCache), NULL);

    if (status == SPIFFS_ERR_NOT_A_FS) {
        // Needs to be formatted before mounting.
        status = SPIFFS_format(&fs);
        status = SPIFFS_mount(&fs, &fsConfig, spiffsWorkBuffer,
            spiffsFileDescriptorCache, sizeof(spiffsFileDescriptorCache),
            spiffsReadWriteCache, sizeof(spiffsReadWriteCache), NULL);
    }

    status = SPIFFS_creat(&fs, "testfile", 0);
    spiffs_file f;
    f = SPIFFS_open(&fs, "testfile", SPIFFS_O_CREAT + SPIFFS_O_WRONLY, 0);
    status = SPIFFS_write(&fs, f, "test", 4);
    status = SPIFFS_close(&fs, f);

    spiffs_stat stat;
    status = SPIFFS_stat(&fs, "testfile", &stat);

    char buf[10];
    f = SPIFFS_open(&fs, "testfile", SPIFFS_O_RDONLY, 0);
    status = SPIFFS_read(&fs, f, (void *)buf, 4);

//    NVS_Handle nvsHandle;
//    NVS_Attrs regionAttrs;
//    NVS_Params nvsParams;

//    NVS_init();
//    NVS_Params_init(&nvsParams);
//    nvsHandle = NVS_open(Board_NVSEXTERNAL, &nvsParams);
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
//    NVS_read(nvsHandle, 0, (void *) buffer, sizeof(signature));
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
//        NVS_erase(nvsHandle, 0, regionAttrs.sectorSize);
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
//        NVS_write(nvsHandle, 0, (void *) signature, sizeof(signature),
//            NVS_WRITE_ERASE | NVS_WRITE_POST_VERIFY);
//    }



    Graphics_Context gr_context;
    Graphics_initContext(&gr_context, &epd_grGraphicsDisplay, &epd_grDisplayFunctions);
    Graphics_setBackgroundColor(&gr_context, GRAPHICS_COLOR_WHITE);
    Graphics_setForegroundColorTranslated(&gr_context, GRAPHICS_COLOR_BLACK);
    Graphics_clearDisplay(&gr_context);
    Graphics_fillCircle(&gr_context, 128, 64, 32);
    Graphics_drawLine(&gr_context, 0, 16, 295, 16);
    Graphics_setFont(&gr_context, &g_sFontCm14);
    Graphics_drawString(&gr_context, "Queercon 2019", 13, 16, 16, 0);
    Graphics_drawString(&gr_context, buf, status, 16, 32, 0);
    epd_flip();
    Graphics_flushBuffer(&gr_context);

    for (;;)
    {
        // Get the ticks since startup
        uint32_t tickStart = Clock_getTicks();
        Task_yield();
    }
}

int main()
{
    PIN_init(BoardGpioInitTable);
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

