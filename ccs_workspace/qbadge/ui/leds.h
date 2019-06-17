/*
 * leds.h
 *
 *  Created on: Jun 15, 2019
 *      Author: george
 */

#ifndef QUEERCON_DRIVERS_LEDS_H_
#define QUEERCON_DRIVERS_LEDS_H_

#include "queercon_drivers/ht16d35b.h"

#define LED_EVENT_FLUSH         Event_Id_00
#define LED_EVENT_BRIGHTNESS    Event_Id_01
#define LED_EVENT_SHOW_UPCONF   Event_Id_02
#define LED_EVENT_HIDE_UPCONF   Event_Id_03

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

typedef struct {
    int_fast16_t r;
    int_fast16_t g;
    int_fast16_t b;
} rgbdelta_t;

extern Event_Handle led_events_h;
extern led_tail_anim_t led_tail_anim_current;
extern rgbcolor16_t led_rainbow_colors[6];

void led_init();
void led_show_curr_colors();

#endif /* QUEERCON_DRIVERS_LEDS_H_ */
