/*
 * storage.c
 *
 *  Created on: Jun 17, 2019
 *      Author: george
 */


#include <third_party/spiffs/SPIFFSNVS.h>
#include <third_party/spiffs/spiffs.h>

#include "board.h"
#include "storage.h"

#include "qbadge.h"

uint8_t spiffsWorkBuffer[SPIFFS_LOGICAL_PAGE_SIZE * 2];
uint8_t spiffsFileDescriptorCache[SPIFFS_FILE_DESCRIPTOR_SIZE * 4];
uint8_t spiffsReadWriteCache[SPIFFS_LOGICAL_PAGE_SIZE * 2];
spiffs           fs;
spiffs_config    fsConfig;
SPIFFSNVS_Data   spiffsnvs;

void storage_init() {
    volatile int32_t status;
    status = SPIFFSNVS_config(&spiffsnvs, QC16_NVSSPI25X0, &fs, &fsConfig,
                              SPIFFS_LOGICAL_BLOCK_SIZE, SPIFFS_LOGICAL_PAGE_SIZE);
    if (status != SPIFFSNVS_STATUS_SUCCESS) {
        while (1); // Spin forever.
    }
    status = SPIFFS_mount(&fs, &fsConfig, spiffsWorkBuffer,
        spiffsFileDescriptorCache, sizeof(spiffsFileDescriptorCache),
        spiffsReadWriteCache, sizeof(spiffsReadWriteCache), NULL);

    if (status == SPIFFS_ERR_NOT_A_FS) {
        // Needs to be formatted before mounting.
        status = SPIFFS_format(&fs);
        status = SPIFFS_mount(&fs, &fsConfig, spiffsWorkBuffer,
            spiffsFileDescriptorCache, sizeof(spiffsFileDescriptorCache),
            spiffsReadWriteCache, sizeof(spiffsReadWriteCache), NULL);
    }

    spiffs_stat stat;
    status = SPIFFS_stat(&fs, "testfile", &stat);

    if (status == SPIFFS_OK) {
        // successfully statted the testfile, need to delete.
        SPIFFS_remove(&fs, "testfile");
    }
}
