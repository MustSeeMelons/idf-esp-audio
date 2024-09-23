#include "sd.h"

#include "esp_log.h"

#define SD_CS 5
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCK 18

// TODO CMD55
// TODO CMD41
// TODO CMD58

#define CMD_0_ID 0
#define CMD_8_ID 8
#define CMD_58_ID 58

#define CMD_0_BODY 0
#define CMD_8_BODY 0x1AA
#define CMD_58_BODY 0

static const char *TAG = "SD";
static spi_device_handle_t spi;

bool sd_is_r1_ok(uint8_t *response)
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

bool sd_is_valid_voltage(uint32_t ocr)
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

esp_err_t sd_recieve_response(uint8_t *response)
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

    sd_recieve_response(&byte);

    // Assume slave not ready if first byte is 0xff
    while (byte == 0xff)
    {
        sd_recieve_response(&byte);
    }

    target[0] = byte;

    // Read the rest
    for (size_t i = 1; i < count; i++)
    {
        sd_recieve_response(&byte);
        target[i] = byte;
    }
}

void sd_init_card(void)
{
    send_dummy_clocks();

    // Send CMD0 to reset the card
    uint8_t response = 0xff;
    sd_send_command(CMD_0_ID, CMD_0_BODY);

    uint8_t retries = 3;
    response = 0xff;
    while (response != 0x01)
    {
        sd_recieve_response(&response);

        retries--;

        if (retries == 0)
        {
            ESP_LOGE(TAG, "Failed to reset card!");
            return;
        }
    }

    ESP_LOGI(TAG, "Card is in idle state.");

    // Send CMD8 to check voltage range
    uint8_t cmd8_response[5] = {};

    sd_send_command(CMD_8_ID, CMD_8_BODY);

    sd_read_bytes(cmd8_response, 5);

    // Validate CMD8: R1 idle, pattern matches & voltage 2.7-3.6 (0x0001 in 16:19 positions)
    if (sd_is_r1_ok(cmd8_response) && cmd8_response[4] != 0xaa && cmd8_response[3] == 0x01)
    {
        ESP_LOGI(TAG, "CMD8 failed!");

        sd_send_command(CMD_58_ID, CMD_58_BODY);

        uint8_t cmd58_response[5] = {};

        sd_read_bytes(cmd58_response, 5);

        if (cmd58_response[0] != 0x01)
        {
            ESP_LOGI(TAG, "CMD58 failed!");
        } else {
            // TODO validate voltage

            // TODO CMD55
            // TODO ACMD41
        }

        return;
    }
    else
    {
        // CMD8 is valid! SD Spec V2!
        ESP_LOGI(TAG, "CMD8 successful.");

        // TODO CMD55
        // TODO ACMD41

        // TODO CMD58

        sd_send_command(CMD_58_ID, CMD_58_BODY);

        uint8_t cmd58_response[5] = {};

        sd_read_bytes(cmd58_response, 5);

        uint32_t ocr = get_ocr(cmd58_response);

        if (sd_is_valid_voltage(ocr))
        {
            ESP_LOGI(TAG, "Card voltage OK");
        }
    }
}

void send_dummy_clocks(void)
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

// TODO rename, this is SPI bus configuration for the SD card
void sd_init()
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

    sd_init_card();
}