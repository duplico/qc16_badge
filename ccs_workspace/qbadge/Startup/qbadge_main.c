#include <string.h>

#include <xdc/runtime/Error.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/NVS.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/ADCBuf.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/uart/UARTCC26XX.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <driverlib/aon_batmon.h>

#include <board.h>

#include <hal_assert.h>
#include <bcomdef.h>

#include "uble_bcast_scan.h"

/* Header files required to enable instruction fetch cache */
#include <inc/hw_memmap.h>
#include <driverlib/vims.h>
#include <queercon_drivers/qbadge_serial.h>
#include "queercon_drivers/epd_phy.h"
#include <queercon_drivers/storage.h>

#include "qbadge.h"
#include <qc16.h>
#include <badge.h>

#include "ui/ui.h"
#include "ui/leds.h"

uint32_t vbat_out_uvolts = 0;
uint16_t vbat_raw;
uint16_t brightness_raw;
Clock_Handle adc_clock_h;

ADCBuf_Handle adc_buf_h;
ADCBuf_Conversion next_conversion;

void adc_cb(ADCBuf_Handle handle, ADCBuf_Conversion *conversion,
    void *completedADCBuffer, uint32_t completedChannel) {
    // TODO: Determine when to set the battery event flag.

    ADCBuf_adjustRawValues(handle, completedADCBuffer, 1, completedChannel);

    switch(completedChannel) {
    case ADCBUF_CH_LIGHT:
        // This is a light.
        break;
    case ADCBUF_CH_VBAT:
        ADCBuf_convertAdjustedToMicroVolts(handle, completedChannel, completedADCBuffer, &vbat_out_uvolts, conversion->samplesRequestedCount);
        break;
    }
}

void adc_timer_fn(UArg a0)
{
    static QC16_ADCBuf0ChannelName curr_channel = ADCBUF_CH_VBAT;
    volatile int_fast16_t res;

    if (curr_channel == ADCBUF_CH_LIGHT) {
        next_conversion.arg = NULL;
        next_conversion.sampleBufferTwo = NULL;
        next_conversion.adcChannel = ADCBUF_CH_LIGHT;
        next_conversion.sampleBuffer = &brightness_raw;
        next_conversion.samplesRequestedCount = 1;
        curr_channel = ADCBUF_CH_VBAT;
    } else {
        next_conversion.arg = NULL;
        next_conversion.sampleBufferTwo = NULL;
        next_conversion.adcChannel = ADCBUF_CH_VBAT;
        next_conversion.sampleBuffer = &vbat_raw;
        next_conversion.samplesRequestedCount = 1;
        curr_channel = ADCBUF_CH_LIGHT;
    }

    res = ADCBuf_convert(adc_buf_h, &next_conversion, 1);

    if (AONBatMonNewTempMeasureReady()) {
        if (AONBatMonTemperatureGetDegC() < 6) { // deg C (-256..255)
            // it's COLD!
        } else if (AONBatMonTemperatureGetDegC() > 37) {
            // it's HOT! (over 100 F)
        }
    }
}

int main()
{
    Power_init();
    if (PIN_init(qc16_pin_init_table) != PIN_SUCCESS) {
        while (1);
    }
    SPI_init();
    I2C_init();
    ADCBuf_init();
    UART_init();

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

    // Create the BLE task:
    UBLEBcastScan_createTask();
    // Create the UI tasks:
    ui_init();
    serial_init();
    led_init();
    epd_phy_init();

    // Set up the ADC reader clock & buffer
    Clock_Params clockParams;
    Error_Block eb;
    Error_init(&eb);

    ADCBuf_Params adc_buf_params;

    ADCBuf_Params_init(&adc_buf_params);
    adc_buf_params.returnMode = ADCBuf_RETURN_MODE_CALLBACK;
    adc_buf_params.blockingTimeout = NULL;
    adc_buf_params.callbackFxn = adc_cb;
    adc_buf_params.recurrenceMode = ADCBuf_RECURRENCE_MODE_ONE_SHOT;
    adc_buf_params.samplingFrequency = 200;

    adc_buf_h = ADCBuf_open(QC16_ADCBUF0, &adc_buf_params);

    Clock_Params_init(&clockParams);
    clockParams.period = LED_BRIGHTNESS_INTERVAL;
    clockParams.startFlag = TRUE;
    adc_clock_h = Clock_create(adc_timer_fn, 2, &clockParams, &eb);

    BIOS_start();     /* enable interrupts and start SYS/BIOS */

    return 0;
}

