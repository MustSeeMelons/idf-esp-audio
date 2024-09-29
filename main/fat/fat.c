#include "fat.h"

static const char *TAG = "FAT";
static uint8_t working_block[SDHC_SDXC_BLOCK_SIZE] = {0};

static uint32_t fat_begin_lba;
static uint32_t cluster_begin_lba;
static uint32_t sectors_per_cluster;
static uint32_t root_cluster; // Clusters start with 2, there is no 0 or 1 cluster

void get_partition_data(uint8_t *source, uint8_t *destination, uint8_t partition)
{
    uint32_t offset = FAT_BOOT_CODE_LEN;

    switch (partition)
    {
    case 2:
        offset += FAT_PARTITION_LEN;
        break;
    case 3:
        offset += FAT_PARTITION_LEN * 2;
        break;
    case 4:
        offset += FAT_PARTITION_LEN * 3;
        break;
    default:
        break;
    }

    memcpy(destination, &source[offset], sizeof(uint8_t) * FAT_PARTITION_LEN);
}

uint32_t get_partition_lba(uint8_t *partition_data)
{
    return extract_uint32_le(partition_data, FAT_PARTITION_LBA_START_INDEX);
}

uint32_t get_partition_sector_count(uint8_t *partition_data)
{
    return extract_uint32_le(partition_data, FAT_PARTITION_TYPE_INDEX);
}

esp_err_t fat_read_bytes(uint8_t *destination, uint32_t size, uint32_t address)
{
    // Calc start block
    uint32_t start_block = address / SDHC_SDXC_BLOCK_SIZE;

    // Calc how many blocks the data spans
    uint32_t block_count = (size + SDHC_SDXC_BLOCK_SIZE - 1) / SDHC_SDXC_BLOCK_SIZE;

    // If start data is not aligned
    uint32_t offset = address % SDHC_SDXC_BLOCK_SIZE;

    uint32_t block_index = start_block;
    bool is_first = true;
    uint32_t bytes_left = size;
    uint32_t index = 0;

    while (block_count != 0)
    {

        esp_err_t op_status = sd_read_block(block_index, working_block);

        if (op_status != ESP_OK)
        {
            return op_status;
        }

        block_index++;

        // Handle possible offset of the first block
        if (is_first)
        {
            memcpy(destination, &working_block[offset], sizeof(uint8_t) * (SDHC_SDXC_BLOCK_SIZE - offset));
            index += SDHC_SDXC_BLOCK_SIZE - offset;
            bytes_left -= SDHC_SDXC_BLOCK_SIZE - offset;
            is_first = false;
        }
        else if (block_count == 1)
        {
            // Handle the last block that might be smaller
            memcpy(destination, &working_block[index], sizeof(uint8_t) * (bytes_left));
            index += bytes_left;
            bytes_left -= bytes_left;
        }
        else
        {
            // Handle the full blocks
            memcpy(destination, &working_block[index], sizeof(uint8_t) * (SDHC_SDXC_BLOCK_SIZE));
            index += SDHC_SDXC_BLOCK_SIZE;
            bytes_left -= SDHC_SDXC_BLOCK_SIZE;
        }

        block_count--;
    }

    return ESP_OK;
}

static uint32_t get_cluster_lba(uint32_t cluster_num)
{
    // Cluster numbering starts from 2, there is no 0th or 1st ¯\_(ツ)_/¯
    return cluster_begin_lba + (cluster_num - 2) * sectors_per_cluster;
}

esp_err_t fat_init()
{
    // Read the MBR
    esp_err_t err = sd_read_block(0, working_block);

    if (err)
    {
        return err;
    }

    // XXX check the rest of the partitions?
    // From the MBR we assume there is but one partition, must find its boot sector
    uint8_t p1_data[FAT_PARTITION_LEN] = {};
    get_partition_data(working_block, p1_data, 1);

    // Extract LBA address of the partition boot sector
    uint32_t p1_lba = get_partition_lba(p1_data);
    ESP_LOGI(TAG, "Partition 1 boot sector LBA Begin: %" PRIu32 "", p1_lba);

    // Read the partitions boot sector/volume id
    fat_read_bytes(working_block, SDHC_SDXC_BLOCK_SIZE, p1_lba);

    // Read all the juicy details
    uint16_t byter_per_sector = extract_uint16_le(working_block, FAT_BOOT_SECTOR_BYTES_PER_SECTOR);
    sectors_per_cluster = extract_uint8_le(working_block, FAT_BOOT_SECTORS_PER_CLUSTER);
    uint16_t reserved_sectors = extract_uint16_le(working_block, FAT_BOOT_RESERVED_SECTORS);
    uint16_t num_fats = extract_uint8_le(working_block, FAT_BOOT_NUM_FATS);
    uint16_t sectors_per_fat = extract_uint32_le(working_block, FAT_BOOT_SECTORS_PER_FAT);
    root_cluster = extract_uint32_le(working_block, FAT_BOOT_ROOT_CLUSTER);
    uint16_t signature = extract_uint16_le(working_block, FAT_BOOT_SIGNATURE);

    // Sanity check, must always match
    if (signature != FAT_BOOT_SIGNATURE_VALUE)
    {
        return ESP_FAIL;
    }

    // Calculate a few necessities
    fat_begin_lba = p1_lba + reserved_sectors;                                    // Where the first FAT is
    cluster_begin_lba = p1_lba + reserved_sectors + (num_fats * sectors_per_fat); // Where the first cluster is

    // TODO use this to read the whole cluster?
    // uint32_t cluster_size_bytes = sectors_per_cluster * byter_per_sector;

    err = fat_read_bytes(working_block, SDHC_SDXC_BLOCK_SIZE, get_cluster_lba(root_cluster));

    // XXX Clusters hold data or directory entries! We must parse directory entries first

    uint8_t dir[32];
    memcpy(dir, working_block, sizeof(uint8_t) * 32);

    log_uint8_array("Root Dir", dir, 32);

    debug_512_block(working_block);

    return ESP_OK;
}