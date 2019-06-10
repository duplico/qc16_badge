/*
 * buttons.h
 *
 *  Created on: Jun 7, 2019
 *      Author: george
 */

#ifndef BUTTONS_H_
#define BUTTONS_H_

extern char key_pressed;

void button_poll();
void button_calibrate();
void button_poll_new(uint8_t button_id);
void button_measure_start(uint8_t button_id);

#endif /* BUTTONS_H_ */
