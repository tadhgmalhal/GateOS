#include "disk_cache.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"

static cache_entry_t cache[CACHE_SIZE];
static uint32_t      cache_time  = 0;
static uint32_t      cache_hits  = 0;
static uint32_t      cache_misses = 0;

void disk_cache_init()
{
    for (int i = 0; i < CACHE_SIZE; i++)
    {
        cache[i].lba       = CACHE_INVALID;
        cache[i].dirty     = 0;
        cache[i].last_used = 0;
    }
    cache_time   = 0;
    cache_hits   = 0;
    cache_misses = 0;

    kprintf("Disk cache initialized.\n");
    kprintf("  slots: %d\n", CACHE_SIZE);
    kprintf("  size:  %d KB\n", (CACHE_SIZE * ATA_SECTOR_SIZE) / 1024);
}

static int cache_find(uint32_t lba)
{
    for (int i = 0; i < CACHE_SIZE; i++)
    {
        if (cache[i].lba == lba)
        {
            return i;
        }
    }
    return -1;
}

static int cache_find_lru()
{
    int      lru_index = 0;
    uint32_t lru_time  = cache[0].last_used;

    for (int i = 1; i < CACHE_SIZE; i++)
    {
        // prefer empty slots
        if (cache[i].lba == CACHE_INVALID)
        {
            return i;
        }

        if (cache[i].last_used < lru_time)
        {
            lru_time  = cache[i].last_used;
            lru_index = i;
        }
    }

    return lru_index;
}

ata_result_t disk_cache_read(uint32_t lba, void *buffer)
{
    cache_time++;

    // check if sector is in cache
    int index = cache_find(lba);

    if (index >= 0)
    {
        // cache hit
        cache_hits++;
        cache[index].last_used = cache_time;
        memcpy(buffer, cache[index].data, ATA_SECTOR_SIZE);
        return ATA_OK;
    }

    // cache miss — read from disk
    cache_misses++;

    int slot = cache_find_lru();

    // if slot is dirty flush it to disk first
    if (cache[slot].dirty && cache[slot].lba != CACHE_INVALID)
    {
        ata_result_t result = ata_write_sectors(cache[slot].lba, 1, cache[slot].data);
        if (result != ATA_OK)
        {
            return result;
        }
        cache[slot].dirty = 0;
    }

    // load new sector into slot
    ata_result_t result = ata_read_sectors(lba, 1, cache[slot].data);
    if (result != ATA_OK)
    {
        return result;
    }

    cache[slot].lba       = lba;
    cache[slot].dirty     = 0;
    cache[slot].last_used = cache_time;

    memcpy(buffer, cache[slot].data, ATA_SECTOR_SIZE);
    return ATA_OK;
}

ata_result_t disk_cache_write(uint32_t lba, const void *buffer)
{
    cache_time++;

    int index = cache_find(lba);

    if (index < 0)
    {
        // not in cache — find a slot
        index = cache_find_lru();

        // flush dirty slot if needed
        if (cache[index].dirty && cache[index].lba != CACHE_INVALID)
        {
            ata_result_t result = ata_write_sectors(cache[index].lba, 1,
                                                     cache[index].data);
            if (result != ATA_OK)
            {
                return result;
            }
            cache[index].dirty = 0;
        }

        cache[index].lba = lba;
    }

    // write into cache
    memcpy(cache[index].data, buffer, ATA_SECTOR_SIZE);
    cache[index].dirty     = 1;
    cache[index].last_used = cache_time;

    return ATA_OK;
}

void disk_cache_flush()
{
    for (int i = 0; i < CACHE_SIZE; i++)
    {
        if (cache[i].dirty && cache[i].lba != CACHE_INVALID)
        {
            ata_write_sectors(cache[i].lba, 1, cache[i].data);
            cache[i].dirty = 0;
        }
    }
}

uint32_t disk_cache_get_hits()
{
    return cache_hits;
}

uint32_t disk_cache_get_misses()
{
    return cache_misses;
}