/*
 * leds.c
 *
 *  Created on: Jun 15, 2019
 *      Author: george
 */

#include <stdint.h>
#include <string.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Clock.h>

#include <qc16.h>
#include <badge.h>
#include <qbadge.h>
#include <post.h>
#include "queercon_drivers/ht16d35b.h"
#include <ui/leds.h>
#include <ui/adc.h>

led_tail_anim_t led_tail_anim_current = {
    .type=LED_TAIL_ANIM_TYPE_CYCLE,
    .modifier = LED_TAIL_ANIM_MOD_NORMAL,
    .colors= {
              {255<<7, 0, 0},  // Red
              {255<<7, 20<<7, 0}, // Orange
              {255<<7, 60<<7, 0}, // Yellow
              {0, 64<<7, 0},   // Green
              {0, 0, 144<<7},  // Blue
              {128<<7, 0, 96<<7}, // Purple
    }
};

const uint8_t led_tail_anim_color_counts[LED_TAIL_ANIM_TYPE_COUNT] = {
    0,  //    LED_TAIL_ANIM_TYPE_OFF,
    1,  //    LED_TAIL_ANIM_TYPE_ON,
    6,  //    LED_TAIL_ANIM_TYPE_CYCLE,
    6,  //    LED_TAIL_ANIM_TYPE_FADE,
    6,  //    LED_TAIL_ANIM_TYPE_SCROLL,
    6,  //    LED_TAIL_ANIM_TYPE_SCROLLFADE,
    6,  //    LED_TAIL_ANIM_TYPE_PANES,
    3,  //    LED_TAIL_ANIM_TYPE_BUBBLE,
    1,  //    LED_TAIL_ANIM_TYPE_FLASH,
    1,  //    LED_TAIL_ANIM_TYPE_FIRE,
};

const uint16_t BRIGHTNESS_STEPS[LED_NUM_BRIGHTNESS_STEPS][2] = {
    {20, 24},
    {50, 40},
    {55, 45},
    {60, 50},
    {65, 55},
    {70, 63}, // "full brightness" (w/ bezel)
};

rgbcolor16_t led_tail_src[6];
/// Current colors of the LED dustbuster
rgbcolor16_t led_tail_curr[6];
/// Destination colors of the dustbuster
rgbcolor16_t led_tail_dest[6];
/// Step values of the dustbuster
rgbdelta_t led_tail_step[6];
uint16_t led_tail_steps_per_frame;
uint16_t led_tail_frames_this_anim;
uint16_t led_tail_frame_curr;
uint16_t led_tail_frame_next;
uint16_t led_tail_step_curr;

#define LED_STACKSIZE 1024
Task_Struct led_task;
uint8_t led_task_stack[LED_STACKSIZE];
Event_Handle led_event_h;

Clock_Handle led_tail_clock_h;

rgbcolor16_t led_rainbow_colors[6] = {
    {255<<7, 0, 0},  // Red
    {255<<7, 20<<7, 0}, // Orange
    {255<<7, 60<<7, 0}, // Yellow
    {0, 64<<7, 0},   // Green
    {0, 0, 144<<7},  // Blue
    {128<<7, 0, 96<<7}, // Purple
};

rgbcolor16_t led_fn_colors[3] = {
    {0xff00, 0x0000, 0xb000}, // pink (locks)
    {0xd500, 0x0000, 0x6900}, // other pink (coins)
    {0x1B00, 0xCE00, 0xFA00}, // blue (cameras)
};

rgbcolor16_t led_off = {0, 0, 0};
rgbcolor16_t led_white = {0xfff, 0xfff, 0xfff};
rgbcolor16_t led_white_full = {0xffff, 0xffff, 0xffff};

uint8_t led_tail_anim_type_is_valid(led_tail_anim_type t) {
    return t < 3;
}

void led_tail_anim_type_next() {
    led_tail_anim_type next_type = led_tail_anim_current.type;
    do {
        next_type += 1;
        if (next_type >= LED_TAIL_ANIM_TYPE_COUNT)
            next_type = (led_tail_anim_type) 0;
    } while (!led_tail_anim_type_is_valid(next_type));
    led_tail_anim_current.type = next_type;
    led_tail_start_anim();
}

void led_tail_anim_type_prev() {
    led_tail_anim_type next_type = led_tail_anim_current.type;
    do {
        if (next_type == 0) {
            next_type = LED_TAIL_ANIM_TYPE_COUNT;
        }
        next_type -= 1;
    } while (!led_tail_anim_type_is_valid(next_type));
    led_tail_anim_current.type = next_type;
    led_tail_start_anim();
}

void led_flush() {
    ht16d_send_gray();
}

void led_show_curr_colors() {
    uint8_t count = led_tail_anim_color_counts[led_tail_anim_current.type];
    if (count == 0) {
        ht16d_put_color(6, 6, &led_off);
        // Turn off the lights if there are no lights in this animation.
        // If count == 0, the for loop will be skipped.
    }

    for (uint8_t i=0; i<count; i++) {
        ht16d_put_color(6+i*(6/count), 6/count, &led_tail_anim_current.colors[i]);
    }

    Event_post(led_event_h, LED_EVENT_FLUSH);
}

/// Start the current frame of the LED animation.
void led_tail_frame_setup() {
    led_tail_frame_next = (led_tail_frame_curr+1)%led_tail_frames_this_anim;

    switch(led_tail_anim_current.type) {
    case LED_TAIL_ANIM_TYPE_CYCLE:
        for (uint8_t i=0; i<6; i++) {
            led_tail_src[i].r = led_tail_anim_current.colors[led_tail_frame_curr].r;
            led_tail_src[i].g = led_tail_anim_current.colors[led_tail_frame_curr].g;
            led_tail_src[i].b = led_tail_anim_current.colors[led_tail_frame_curr].b;

            led_tail_dest[i].r = led_tail_anim_current.colors[led_tail_frame_next].r;
            led_tail_dest[i].g = led_tail_anim_current.colors[led_tail_frame_next].g;
            led_tail_dest[i].b = led_tail_anim_current.colors[led_tail_frame_next].b;
        }
        break;
    }

    memcpy(led_tail_curr, led_tail_src, sizeof(rgbcolor16_t)*6);

    ht16d_put_colors(0, 6, led_tail_curr);
    Event_post(led_event_h, LED_EVENT_FLUSH); // ready to show.
}

void led_tail_timestep() {
    led_tail_step_curr += 1;
    if (led_tail_step_curr >= led_tail_steps_per_frame) {
        led_tail_step_curr = 0;
        led_tail_frame_curr += 1;
        if (led_tail_frame_curr == led_tail_frames_this_anim) {
            led_tail_frame_curr = 0;
        }
        led_tail_frame_setup();
    } else {
        // This will only be encountered for an animation that fades
        //  (that is, an animation with more than one step per frame)
        for (uint8_t i=0; i<6; i++) {
            led_tail_step[i].r = ((int32_t)led_tail_dest[i].r - led_tail_src[i].r) / led_tail_steps_per_frame;
            led_tail_step[i].g = ((int32_t)led_tail_dest[i].g - led_tail_src[i].g) / led_tail_steps_per_frame;
            led_tail_step[i].b = ((int32_t)led_tail_dest[i].b - led_tail_src[i].b) / led_tail_steps_per_frame;

            led_tail_curr[i].r = led_tail_src[i].r + (led_tail_step_curr * led_tail_step[i].r);
            led_tail_curr[i].g = led_tail_src[i].g + (led_tail_step_curr * led_tail_step[i].g);
            led_tail_curr[i].b = led_tail_src[i].b + (led_tail_step_curr * led_tail_step[i].b);
        }

        ht16d_put_colors(0, 6, led_tail_curr);
        Event_post(led_event_h, LED_EVENT_FLUSH); // ready to show.
    }

    Clock_setTimeout(led_tail_clock_h, 100000/led_tail_steps_per_frame);
    Clock_start(led_tail_clock_h);
}

void led_tail_start_anim() {
    Clock_stop(led_tail_clock_h);
    memset(led_tail_src, 0x00, sizeof(rgbcolor16_t)*6);
    memset(led_tail_dest, 0x00, sizeof(rgbcolor16_t)*6);
    memset(led_tail_src, 0x00, sizeof(rgbcolor16_t)*6);
    memset(led_tail_step, 0x00, sizeof(rgbdelta_t)*6);
    led_tail_frame_curr = 0;
    led_tail_step_curr = 0;

    switch(led_tail_anim_current.type) {
    case LED_TAIL_ANIM_TYPE_OFF:
        ht16d_put_color(0, 6, &led_off);
        Event_post(led_event_h, LED_EVENT_FLUSH); // ready to show.
        // No animating; just set the color.
        return;
    case LED_TAIL_ANIM_TYPE_ON:
        ht16d_put_color(0, 6, &led_tail_anim_current.colors[0]);
        Event_post(led_event_h, LED_EVENT_FLUSH); // ready to show.
        // No animating; just set the color.
        return;
    case LED_TAIL_ANIM_TYPE_CYCLE:
        led_tail_steps_per_frame = 100;
        led_tail_frames_this_anim = 6;
        break;
    }

    led_tail_frame_setup();

    Clock_setTimeout(led_tail_clock_h, 100000/led_tail_steps_per_frame);
    Clock_start(led_tail_clock_h);
}

void led_tail_step_swi(UArg a0) {
    Event_post(led_event_h, LED_EVENT_TAIL_STEP);
}

void led_sidelight_set_color(rgbcolor16_t *color) {
    ht16d_put_color(12, 12, color);
    ht16d_send_gray();
//    Event_post(led_event_h, LED_EVENT_FLUSH);
}

void led_sidelight_activate() {
    led_sidelight_set_color(&led_white_full);
}

void led_sidelight_deactivate() {
    led_sidelight_set_color(&led_off);
}

void led_element_light() {
    ht16d_put_color(24, 3, &led_off);

    Event_post(led_event_h, LED_EVENT_FLUSH);

    if (badge_conf.element_selected > ELEMENT_CAMERAS) {
        // no element selected, so we're done.
        return;
    }

    ht16d_put_color(24+(1+(uint8_t)badge_conf.element_selected)%3, 1, &led_fn_colors[(uint8_t)badge_conf.element_selected]);
}

void led_adjust_brightness() {
    ht16d_set_global_brightness(BRIGHTNESS_STEPS[brightness][1]);
    if (brightness < BRIGHTNESS_LEVEL_SIDELIGHTS_THRESH) {
        // Dim enough that we want to turn on the sidelights.
        led_sidelight_activate();
    } else {
        // Bright enough that we want to turn off the sidelights.
        led_sidelight_deactivate();
    }
}

void led_task_fn(UArg a0, UArg a1) {
    UInt events;

    ht16d_init();
    ht16d_all_one_color(0,0,0);

    if (post_status_leds < 0) {
        while (1) {
            // If the LEDs are broken, don't actually use them.
            Task_yield();
        }
    }

    while (1) {
        events = Event_pend(led_event_h, Event_Id_NONE, ~Event_Id_NONE,  BIOS_WAIT_FOREVER);

        if (events & LED_EVENT_FLUSH) {
            led_flush();
        }

        if (events & LED_EVENT_SHOW_UPCONF) {
            led_show_curr_colors();
        }

        if (events & LED_EVENT_HIDE_UPCONF) {
            ht16d_put_color(6, 6, &led_off);
            Event_post(led_event_h, LED_EVENT_FLUSH);
        }


        if (events & LED_EVENT_TAIL_STEP) {
            led_tail_timestep();
        }

        if (events & LED_EVENT_FN_LIGHT) {
            led_element_light();
        }

        if (events & LED_EVENT_BRIGHTNESS) {
            led_adjust_brightness();
        }

        if (events & LED_EVENT_SIDE_ON) {
            led_sidelight_activate();
        }

        if (events & LED_EVENT_SIDE_OFF) {
            led_sidelight_deactivate();
        }

        if (events & LED_EVENT_SIDE_OFF_AND_TRIGGER_ADC) {
            ht16d_put_color(12, 12, &led_off);
            ht16d_send_gray();
            adc_trigger_light();
        }
    }
}

void led_init() {
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stack = led_task_stack;
    taskParams.stackSize = LED_STACKSIZE;
    taskParams.priority = 1;
    Task_construct(&led_task, led_task_fn, &taskParams, NULL);

    Clock_Params clockParams;
    Clock_Params_init(&clockParams);
    clockParams.period = 0; // one-shot clock.
    clockParams.startFlag = FALSE;
    led_tail_clock_h = Clock_create(led_tail_step_swi, 100, &clockParams, NULL);
}
