#include "ata.h"
#include "../lib/kprintf.h"
#include "../lib/string.h"

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %1, %0" : : "dN"(port), "a"(value));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t value)
{
    __asm__ volatile("outw %1, %0" : : "dN"(port), "a"(value));
}

static void ata_io_wait()
{
    // reading status port 4 times gives the drive 400ns to update
    inb(ATA_PRIMARY_STATUS);
    inb(ATA_PRIMARY_STATUS);
    inb(ATA_PRIMARY_STATUS);
    inb(ATA_PRIMARY_STATUS);
}

static ata_result_t ata_wait_ready()
{
    uint32_t timeout = 100000;
    while (timeout--)
    {
        uint8_t status = inb(ATA_PRIMARY_STATUS);
        if (status & ATA_STATUS_ERR)
        {
            return ATA_ERROR;
        }
        if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_RDY))
        {
            return ATA_OK;
        }
    }
    return ATA_TIMEOUT;
}

static ata_result_t ata_wait_drq()
{
    uint32_t timeout = 100000;
    while (timeout--)
    {
        uint8_t status = inb(ATA_PRIMARY_STATUS);
        if (status & ATA_STATUS_ERR)
        {
            return ATA_ERROR;
        }
        if (status & ATA_STATUS_DRQ)
        {
            return ATA_OK;
        }
    }
    return ATA_TIMEOUT;
}

void ata_init()
{
    // select master drive
    outb(ATA_PRIMARY_DRIVE, ATA_MASTER);
    ata_io_wait();

    // send IDENTIFY command
    outb(ATA_PRIMARY_SECTOR_COUNT, 0);
    outb(ATA_PRIMARY_LBA_LOW,      0);
    outb(ATA_PRIMARY_LBA_MID,      0);
    outb(ATA_PRIMARY_LBA_HIGH,     0);
    outb(ATA_PRIMARY_COMMAND,      ATA_CMD_IDENTIFY);
    ata_io_wait();

    uint8_t status = inb(ATA_PRIMARY_STATUS);
    if (status == 0)
    {
        kprintf("ATA: no drive detected\n");
        return;
    }

    // wait for BSY to clear
    ata_result_t result = ata_wait_drq();
    if (result != ATA_OK)
    {
        kprintf("ATA: drive did not respond to IDENTIFY\n");
        return;
    }

    // read 256 words of identify data
    uint16_t identify[256];
    for (int i = 0; i < 256; i++)
    {
        identify[i] = inw(ATA_PRIMARY_DATA);
    }

    // extract drive size from identify data (words 60-61)
    uint32_t sectors = ((uint32_t)identify[61] << 16) | identify[60];

    kprintf("ATA: drive detected.\n");
    kprintf("  sectors: %u\n", sectors);
    kprintf("  size:    %u MB\n", (sectors / 2) / 1024);
}

ata_result_t ata_read_sectors(uint32_t lba, uint8_t count, void *buffer)
{
    ata_result_t result = ata_wait_ready();
    if (result != ATA_OK)
    {
        return result;
    }

    // send LBA and sector count
    outb(ATA_PRIMARY_DRIVE,        ATA_MASTER | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECTOR_COUNT, count);
    outb(ATA_PRIMARY_LBA_LOW,      (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_LBA_MID,      (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_LBA_HIGH,     (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_PRIMARY_COMMAND,      ATA_CMD_READ_PIO);

    uint16_t *buf = (uint16_t *)buffer;

    for (uint8_t s = 0; s < count; s++)
    {
        result = ata_wait_drq();
        if (result != ATA_OK)
        {
            return result;
        }

        // read 256 16-bit words = 512 bytes
        for (int i = 0; i < 256; i++)
        {
            buf[s * 256 + i] = inw(ATA_PRIMARY_DATA);
        }

        ata_io_wait();
    }

    return ATA_OK;
}

ata_result_t ata_write_sectors(uint32_t lba, uint8_t count, const void *buffer)
{
    ata_result_t result = ata_wait_ready();
    if (result != ATA_OK)
    {
        return result;
    }

    // send LBA and sector count
    outb(ATA_PRIMARY_DRIVE,        ATA_MASTER | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECTOR_COUNT, count);
    outb(ATA_PRIMARY_LBA_LOW,      (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_LBA_MID,      (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_LBA_HIGH,     (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_PRIMARY_COMMAND,      ATA_CMD_WRITE_PIO);

    const uint16_t *buf = (const uint16_t *)buffer;

    for (uint8_t s = 0; s < count; s++)
    {
        result = ata_wait_drq();
        if (result != ATA_OK)
        {
            return result;
        }

        // write 256 16-bit words = 512 bytes
        for (int i = 0; i < 256; i++)
        {
            outw(ATA_PRIMARY_DATA, buf[s * 256 + i]);
        }

        ata_io_wait();
    }

    // flush write cache
    outb(ATA_PRIMARY_COMMAND, 0xE7);
    ata_wait_ready();

    return ATA_OK;
}