#ifndef SD_H
#define SD_H

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <string.h>
#include "utils.h"

#define SDHC_SDXC_BLOCK_SIZE 512
#define SDSC_BLOCK_SIZE 256

#define READ_START_TOKEN 0xFE
#define READ_TOKEN_OK 0xFE
#define READ_TOKEN_TIMEOUT_ERROR 0xFF // 0x0X exists for data errors as well

typedef struct
{
    volatile uint16_t reserved : 15;
    volatile uint8_t v27_28 : 1;
    volatile uint8_t v28_29 : 1;
    volatile uint8_t v29_30 : 1;
    volatile uint8_t v30_31 : 1;
    volatile uint8_t v31_32 : 1;
    volatile uint8_t v32_33 : 1;
    volatile uint8_t v33_34 : 1;
    volatile uint8_t v34_35 : 1;
    volatile uint8_t v35_36 : 1;
    volatile uint8_t s18a : 1; // Switching to 1.8V accepted
    volatile uint8_t reserved_2 : 4;
    volatile uint8_t card_status : 1;
    volatile uint8_t card_capacity_status : 1;
    volatile uint8_t card_power_up_status_bit : 1; // Busy
} CMD58_OCR;

///////// General Initialization /////////

// General initialization function
esp_err_t sd_init();

esp_err_t sd_init_card(void);

///////// SD Communication /////////

// Sends an SD SPI commabdm the whole 48 bits
esp_err_t sd_send_command(uint8_t cmd, uint32_t arg);

/**
 * Read a byte from SPI.
 * Returns status of operation.
 * Most of the time better to use `sd_read_bytes`.
 */
esp_err_t sd_read_byte(uint8_t *response);

/**
 * Tries to read X bytes into the supplied buffer.
 * Will retry for a valid byte.
 * Returns status of operation.
 */
esp_err_t sd_read_bytes(uint8_t *target, uint32_t count);

///////// SD Response Processing /////////

/**
 * Extracts the 32 trailing data bits from an R3/R7 response.
 */
uint32_t get_ocr(uint8_t *response);

bool sd_is_valid_voltage_cmd58(uint32_t ocr);

/**
 * // XXX might want to create an enum of all posibilities, for now only interested in idle state
 * Returns true if R1 suggests device is in idle state.
 *
 * R1 is 8 bytes:
 * 0 - idle state
 * 1 - erease reset
 * 2 - illegal
 * 3 - CRC error
 * 4 - erease seq error
 * 5 - address error
 * 6 - param error
 * 7 - not used
 */
bool sd_is_idle_state(uint8_t *response);

///////// SD Flow /////////

/**
 * Get the SD card into a working, non-idle state
 */
bool sd_ready_card();

esp_err_t sd_read_block(uint32_t block_id, uint8_t *destination);

#endif