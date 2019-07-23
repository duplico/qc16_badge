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

#include <qc16_serial_common.h>

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

extern uint8_t badge_paired;
extern pair_payload_t paired_badge;
extern char handler_name_missionpicking[QC16_BADGE_NAME_LEN+1];
extern char handler_name[QC16_BADGE_NAME_LEN+1];
uint8_t mission_getting_possible();
uint8_t handler_nearby();
mission_t generate_mission();
void mission_begin_by_id(uint8_t mission_id);
void complete_mission(mission_t *mission);
void complete_mission_id(uint8_t mission_id);
uint8_t mission_possible(mission_t *mission);
uint8_t mission_local_qualified_for_element_id(mission_t *mission, uint8_t element_position);
uint8_t mission_remote_qualified_for_element_id(mission_t *mission, uint8_t element_position);
uint8_t mission_element_id_we_fill(mission_t *mission);
uint8_t mission_element_id_remote_fills(mission_t *mission);
uint8_t mission_element_id_fulfilled_by(mission_t *mission, uint8_t *this_element_level, element_type this_element_selected, uint16_t this_id,
                                        uint8_t *other_element_level, element_type other_element_selected, uint16_t other_id, uint8_t paired);

void load_conf();
void write_conf();
void config_init();
uint8_t badge_seen(uint16_t id);
uint8_t badge_connected(uint16_t id);
void set_badge_seen(uint16_t id, char *name);
uint8_t set_badge_connected(uint16_t id, char *handle);
void write_anim_curr();
void save_anim(char *name);
void load_anim_abs(char *pathname);
void load_anim(char *name);
void process_seconds();
void radar_init();

#endif /* BADGE_H_ */
