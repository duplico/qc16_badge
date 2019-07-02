/*
 * storage.h
 *
 *  Created on: Jun 17, 2019
 *      Author: george
 */

#ifndef QUEERCON_DRIVERS_STORAGE_H_
#define QUEERCON_DRIVERS_STORAGE_H_

#include <third_party/spiffs/spiffs.h>

void storage_init();
void storage_read_file(char *fname, uint8_t *dest, uint16_t size);
void storage_overwrite_file(char *fname, uint8_t *src, uint16_t size);

extern spiffs fs;

#endif /* QUEERCON_DRIVERS_STORAGE_H_ */
