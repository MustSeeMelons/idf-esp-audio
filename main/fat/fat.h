#ifndef FAT_H
#define FAT_H

#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <inttypes.h> // for printf's
#include <stdbool.h>

#include "utils.h"
#include "sd/sd.h"

// FAT12/16/32 is determined by sector count only

// BPB - BIOS Parameter Block, first sector
// BS - Boot Sector
// FAT itself - list of clusters

// BPB_RootClus

#define FAT_EndOfCluster 0x0FFFFFFF
#define FAT_BadCluster 0x0FFFFFF7

// DIR_Name[0] special case when all dirs after this one are free
#define FAT_DIRECTORY_ALL_FREE 0x00

// DIR_Name[0] special case when directory entry is free
#define FAT_DIRECTORY_EMPTY 0xE5

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

// Short name dir entry/long file name entry for high capacity cards
#define FAT_CLUSTER_ENTRY_LENGTH 32

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

// These are to be used as bit flags
typedef enum
{
    READ_ONLY = 0x01,
    HIDDEN = 0x02,
    SYSTEM = 0x04,
    VOLUME_ID = 0x08,
    DIRECTORY = 0x10,
    ARCHIVE = 0x20,
    LONG_NAME = 0x0F
} FAT_Directory_Attr;

typedef struct
{
    uint8_t DIR_Name[11]; // File shortname
    uint8_t DIR_Attr;     // Atrtibutes
    uint8_t DIR_NTRes;    // Windows NT, always 0
    uint8_t DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI; // First cluster number 2x HIGH byte
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO; // First cluster number 2x LOW byte
    uint32_t DIR_FileSize;
} FAT_Directory_Entry;

typedef struct
{
    uint8_t LDIR_Ord;
    uint8_t LDIR_Name1[10];
    uint8_t LDIR_Attr; // Must be LONG_NAME
    uint8_t LDIR_Type;
    uint8_t LDIR_Chksum;
    uint8_t LDIR_Name2[12];
    uint16_t LDIR_FstClusLO; // Must be ZERO, compatability field
    uint8_t LDIR_Name3[4];
} FAT_Directory_Long_Entry;

#endif