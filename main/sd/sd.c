#include "sd.h"

#include "esp_log.h"

#define SD_CS 5
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCK 18

#define CMD_0_ID 0
#define CMD_8_ID 8
#define CMD_58_ID 58
#define CMD_55_ID 55
#define CMD_41_ID 41

#define CMD_0_BODY 0x00
#define CMD_8_BODY 0x1AA
#define CMD_58_BODY 0x00
#define CMD_55_BODY 0x00
#define CMD_41_BODY 0x40000000 // HCS to 1, for SDHC/SDXC support

void sd_warmup(void);
esp_err_t sd_spi_init(void);

static const char *TAG = "SD";
static spi_device_handle_t spi;

bool sd_is_idle_state(uint8_t *response)
{
    return response[0] == 0x01;
};

uint32_t get_ocr(uint8_t *response)
{
    return (response[1] << 24) |
           (response[2] << 16) |
           (response[3] << 8) |
           response[4];
}

bool sd_is_valid_voltage_cmd58(uint32_t ocr)
{
    CMD58_OCR *ocr_struct = (CMD58_OCR *)&ocr;

    return ocr_struct->v29_30 || ocr_struct->v30_31 || ocr_struct->v31_32 || ocr_struct->v32_33;
}

esp_err_t sd_send_command(uint8_t cmd, uint32_t arg)
{
    uint8_t command[6];
    // 0x40 is 01 that are the MSB of a command
    command[0] = 0x40 | cmd;
    command[1] = (arg >> 24) & 0xFF;
    command[2] = (arg >> 16) & 0xFF;
    command[3] = (arg >> 8) & 0xFF;
    command[4] = arg & 0xFF;

    // CRC, required only by a few, so better to just hardcode for them
    switch (cmd)
    {
    case 0:
        command[5] = 0x95;
        break;
    case 8:
        command[5] = 0x87;
        break;
    default:
    case 58:
        command[5] = 0xff;
    }

    spi_transaction_t t = {
        .length = 6 * 8,
        .tx_buffer = command,
    };
    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));

    return ESP_OK;
}

esp_err_t sd_read_byte(uint8_t *response)
{
    uint8_t dummy = 0xFF;

    spi_transaction_t r = {
        .length = 8, // these are bits
        .tx_buffer = &dummy,
        .rx_buffer = response,
    };

    ESP_ERROR_CHECK(spi_device_transmit(spi, &r));

    return ESP_OK;
}

void sd_read_bytes(uint8_t *target, uint8_t count)
{
    uint8_t byte = 0x00;
    // XXX we could check if MISO is high as well?
    sd_read_byte(&byte);

    // TODO have max retries & return status
    // Assume slave not ready if first byte is 0xff
    while (byte == 0xff)
    {
        sd_read_byte(&byte);
    }

    target[0] = byte;

    // Read the rest
    for (size_t i = 1; i < count; i++)
    {
        sd_read_byte(&byte);
        target[i] = byte;
    }
}

bool sd_ready_card()
{
    uint8_t retries = 10;
    while (true)
    {
        if (retries <= 0)
        {
            return false;
        }

        // Tell the next command is application specific
        sd_send_command(CMD_55_ID, CMD_55_BODY);
        uint8_t cmd55_response;
        sd_read_bytes(&cmd55_response, 1);

        if (!sd_is_idle_state(&cmd55_response))
        {
            ESP_LOGI(TAG, "CMD55 fail.");
            // Not counting towards fail counter, need both for a valid prompt
            continue;
        }

        // Capacity information & activate initialization process
        sd_send_command(CMD_41_ID, CMD_41_BODY);
        uint8_t acmd41_response;
        sd_read_bytes(&acmd41_response, 1);

        // We must not be in idle state, response should be 0x00
        if (sd_is_idle_state(&acmd41_response))
        {
            ESP_LOGI(TAG, "CMD41 fail.");
            retries--;
            continue;
        }

        break;
    }

    return true;
}

esp_err_t sd_init_card(void)
{
    // Neet a cycle warmp-up period for the card to start to function
    sd_warmup();

    // Send CMD0 to reset the card, try a few times
    sd_send_command(CMD_0_ID, CMD_0_BODY);

    uint8_t retries = 3;
    uint8_t response = 0xff;
    while (response != 0x01)
    {
        sd_read_byte(&response);

        retries--;

        if (retries == 0)
        {
            ESP_LOGE(TAG, "Failed to reset card!");
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "Card is in idle state.");

    // Send CMD8 to check card version and voltage range
    uint8_t cmd8_response[5] = {};
    sd_send_command(CMD_8_ID, CMD_8_BODY);
    sd_read_bytes(cmd8_response, 5);

    // Validate CMD8: R1 idle, pattern matches & voltage 2.7-3.6 (0x0001 in 16:19 positions)
    if (sd_is_idle_state(cmd8_response) && cmd8_response[4] == 0xaa && cmd8_response[3] == 0x01)
    {
        // Might be a SDHC/SDXC card
        ESP_LOGI(TAG, "CMD8 successful.");

        if (!sd_ready_card())
        {
            return ESP_FAIL;
        }

        sd_send_command(CMD_58_ID, CMD_58_BODY);

        uint8_t cmd58_response[5] = {};

        sd_read_bytes(cmd58_response, 5);

        uint32_t ocr = get_ocr(cmd58_response);

        CMD58_OCR *c = (CMD58_OCR *)&ocr;

        // XXX set block size accordingly?
        if (c->card_capacity_status == 1)
        {
            ESP_LOGI(TAG, "High/Extended capacity card.");
        }
        else
        {
            ESP_LOGI(TAG, "Standard capacity card.");
        }
    }
    else
    {
        // XXX This 'should' work, don't have such a card on hand
        // This shall be a standard capacity card (SDSC)
        ESP_LOGI(TAG, "CMD8 failed!");

        sd_send_command(CMD_58_ID, CMD_58_BODY);
        uint8_t cmd58_response[5] = {};
        sd_read_bytes(cmd58_response, 5);

        if (!sd_is_idle_state(cmd58_response))
        {
            ESP_LOGI(TAG, "CMD58 failed!");
            return ESP_FAIL;
        }
        else
        {
            uint32_t ocr = get_ocr(cmd58_response);

            if (sd_is_valid_voltage_cmd58(ocr))
            {
                ESP_LOGI(TAG, "Card voltage OK");
            }

            if (!sd_ready_card())
            {
                return ESP_FAIL;
            }

            return ESP_OK;
        }
    }

    return ESP_OK;
}

/**
 * Before we do anything with the SD card, we must clock it atleast 74 times.
 */
void sd_warmup(void)
{
    uint8_t dummy = 0xFF;
    for (int i = 0; i < 10; i++)
    {
        spi_transaction_t t = {
            .length = 8, // 1 byte
            .tx_buffer = &dummy,
        };
        spi_device_transmit(spi, &t);
    }
}

esp_err_t sd_init()
{
    // Configure the bus
    sd_spi_init();

    // Get the SD card itself into a functional state
    return sd_init_card();
}

esp_err_t sd_spi_init()
{
    gpio_pullup_en(SD_MOSI);
    gpio_pullup_en(SD_MISO);
    gpio_pullup_en(SD_SCK);

    // SPI bus configuration
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI,
        .miso_io_num = SD_MISO,
        .sclk_io_num = SD_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64,
    };

    // Initialize the SPI bus
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    // SPI device configuration
    spi_device_interface_config_t dev_cfg = {
        .mode = 0, // SPI mode 0
        .spics_io_num = SD_CS,
        .clock_speed_hz = 100000, // Initialize at low clock speed (100 kHz)
        .queue_size = 3,
    };

    // Attach the SD card to the SPI bus
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi));

    return ESP_OK;
}