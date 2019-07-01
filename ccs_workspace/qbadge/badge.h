/*
 * badge.h
 *
 *  Created on: Jun 29, 2019
 *      Author: george
 */

#ifndef BADGE_H_
#define BADGE_H_

extern qbadge_conf_t my_conf;

extern volatile uint32_t qc_clock;

extern uint32_t qbadges_near[21];
extern uint32_t qbadges_seen[21];
extern uint32_t qbadges_connected[21];
extern uint32_t cbadges_connected[47];

#endif /* BADGE_H_ */
