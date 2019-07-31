/*
 * qc16.h
 *
 *  Created on: Jun 17, 2019
 *      Author: george
 */

#ifndef QC16_H_
#define QC16_H_

#define EXP_PER_LEVEL0 1
#define EXP_PER_LEVEL1 2
#define EXP_PER_LEVEL2 4
#define EXP_PER_LEVEL3 8
#define EXP_PER_LEVEL4 16
#define EXP_PER_LEVEL5 0

#define MISSIONS_TO_LEVEL1 2
#define MISSIONS_TO_LEVEL2 4
#define MISSIONS_TO_LEVEL3 8
#define MISSIONS_TO_LEVEL4 8
#define MISSIONS_TO_LEVEL5 8

#define QC16_BADGE_NAME_LEN 12

#define QC16_PHOTO_NAME_LEN 16
#define QC16_COLOR_NAME_LEN 16

#define BITFIELD_BYTES_QBADGE 82
#define BITFIELD_BYTES_CBADGE 200

// Maximum length of a serial payload (the max work buffer length)
#define SERIAL_BUFFER_LEN 128

typedef enum {
    ELEMENT_LOCKS = 0,
    ELEMENT_COINS,
    ELEMENT_CAMERAS,
    ELEMENT_KEYS,
    ELEMENT_COCKTAILS,
    ELEMENT_FLAGS,
    ELEMENT_COUNT_NONE,
} element_type;

typedef struct {
    __packed element_type element_types[2];
    __packed uint8_t element_levels[2];
    __packed uint8_t element_rewards[2];
    __packed uint8_t element_progress[2];
    __packed uint16_t duration_seconds;
} mission_t;

typedef struct {
    uint16_t qbadges_connected_count;
    uint16_t cbadges_connected_count;
    uint32_t missions_run;
    uint32_t element_qty_cumulative[3];
    __packed uint8_t qbadges_uber_connected_count;
    __packed uint8_t qbadges_handler_connected_count;
    __packed uint8_t cbadges_handler_connected_count;
    __packed uint8_t cbadges_uber_connected_count;
    uint16_t qbadges_in_system;
    uint16_t cbadges_in_system;
    uint16_t qbadges_seen_count;
    __packed uint8_t qbadges_uber_seen_count;
    __packed uint8_t qbadges_handler_seen_count;
    __packed uint8_t qbadge_ubers_in_system;
    __packed uint8_t qbadge_handlers_in_system;
    __packed uint8_t cbadge_ubers_in_system;
    __packed uint8_t cbadge_handlers_in_system;
} qbadge_stats_t;

// NB: It's (probably) important that this is a strict subset of the above,
//     and, moreover, that it be the leading subset of the above.
typedef struct {
    uint16_t qbadges_connected_count;
    uint16_t cbadges_connected_count;
} cbadge_stats_t;

typedef struct {
    uint16_t badge_id;
    uint8_t initialized;
    uint8_t badge_type;
    uint8_t element_level[3];
    uint8_t element_level_max[3];
    uint8_t element_level_progress[3];
    uint32_t element_qty[3];
    element_type element_selected;
    uint32_t last_clock;
    uint8_t clock_is_set;
    uint8_t agent_present;
    uint8_t agent_mission_id;
    uint32_t agent_return_time;
    uint8_t vhandler_present;
    uint32_t vhandler_return_time;
    uint8_t handler_allowed;
    uint32_t handler_cooldown_time;
    uint8_t mission_assigned[4];
    mission_t missions[4];
    uint8_t color_types_unlocked;
    uint8_t color_mods_unlocked;
    char current_photo[QC16_PHOTO_NAME_LEN+1];
    char handle[QC16_BADGE_NAME_LEN + 1];
    qbadge_stats_t stats;
} qbadge_conf_t;

typedef struct {
    /// The badge's ID, between CBADGE_ID_START and CBADGE_ID_MAX_UNASSIGNED
    uint16_t badge_id;
    /// Whether the badge has a config created for it.
    uint8_t initialized;
    /// Whether the badge has been assigned an ID or is otherwise "in use"
    uint8_t in_service;
    /// Whether the badge has ever run under its own power:
    uint8_t activated;
    /// Uber or handler, and handler type if relevant.
    uint8_t badge_type;
    /// The currently selected element on this badge
    element_type element_selected;
    /// This badge's element levels, global IDs [2..4]
    uint8_t element_level[3];
    /// Progress toward the next level in each element:
    uint8_t element_level_progress[3];
    /// Quantity of each cbadge element on this badge:
    uint32_t element_qty[3];
    /// This badge's assigned handle.
    char handle[QC16_BADGE_NAME_LEN + 1];
    /// Running statistics:
    cbadge_stats_t stats;
} cbadge_conf_t;

#define CBADGE_QTY_PLUS1    1
#define CBADGE_QTY_PLUS2    50
#define CBADGE_QTY_PLUS3    100
#define CBADGE_QTY_PLUS4    500

#define QBADGE_ID_START 0
#define QBADGE_COUNT_INITIAL 650
#define QBADGE_ID_MAX_UNASSIGNED 999
#define CBADGE_ID_START 1000
#define CBADGE_COUNT_INITIAL 1550
#define CBADGE_ID_MAX_UNASSIGNED 9999

#define CONTROLLER_ID 29635
#define PHONE_ID 22223
#define PILLAR_ID 20001

// TYPE FLAG (BIT7=UBER; BIT6=HANDLER; BIT5-3=unused BIT2-0=ELEMENT)

#define BADGE_TYPE_UBER_MASK 0b10000000
#define BADGE_TYPE_HANDLER_MASK 0b01000000

#define BADGE_TYPE_NORMAL 0x00
#define BADGE_TYPE_UBER BADGE_TYPE_UBER_MASK
#define BADGE_TYPE_UBER_HANDLER (BADGE_TYPE_UBER_MASK | BADGE_TYPE_HANDLER_MASK)
#define BADGE_TYPE_HANDLER BADGE_TYPE_HANDLER_MASK
#define BADGE_TYPE_ELEMENT_MASK 0b111

#endif /* QC16_H_ */
