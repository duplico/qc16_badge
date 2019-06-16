/*
 * leds.h
 *
 *  Created on: Jun 15, 2019
 *      Author: george
 */

#ifndef QUEERCON_DRIVERS_LEDS_H_
#define QUEERCON_DRIVERS_LEDS_H_

#include "queercon_drivers/ht16d35b.h"

typedef enum {
    LED_TAIL_ANIM_TYPE_OFF = 0,
    LED_TAIL_ANIM_TYPE_SCROLL,
    LED_TAIL_ANIM_TYPE_CYCLE,
    LED_TAIL_ANIM_TYPE_BUBBLE,
    LED_TAIL_ANIM_TYPE_FLASH,
    LED_TAIL_ANIM_TYPE_COUNT
} led_tail_anim_type;

typedef struct {
    led_tail_anim_type type;
    rgbcolor16_t colors[6];
    uint16_t speed;
} led_tail_anim_t;

extern led_tail_anim_t led_tail_anim_current;

#endif /* QUEERCON_DRIVERS_LEDS_H_ */
