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
extern uint16_t handlers_near_count;
extern uint16_t handlers_near_count_running;

extern uint8_t mission_accepted[3];
extern mission_t missions[3];

uint8_t mission_getting_possible();
mission_t generate_mission();
void mission_begin_by_id(uint8_t mission_id);
void complete_mission(mission_t *mission);
void complete_mission_id(uint8_t mission_id);
uint8_t mission_solo_qualifies(uint8_t mission_id);
uint8_t mission_qualified_for_element_id(mission_t *mission, uint8_t element_position);

void load_conf();
void write_conf();
void config_init();
uint8_t badge_seen(uint16_t id);
uint8_t badge_connected(uint16_t id);
uint8_t set_badge_seen(uint16_t id, char *name);
uint8_t set_badge_connected(uint16_t id, char *handle);
void write_anim_curr();
void save_anim(char *name);
void load_anim_abs(char *pathname);
void load_anim(char *name);

#endif /* BADGE_H_ */
