/*
 * qc16.h
 *
 *  Created on: Jun 17, 2019
 *      Author: george
 */

#ifndef QC16_H_
#define QC16_H_

typedef struct {
    uint16_t badge_id;
    uint8_t initialized;
    uint8_t element_level[3];
    uint8_t element_level_progress[3];
    uint16_t element_qty[3];
} qbadge_conf_t;


typedef struct {
    uint16_t badge_id;
    uint8_t active;
    uint8_t activated;
    uint8_t initialized;
    uint8_t element_level[3];
    uint8_t element_level_progress[3];
    uint16_t element_qty[3];
} cbadge_conf_t;

#define QBADGE_ID_START 0
#define QBADGE_ID_UNASSIGNED 999 // TODO
#define QBADGES_IN_SYSTEM 650 // TODO
#define CBADGE_ID_START 1000
#define CBADGES_IN_SYSTEM 1500 // TODO
#define CBADGE_ID_UNASSIGNED 9999

#endif /* QC16_H_ */
