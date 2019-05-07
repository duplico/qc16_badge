#include <xdc/runtime/Error.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
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

#include "queercon/epd_driver.h"
//#include "queercon/epd.h"

// TODO:
//
Task_Struct epaperTask;
Char epaperTaskStack[660];

void epaper_spi_task_fn(UArg a0, UArg a1)
{
    init_epd(0);

    Graphics_Context gr_context;
    Graphics_initContext(&gr_context, &epd_grGraphicsDisplay, &epd_grDisplayFunctions);
    Graphics_setBackgroundColor(&gr_context, GRAPHICS_COLOR_BLACK);
    Graphics_setForegroundColorTranslated(&gr_context, GRAPHICS_COLOR_WHITE);
    Graphics_clearDisplay(&gr_context);
    Graphics_fillCircle(&gr_context, 64, 64, 32);
    Graphics_drawLine(&gr_context, 0, 16, 295, 16);
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

#ifndef POWER_SAVING
  /* Set constraints for Standby, powerdown and idle mode */
  /* PowerCC26XX_SB_DISALLOW may be redundant */
  Power_setConstraint(PowerCC26XX_SB_DISALLOW);
  Power_setConstraint(PowerCC26XX_IDLE_PD_DISALLOW);
#endif /* POWER_SAVING */

  /* Create Application task. */
  UBLEBcastScan_createTask();

  // Set up the epaper task:
  Task_Params taskParams;
  // Configure task
  Task_Params_init(&taskParams);
  taskParams.stack = epaperTaskStack;
  taskParams.stackSize = 660;
  taskParams.priority = 1;
  Task_construct(&epaperTask, epaper_spi_task_fn, &taskParams, NULL);

  BIOS_start();     /* enable interrupts and start SYS/BIOS */

  return 0;
}

