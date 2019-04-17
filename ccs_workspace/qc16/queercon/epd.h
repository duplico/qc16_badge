/*
 * epd.h
 *
 *  Created on: Apr 11, 2019
 *      Author: george
 */

#ifndef QUEERCON_EPD_H_
#define QUEERCON_EPD_H_

/*
 * epd.c
 *
 *  Created on: Apr 11, 2019
 *      Author: george
 */

#include <ti/sysbios/knl/Task.h>
#include <board.h>


extern Task_Struct epaperTask;
extern Char epaperTaskStack[660];

#define EPAPER_SDIO     IOID_9
#define EPAPER_SCLK     IOID_10
#define EPAPER_CSN      CC2640R2_LAUNCHXL_DIO0
#define EPAPER_DCN      CC2640R2_LAUNCHXL_DIO1_RFSW
#define EPAPER_RESN     CC2640R2_LAUNCHXL_DIO12
#define EPAPER_BUSY     CC2640R2_LAUNCHXL_DIO15

#define EPD_WIDTH       128
#define EPD_HEIGHT      296

void EPD_WaitUntilIdle(void);
uint8_t EPD_Init(const unsigned char* lut);
void EPD_Clear(void);
void EPD_Display(uint8_t *Image);
void EPD_Sleep(void);
void epaper_spi_task_fn(UArg a0, UArg a1);

#endif /* QUEERCON_EPD_H_ */
