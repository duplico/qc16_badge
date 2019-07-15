/*
 * adc.h
 *
 *  Created on: Jul 14, 2019
 *      Author: george
 */

#ifndef UI_ADC_H_
#define UI_ADC_H_

extern Clock_Handle adc_clock_h;

extern uint8_t brightness;

void adc_init();
void adc_timer_fn(UArg a0);

#endif /* UI_ADC_H_ */
