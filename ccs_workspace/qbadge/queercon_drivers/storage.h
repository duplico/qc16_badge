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

extern spiffs fs;

#endif /* QUEERCON_DRIVERS_STORAGE_H_ */
