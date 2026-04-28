#ifndef ATA_H
#define ATA_H

#include <stdint.h>

// ATA I/O ports (primary bus)
#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_ERROR        0x1F1
#define ATA_PRIMARY_SECTOR_COUNT 0x1F2
#define ATA_PRIMARY_LBA_LOW      0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HIGH     0x1F5
#define ATA_PRIMARY_DRIVE        0x1F6
#define ATA_PRIMARY_STATUS       0x1F7
#define ATA_PRIMARY_COMMAND      0x1F7

// ATA status bits
#define ATA_STATUS_ERR  0x01  // error occurred
#define ATA_STATUS_DRQ  0x08  // data request ready
#define ATA_STATUS_SRV  0x10  // overlapped mode service request
#define ATA_STATUS_DF   0x20  // drive fault
#define ATA_STATUS_RDY  0x40  // drive ready
#define ATA_STATUS_BSY  0x80  // drive busy

// ATA commands
#define ATA_CMD_READ_PIO  0x20
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_IDENTIFY  0xEC

// drive select
#define ATA_MASTER 0xE0
#define ATA_SLAVE  0xF0

// sector size
#define ATA_SECTOR_SIZE 512

typedef enum
{
    ATA_OK,
    ATA_ERROR,
    ATA_NO_DRIVE,
    ATA_TIMEOUT
} ata_result_t;

void         ata_init();
ata_result_t ata_read_sectors(uint32_t lba, uint8_t count, void *buffer);
ata_result_t ata_write_sectors(uint32_t lba, uint8_t count, const void *buffer);

#endif