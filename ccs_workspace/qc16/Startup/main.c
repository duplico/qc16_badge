#include <string.h>

#include <xdc/runtime/Error.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/NVS.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/ADC.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/grlib/grlib.h>
#include <driverlib/aon_batmon.h>

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
#include "ui.h"

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

#define ADC_STACKSIZE 512
Task_Struct adc_task;
char adc_task_stack[ADC_STACKSIZE];

/* Buffer placed in RAM to hold bytes read from non-volatile storage. */
static char buffer[64];
static const char signature[] =
    {"SimpleLink SDK Non-Volatile Storage (NVS) SPI Example."};

void epaper_spi_task_fn(UArg a0, UArg a1)
{
//    init_epd(0);

    volatile int32_t status;

    ht16d_init_io();
    ht16d_init();

    ht16d_all_one_color(255,255,255);

    for (;;)
    {
        // Get the ticks since startup
        uint32_t tickStart = Clock_getTicks();
        Task_yield();
    }

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
}

#define LED_BRIGHTNESS_INTERVAL 12500
uint_fast16_t vbat_out = 0;

// TODO: Should we be using ADCBuf so this can be asynchronous? (and run in a clock)
//  Because... the ADC module blocks. Womp.
// TODO: Consider moving into application loop
void led_brightness_task_fn(UArg a0, UArg a1)
{
    ADC_Handle light_adc_h, vbat_adc_h;
    ADC_Params adcp;
    ADC_Params_init(&adcp);
    light_adc_h = ADC_open(QC16_ADC_LIGHT, &adcp);

    ADC_Params_init(&adcp);
    vbat_adc_h = ADC_open(QC16_ADC_VBAT, &adcp);

    int_fast16_t res;
    uint_fast16_t adc_value = 0;

    uint_fast8_t target_brightness_level;
    do {
        res = ADC_convert(light_adc_h, (uint16_t*) &adc_value);
        if (res == ADC_STATUS_SUCCESS) {
            // Do stuff with the ADC status value.
            target_brightness_level = 0;
//            while (target_brightness_level < (LED_NUM_BRIGHTNESS_STEPS-1) &&
//                    adc_value > BRIGHTNESS_STEPS[target_brightness_level][0]) {
//                target_brightness_level++;
//            }
//
//            if (led_global_brightness_level != target_brightness_level) {
//                if (target_brightness_level>led_global_brightness_level)
//                    led_global_brightness_level++;
//                else
//                    led_global_brightness_level--;
//
// TODO: The following is not to use:
//                tlc_msg_fun_base[18] = (0b10000000 & tlc_msg_fun_base[18]) |
//                        (0b01111111 & BRIGHTNESS_STEPS[led_global_brightness_level][1]);
//                tlc_update_fun = 1;
//
//                if (led_global_brightness_level == LED_NUM_BRIGHTNESS_STEPS - 1)
//                    its_bright();
        }

        res = ADC_convert(vbat_adc_h, (uint16_t*) &adc_value);
        if (res == ADC_STATUS_SUCCESS) {
            vbat_out = ADC_convertToMicroVolts(vbat_adc_h, adc_value);
        }

        if (AONBatMonNewTempMeasureReady()) {
            if (AONBatMonTemperatureGetDegC() < 6) { // deg C (-256..255)
                //                its_cold();
                // TODO: do we care about temp?
            }
        }

        Task_sleep(LED_BRIGHTNESS_INTERVAL);
    } while (1);
}

Clock_Handle adc_clock_h;

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
    ADC_init();
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

    ui_init();

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

    Task_Params_init(&taskParams);
    taskParams.stack = adc_task_stack;
    taskParams.stackSize = ADC_STACKSIZE;
    taskParams.priority = 1;
    Task_construct(&adc_task, led_brightness_task_fn, &taskParams, NULL);

//    Clock_Params clockParams;
//    Error_Block eb;
//    Error_init(&eb);
//
//    Clock_Params_init(&clockParams);
//    clockParams.period = LED_BRIGHTNESS_INTERVAL;
//    clockParams.startFlag = TRUE;
//    adc_clock_h = Clock_create(led_brightness_task_fn, 2, &clockParams, &eb);


//    Clock_Params_init(&clockParams);
//    clockParams.period = 0; // One-shot clock.
//    clockParams.startFlag = FALSE;
//    screen_anim_clock_h = Clock_create(screen_anim_tick_swi, 100, &clockParams, NULL); // Wait 100 ticks (1ms) before firing for the first time.
//
//    Clock_Params_init(&clockParams);
//    clockParams.period = 0;
//    clockParams.startFlag = FALSE;
//    arm_anim_clock_h = Clock_create(arm_anim_tick_swi, ARM_ANIM_PERIOD, &clockParams, NULL);
//
//    Clock_Params blink_clock_params;
//    Clock_Params_init(&blink_clock_params);
//    blink_clock_params.period = BLINK_LEN; // 500 ms recurring
//    blink_clock_params.startFlag = FALSE; // Don't auto-start (only when we blink-on)
//    screen_blink_clock_h = Clock_create(screen_blink_tick_swi, 0, &blink_clock_params, NULL);
//
//    Clock_Params csecs_clock_params;
//    Clock_Params_init(&csecs_clock_params);
//    csecs_clock_params.period = 1000; // 10 ms recurring ( 1 centisecond)
//    csecs_clock_params.startFlag = FALSE; // Auto-start off.
//    csecs_clock_h = Clock_create(csecs_swi, 1000, &csecs_clock_params, NULL);



    BIOS_start();     /* enable interrupts and start SYS/BIOS */

    return 0;
}

