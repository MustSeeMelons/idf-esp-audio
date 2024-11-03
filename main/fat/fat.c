#include "fat.h"

#include <wchar.h>
#include <locale.h>

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

void utf16_to_utf8(uint8_t *utf16_array, size_t utf16_len, uint8_t *utf8_array)
{
    size_t idx = 0;

    for (size_t i = 0; i < utf16_len; i += 2)
    {
        // 0xffff is padding
        if ((utf16_array[i] == 0xff && utf16_array[i + 1] == 0xff) || (utf16_array[i] == 0 && utf16_array[i + 1] == 0))
        {
            break;
        }

        // Combine two uint8s into a single uint16 character
        uint16_t utf16_char = (utf16_array[i + 1] << 8) | utf16_array[i];

        // Convert the uint16 character to UTF-8
        if (utf16_char <= 0x7F)
        {
            // 1-byte UTF-8
            utf8_array[idx++] = (uint8_t)utf16_char;
        }
        else if (utf16_char <= 0x7FF)
        {
            // 2-byte UTF-8
            utf8_array[idx++] = 0xC0 | (utf16_char >> 6);
            utf8_array[idx++] = 0x80 | (utf16_char & 0x3F);
        }
        else
        {
            // 3-byte UTF-8
            utf8_array[idx++] = 0xE0 | (utf16_char >> 12);
            utf8_array[idx++] = 0x80 | ((utf16_char >> 6) & 0x3F);
            utf8_array[idx++] = 0x80 | (utf16_char & 0x3F);
        }
    }

    utf8_array[idx] = '\0';
}

void log_attributes(FAT_Directory_Entry *entry)
{

    if (entry->DIR_Attr & READ_ONLY)
    {
        ESP_LOGI(TAG, "READ_ONLY");
    }

    if (entry->DIR_Attr & HIDDEN)
    {
        ESP_LOGI(TAG, "HIDDEN");
    }

    if (entry->DIR_Attr & SYSTEM)
    {
        ESP_LOGI(TAG, "SYSTEM");
    }

    if (entry->DIR_Attr & VOLUME_ID)
    {
        ESP_LOGI(TAG, "VOLUME_ID");
    }

    if (entry->DIR_Attr & DIRECTORY)
    {
        ESP_LOGI(TAG, "DIRECTORY");
    }

    if (entry->DIR_Attr & ARCHIVE)
    {
        ESP_LOGI(TAG, "ARCHIVE");
    }

    if (entry->DIR_Attr & LONG_NAME)
    {
        ESP_LOGI(TAG, "LONG_NAME");
    }
}

esp_err_t fat_init()
{
    // Read the MBR
    esp_err_t err = fat_read_bytes(working_block, SDHC_SDXC_BLOCK_SIZE, 0);

    // ESP_LOGI(TAG, "MBR, Sector 0:");
    // debug_512_block(working_block);

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
    ESP_LOGI(TAG, "Partition 1 Boot Sector LBA Begin: %" PRIu32 "", p1_lba);

    // Read the partitions boot sector/volume id
    fat_read_bytes(working_block, SDHC_SDXC_BLOCK_SIZE, p1_lba);

    // ESP_LOGI(TAG, "Boot Sector");
    // debug_512_block(working_block);

    // Read all the juicy details
    uint16_t byter_per_sector = extract_uint16_le(working_block, FAT_BOOT_SECTOR_BYTES_PER_SECTOR);
    sectors_per_cluster = extract_uint8_le(working_block, FAT_BOOT_SECTORS_PER_CLUSTER);
    uint16_t reserved_sectors = extract_uint16_le(working_block, FAT_BOOT_RESERVED_SECTORS);
    uint16_t num_fats = extract_uint8_le(working_block, FAT_BOOT_NUM_FATS);
    uint16_t sectors_per_fat = extract_uint32_le(working_block, FAT_BOOT_SECTORS_PER_FAT);
    root_cluster = extract_uint32_le(working_block, FAT_BOOT_ROOT_CLUSTER);
    uint16_t signature = extract_uint16_le(working_block, FAT_BOOT_SIGNATURE);

    ESP_LOGI(TAG, "byter_per_sector: %d", (unsigned int)byter_per_sector);
    ESP_LOGI(TAG, "sectors_per_cluster: %d", (unsigned int)sectors_per_cluster);
    ESP_LOGI(TAG, "reserved_sectors: %d", (unsigned int)reserved_sectors);
    ESP_LOGI(TAG, "num_fats: %d", (unsigned int)num_fats);
    ESP_LOGI(TAG, "sectors_per_fat: %d", (unsigned int)sectors_per_fat);
    ESP_LOGI(TAG, "root_cluster: %d", (unsigned int)root_cluster);

    // Sanity check, must always match
    if (signature != FAT_BOOT_SIGNATURE_VALUE)
    {
        return ESP_FAIL;
    }

    // Calculate a few necessities
    fat_begin_lba = p1_lba + reserved_sectors;                                    // Where the first FAT is
    cluster_begin_lba = p1_lba + reserved_sectors + (num_fats * sectors_per_fat); // Where the first cluster is

    ESP_LOGI(TAG, "fat_begin_lba: %d", (unsigned int)fat_begin_lba);
    ESP_LOGI(TAG, "cluster_begin_lba: %d", (unsigned int)cluster_begin_lba);

    // TODO use this to read the whole cluster?
    // uint32_t cluster_size_bytes = sectors_per_cluster * byter_per_sector;

    uint32_t offset = 0;

    // XXX Clusters hold data or directory entries! We must parse directory entries first

    // A file has a short & n long directory entries
    // A directory is a file
    uint32_t cluster_sector = get_cluster_lba(root_cluster) + offset;

    fat_read_bytes(working_block, SDHC_SDXC_BLOCK_SIZE, cluster_sector);

    // ESP_LOGI(TAG, "cluster_sector: %d", (unsigned int)cluster_sector);
    // debug_512_block(working_block);

    bool is_loop = true;
    while (is_loop)
    {
        // Read a sector
        uint32_t cluster_sector = get_cluster_lba(root_cluster) + offset;

        err = fat_read_bytes(working_block, SDHC_SDXC_BLOCK_SIZE, cluster_sector);

        for (uint8_t i = 0; i < SDHC_SDXC_BLOCK_SIZE / FAT_CLUSTER_ENTRY_LENGTH; i++)
        {
            uint8_t dir[FAT_CLUSTER_ENTRY_LENGTH];
            memcpy(dir, &working_block[i * FAT_CLUSTER_ENTRY_LENGTH], sizeof(uint8_t) * FAT_CLUSTER_ENTRY_LENGTH);

            FAT_Directory_Entry *entry = (FAT_Directory_Entry *)&dir;

            if (entry->DIR_Name[0] == FAT_DIRECTORY_ALL_FREE)
            {
                is_loop = false;
                break;
            }

            // XXX ONLY FOR A DIRECTORY
            // Not a valid file
            // if (entry->DIR_Name[0] == FAT_DIRECTORY_EMPTY)
            // {
            //     break;
            // }

            ESP_LOGI(TAG, "\n\nEntry Index: %d", (unsigned int)i);

            log_attributes(entry);

            // Take 4 LSB's and check they all are set
            if ((entry->DIR_Attr & LONG_NAME) == LONG_NAME)
            {
                FAT_Directory_Long_Entry *long_entry = (FAT_Directory_Long_Entry *)&dir;

                uint8_t name1_utf8[16];
                uint8_t name2_utf8[19];
                uint8_t name3_utf8[7];

                utf16_to_utf8(long_entry->LDIR_Name1, 10, name1_utf8);
                utf16_to_utf8(long_entry->LDIR_Name2, 12, name2_utf8);
                utf16_to_utf8(long_entry->LDIR_Name3, 4, name3_utf8);

                name1_utf8[15] = '\0';
                name2_utf8[18] = '\0';
                name3_utf8[6] = '\0';

                uint8_t index = 0;
                uint8_t full_name[52];

                for (size_t i = 0; i < 16 && name1_utf8[i] != '\0'; i++)
                {
                    full_name[index++] = name1_utf8[i];
                }
                for (size_t i = 0; i < 19 && name2_utf8[i] != '\0'; i++)
                {
                    full_name[index++] = name2_utf8[i];
                }
                for (size_t i = 0; i < 7 && name3_utf8[i] != '\0'; i++)
                {
                    full_name[index++] = name3_utf8[i];
                }

                full_name[index] = '\0';

                ESP_LOGI(TAG, "%s", (char *)full_name);
            }
            else
            {
                char short_name[12];
                memcpy(short_name, entry->DIR_Name, 11);

                uint8_t idx = 0;
                for (size_t i = 0; i < 11; i++)
                {
                    if (entry->DIR_Name[i] != 0x20)
                    {
                        short_name[idx++] = entry->DIR_Name[i];
                    }
                }

                short_name[idx] = '\0';

                ESP_LOGI(TAG, "Filename: %s", short_name);
                ESP_LOGI(TAG, "Filesize: %u bytes", (unsigned int)entry->DIR_FileSize);
            }
        }

        break;

        // offset += SDHC_SDXC_BLOCK_SIZE;

        // // Debugging purposes
        // if (offset == SDHC_SDXC_BLOCK_SIZE * sectors_per_cluster)
        // {
        //     break;
        // }
    }

    return ESP_OK;
}