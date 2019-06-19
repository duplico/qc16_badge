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

#include "queercon_drivers/ht16d35b.h"
#include "ui/leds.h"

led_tail_anim_t led_tail_anim_current = {
    .speed=1,
    .type=LED_TAIL_ANIM_TYPE_CYCLE,
    .colors= {
              {255<<7, 0, 0},  // Red
              {255<<7, 20<<7, 0}, // Orange
              {255<<7, 60<<7, 0}, // Yellow
              {0, 64<<7, 0},   // Green
              {0, 0, 144<<7},  // Blue
              {128<<7, 0, 96<<7}, // Purple
    }
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

rgbcolor16_t led_off = {0, 0, 0};

uint8_t led_tail_anim_type_is_valid(led_tail_anim_type t) {
    return t < 2;
}

void led_tail_anim_type_next() {
    led_tail_anim_type next_type = led_tail_anim_current.type;
    do {
        next_type += 1;
        if (next_type >= LED_TAIL_ANIM_TYPE_COUNT)
            next_type = 0;
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
    ht16d_put_colors(6, 6, led_tail_anim_current.colors);
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
        for (uint8_t i=0; i<6; i++) {
            led_tail_step[i].r = ((int32_t)led_tail_dest[i].r - led_tail_src[i].r) / led_tail_steps_per_frame;
            led_tail_step[i].g = ((int32_t)led_tail_dest[i].g - led_tail_src[i].g) / led_tail_steps_per_frame;
            led_tail_step[i].b = ((int32_t)led_tail_dest[i].b - led_tail_src[i].b) / led_tail_steps_per_frame;

            led_tail_curr[i].r = led_tail_src[i].r + (led_tail_step_curr * led_tail_step[i].r);
            led_tail_curr[i].g = led_tail_src[i].g + (led_tail_step_curr * led_tail_step[i].g);
            led_tail_curr[i].b = led_tail_src[i].b + (led_tail_step_curr * led_tail_step[i].b);
        }

        switch(led_tail_anim_current.type) {
        case LED_TAIL_ANIM_TYPE_CYCLE:
            // we're not doing any stepping here because it's not one of
            // the animations that fades.
            break;
        }
        ht16d_put_colors(0, 6, led_tail_curr);
        Event_post(led_event_h, LED_EVENT_FLUSH); // ready to show.
    }

    Clock_setTimeout(led_tail_clock_h, 100000/led_tail_steps_per_frame); // TODO: this is just 1 second for now.
    Clock_start(led_tail_clock_h);
}

void led_tail_start_anim() {
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
        return;
        break;
    case LED_TAIL_ANIM_TYPE_CYCLE:
        led_tail_steps_per_frame = 100;
        led_tail_frames_this_anim = 6;
        break;
    }

    led_tail_frame_setup();

    Clock_setTimeout(led_tail_clock_h, 100000/led_tail_steps_per_frame); // TODO: this is just 1 second per frame now.
    Clock_start(led_tail_clock_h);
}

void led_tail_step_swi(UArg a0) {
    Event_post(led_event_h, LED_EVENT_TAIL_STEP);
}

void led_task_fn(UArg a0, UArg a1) {
    UInt events;
    led_event_h = Event_create(NULL, NULL);

    ht16d_init();
    ht16d_all_one_color(0,0,0);
    led_tail_start_anim();

    while (1) {
        events = Event_pend(led_event_h, Event_Id_NONE, ~Event_Id_NONE,  BIOS_WAIT_FOREVER);

        if (events & LED_EVENT_SHOW_UPCONF) {
            led_show_curr_colors();
        }

        if (events & LED_EVENT_HIDE_UPCONF) {
            ht16d_put_color(6, 6, &led_off);
            Event_post(led_event_h, LED_EVENT_FLUSH);
        }

        if (events & LED_EVENT_FLUSH) {
            led_flush();
        }

        if (events & LED_EVENT_TAIL_STEP) {
            led_tail_timestep();
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
