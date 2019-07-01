/*
 * badge.c
 *
 *  Created on: Jun 29, 2019
 *      Author: george
 */
#include <stdint.h>

#include <qc16.h>

#include "badge.h"

// In this file lives all the state and stuff related to the game.
// Most of this is persistent.

qbadge_conf_t my_conf;

volatile uint32_t qc_clock;

uint32_t qbadges_near[21];
uint32_t qbadges_seen[21];
uint32_t qbadges_connected[21];
uint32_t cbadges_connected[47];

// TODO: Persistence for:
//  Current LED animation
//  Current photo
//  Badges seen
//  Badges connected
//  Handle?

// Maybe have a /conf/main
//              /conf/backup
