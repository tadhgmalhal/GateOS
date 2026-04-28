#ifndef DISK_CACHE_H
#define DISK_CACHE_H

#include <stdint.h>
#include "ata.h"

#define CACHE_SIZE    64        // number of cached sectors
#define CACHE_INVALID 0xFFFFFFFF

typedef struct
{
    uint32_t lba;               // which sector this entry holds
    uint8_t  data[ATA_SECTOR_SIZE]; // the sector data
    uint8_t  dirty;             // 1 = written but not flushed to disk
    uint32_t last_used;         // LRU timestamp
} cache_entry_t;

void         disk_cache_init();
ata_result_t disk_cache_read(uint32_t lba, void *buffer);
ata_result_t disk_cache_write(uint32_t lba, const void *buffer);
void         disk_cache_flush();
uint32_t     disk_cache_get_hits();
uint32_t     disk_cache_get_misses();

#endif