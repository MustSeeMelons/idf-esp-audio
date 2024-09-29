#ifndef FAT_H
#define FAT_H

#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <inttypes.h> // for printf's
#include <stdbool.h>

#include "utils.h"
#include "sd/sd.h"

// MBR
#define FAT_BOOT_CODE_LEN 446
#define FAT_PARTITION_LEN 16

// MBR Partition
#define FAT_PARTITION_TYPE_INDEX 4
#define FAT_PARTITION_LBA_START_INDEX 8

// Partition boot sector
#define FAT_BOOT_SECTOR_BYTES_PER_SECTOR 0x0B // 2 bytes
#define FAT_BOOT_SECTORS_PER_CLUSTER 0x0D     // 1 byte
#define FAT_BOOT_RESERVED_SECTORS 0x0E        // 2 bytes
#define FAT_BOOT_NUM_FATS 0x10                // 1 bytes
#define FAT_BOOT_SECTORS_PER_FAT 0x24         // 4 bytes
#define FAT_BOOT_ROOT_CLUSTER 0x2C            // 4 bytes
#define FAT_BOOT_SIGNATURE 0x1FE              // 2 bytes

#define FAT_BOOT_SIGNATURE_VALUE 0xAA55

/**
 * FAT is little endian - LSB is stored first.
 * LBA - Logical Block Addressing.
 * FAT - File Allocation Table.
 */

/**
 * Initialize the FAT for reading files
 */
esp_err_t fat_init();

/**
 * Copies partition data into the destination from the source
 */
void get_partition_data(uint8_t *source, uint8_t *destination, uint8_t partition);

uint32_t get_partition_lba(uint8_t *partition_data);

uint32_t get_partition_sector_count(uint8_t *partition_data);

#endif