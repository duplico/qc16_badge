/*
 * badge.h
 *
 *  Created on: Jun 29, 2019
 *      Author: george
 */

#ifndef BADGE_H_
#define BADGE_H_

#include <stdint.h>

#include <qc16.h>

extern qbadge_conf_t badge_conf;

extern volatile uint32_t qc_clock;

extern uint32_t qbadges_near[21];
extern uint32_t qbadges_seen[21];
extern uint32_t qbadges_connected[21];
extern uint32_t cbadges_connected[47];
extern uint16_t qbadges_near_count;
extern uint16_t handlers_near_count=0;
extern uint16_t handlers_near_count_running=0;

extern uint8_t mission_accepted[3];
extern mission_t missions[3];

void load_conf();
void write_conf();
void init_config();
uint8_t badge_seen(uint16_t id);
uint8_t badge_connected(uint16_t id);
uint8_t set_badge_seen(uint16_t id, char *name);
uint8_t set_badge_connected(uint16_t id, char *handle);
void write_anim_curr();
void save_anim(char *name);
void load_anim(char *name);

#endif /* BADGE_H_ */
