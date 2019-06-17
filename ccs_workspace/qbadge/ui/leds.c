/*
 * leds.c
 *
 *  Created on: Jun 15, 2019
 *      Author: george
 */


// TODO: UGH. This needs another thread.

#include <stdint.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>

#include "queercon_drivers/ht16d35b.h"
#include "ui/leds.h"

led_tail_anim_t led_tail_anim_current;

rgbcolor16_t led_tail_src[6];
/// Current colors of the LED dustbuster
rgbcolor16_t led_tail_curr[6];
/// Destination colors of the dustbuster
rgbcolor16_t led_tail_dest[6];
/// Step values of the dustbuster
rgbdelta_t led_tail_step[6];


#define LED_STACKSIZE 1024
Task_Struct led_task;
uint8_t led_task_stack[LED_STACKSIZE];
Event_Handle led_events_h;

rgbcolor16_t led_rainbow_colors[6] = {
    {255<<7, 0, 0},  // Red
    {255<<7, 20<<7, 0}, // Orange
    {255<<7, 60<<7, 0}, // Yellow
    {0, 64<<7, 0},   // Green
    {0, 0, 144<<7},  // Blue
    {128<<7, 0, 96<<7}, // Purple
};

rgbcolor16_t led_off = {0, 0, 0};

void led_flush() {
    ht16d_send_gray();
}

void led_show_curr_colors() {
    ht16d_put_colors(6, 6, led_tail_anim_current.colors);
    Event_post(led_events_h, LED_EVENT_FLUSH);
}

// Call this from a task context when it's time to do a thing.
void led_tail_timestep() {

}

void led_task_fn(UArg a0, UArg a1) {
    UInt events;
    led_events_h = Event_create(NULL, NULL);
    while (1) {
        events = Event_pend(led_events_h, Event_Id_NONE, ~Event_Id_NONE,  BIOS_WAIT_FOREVER);

        if (events & LED_EVENT_SHOW_UPCONF) {
            led_show_curr_colors();
        }

        if (events & LED_EVENT_HIDE_UPCONF) {
            ht16d_put_color(6, 6, &led_off);
            Event_post(led_events_h, LED_EVENT_FLUSH);
        }

        if (events & LED_EVENT_FLUSH) {
            led_flush();
        }
    }
}

void led_init() {
    // TODO: load current tail animation

    // TODO: rename all the taskParams and such
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stack = led_task_stack;
    taskParams.stackSize = LED_STACKSIZE;
    taskParams.priority = 1;
    Task_construct(&led_task, led_task_fn, &taskParams, NULL);
}
