/*
 * qc16.h
 *
 *  Created on: Jun 17, 2019
 *      Author: george
 */

#ifndef QC16_H_
#define QC16_H_

#define QC16_BADGE_NAME_LEN 9

typedef enum {
    ELEMENT_LOCKS = 0,
    ELEMENT_COINS,
    ELEMENT_CAMERAS,
    ELEMENT_KEYS,
    ELEMENT_COCKTAILS,
    ELEMENT_FLAGS,
    ELEMENT_NONE,
    ELEMENT_COUNT,
} element_type;

typedef struct {
    uint16_t badge_id;
    uint8_t initialized;
    uint8_t element_level[3];
    uint8_t element_level_progress[3];
    uint16_t element_qty[3];
    uint16_t qbadges_seen_count;
    uint16_t qbadges_connected_count;
    uint16_t cbadges_connected_count;
    uint32_t last_clock;
    uint8_t agent_present;
    uint32_t agent_return_time;
    element_type element_selected;
    char handle[QC16_BADGE_NAME_LEN + 1];
} qbadge_conf_t;

typedef struct {
    uint16_t badge_id;
    uint8_t active;
    uint8_t activated;
    uint8_t initialized;
    uint8_t element_level[3];
    uint8_t element_level_progress[3];
    uint16_t element_qty[3];
    char handle[QC16_BADGE_NAME_LEN + 1];
} cbadge_conf_t;

#define QBADGE_ID_START 0
#define QBADGE_ID_UNASSIGNED 999 // TODO
#define QBADGES_IN_SYSTEM 650 // TODO
#define CBADGE_ID_START 1000
#define CBADGES_IN_SYSTEM 1500 // TODO
#define CBADGE_ID_UNASSIGNED 9999

#define BADGE_TYPE_INVALID 0

#define BADGE_TYPE_CBADGE_NORMAL 101
#define BADGE_TYPE_CBADGE_HANDLER 102
#define BADGE_TYPE_CBADGE_UBER 103
#define BADGE_TYPE_CBADGE_SEQUEERITY 104

#define BADGE_TYPE_QBADGE_NORMAL 201
#define BADGE_TYPE_QBADGE_UBER 202
#define BADGE_TYPE_QBADGE_HANDLER 203

#define BADGE_TYPE_BRIDGE 301
#define BADGE_TYPE_PILLAR 302

#define BADGE_TYPE_CONTROLLER 401

typedef struct {
    // TODO: name?
    element_type element_types[2];
    uint8_t element_levels[2];
    uint8_t element_rewards[2];
    uint8_t element_progress[2];
} mission_t;

#endif /* QC16_H_ */
