/*
 * adc.c
 *
 *  Created on: Jul 14, 2019
 *      Author: george
 */

#include <stdint.h>

#include <xdc/runtime/Error.h>
#include <ti/drivers/ADCBuf.h>
#include <driverlib/aon_batmon.h>
#include <ti/sysbios/knl/Clock.h>

#include <board.h>

#include <qbadge.h>

#include <ui/leds.h>

uint16_t vbat_raw;
uint16_t brightness_raw;
uint8_t brightness = 0x10;
Clock_Handle adc_clock_h;

ADCBuf_Handle adc_buf_h;
ADCBuf_Conversion next_conversion;

void adc_cb(ADCBuf_Handle handle, ADCBuf_Conversion *conversion,
    void *completedADCBuffer, uint32_t completedChannel) {
    // TODO: Determine when to set the battery event flag.
    uint8_t target_brightness_level;

    ADCBuf_adjustRawValues(handle, completedADCBuffer, 1, completedChannel);

    switch(completedChannel) {
    case ADCBUF_CH_LIGHT:
        // This is a light.
        target_brightness_level = 0;
        while (target_brightness_level < (LED_NUM_BRIGHTNESS_STEPS-1) &&
                brightness_raw > BRIGHTNESS_STEPS[target_brightness_level][0]) {
            target_brightness_level++;
        }

        if (brightness != target_brightness_level) {
            if (target_brightness_level>brightness)
                brightness++;
            else
                brightness--;

            // TODO: Event flag to the LED system
            if (brightness == LED_NUM_BRIGHTNESS_STEPS - 1) {
                // TODO: unlock something for it being very bright.
            }

            Event_post(led_event_h, LED_EVENT_BRIGHTNESS);
        }
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
            // TODO: Unlock the ICE (?) animation
        } else if (AONBatMonTemperatureGetDegC() > 37) {
            // it's HOT! (over 100 F)
            // TODO: Unlock the FIRE animation
        }
    }
}

void adc_init() {
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
    clockParams.period = ADC_INTERVAL;
    clockParams.startFlag = TRUE;
    adc_clock_h = Clock_create(adc_timer_fn, 2, &clockParams, &eb);
}
