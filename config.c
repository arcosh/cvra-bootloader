#include <string.h>
#include <cmp_mem_access/cmp_mem_access.h>
#include <crc/crc32.h>
#include "config.h"

static inline uint32_t config_calculate_crc(void *page, size_t page_size)
{
    return crc32(0, (uint8_t *)page + 4, page_size - 4);
}

static inline uint32_t config_read_crc(void *page)
{
    uint32_t crc = 0;
    uint8_t *p = page;
    crc |= p[0] << 24;
    crc |= p[1] << 16;
    crc |= p[2] << 8;
    crc |= p[3] << 0;
    return crc;
}

bool config_is_valid(void *page, size_t page_size)
{
    // Make sure, the given address lies within the flash memory boundaries
    extern uint32_t flash_begin, flash_end;
    if ((uint32_t) page < (uint32_t) (&flash_begin) || (uint32_t) page > (uint32_t) (&flash_end))
        return false;

    return (config_read_crc(page) == config_calculate_crc(page, page_size));
}

void config_write(void *buffer, bootloader_config_t *config, size_t buffer_size)
{
    cmp_ctx_t context;
    cmp_mem_access_t cma;
    uint8_t *p = buffer;

    cmp_mem_access_init(&context, &cma, &p[4], buffer_size - 4);
    config_write_messagepack(&context, config);

    uint32_t crc = config_calculate_crc(buffer, buffer_size);
    p[0] = ((crc >> 24) & 0xff);
    p[1] = ((crc >> 16) & 0xff);
    p[2] = ((crc >> 8) & 0xff);
    p[3] = (crc & 0xff);
}

void config_write_messagepack(cmp_ctx_t *context, bootloader_config_t *config)
{
    cmp_write_map(context, 6);

    cmp_write_str(context, CONFIG_KEY_ID, sizeof(CONFIG_KEY_ID)-1);
    cmp_write_u8(context, config->ID);

    cmp_write_str(context, CONFIG_KEY_BOARD_NAME, sizeof(CONFIG_KEY_BOARD_NAME)-1);
    cmp_write_str(context, config->board_name, strlen(config->board_name));

    cmp_write_str(context, CONFIG_KEY_DEVICE_CLASS, sizeof(CONFIG_KEY_DEVICE_CLASS)-1);
    cmp_write_str(context, config->device_class, strlen(config->device_class));

    cmp_write_str(context, CONFIG_KEY_APPLICATION_CRC, sizeof(CONFIG_KEY_APPLICATION_CRC)-1);
    cmp_write_uint(context, config->application_crc);

    cmp_write_str(context, CONFIG_KEY_APPLICATION_SIZE, sizeof(CONFIG_KEY_APPLICATION_SIZE)-1);
    cmp_write_uint(context, config->application_size);

    cmp_write_str(context, CONFIG_KEY_UPDATE_COUNT, sizeof(CONFIG_KEY_UPDATE_COUNT)-1);
    cmp_write_uint(context, config->update_count);
}

bootloader_config_t config_read(void *buffer, size_t buffer_size)
{
    bootloader_config_t result;

    cmp_ctx_t context;
    cmp_mem_access_t cma;

    cmp_mem_access_ro_init(&context, &cma, (uint8_t *)buffer + 4, buffer_size - 4);
    config_update_from_serialized(&result, &context);

    return result;
}

void config_update_from_serialized(bootloader_config_t *config, cmp_ctx_t *context)
{
    uint32_t key_count = 0;
    char key[64];
    uint32_t key_len;

    cmp_read_map(context, &key_count);

    while (key_count--) {
        key_len = sizeof key;
        cmp_read_str(context, key, &key_len);
        key[key_len] = 0;

        if (!strcmp(CONFIG_KEY_ID, key)) {
            cmp_read_uchar(context, &config->ID);
        }

        if (!strcmp(CONFIG_KEY_BOARD_NAME, key)) {
            uint32_t name_len = 65;
            cmp_read_str(context, config->board_name, &name_len);
        }

        if (!strcmp(CONFIG_KEY_DEVICE_CLASS, key)) {
            uint32_t name_len = 65;
            cmp_read_str(context, config->device_class, &name_len);
        }

        if (!strcmp(CONFIG_KEY_APPLICATION_CRC, key)) {
            cmp_read_uint(context,  &config->application_crc);
        }

        if (!strcmp(CONFIG_KEY_APPLICATION_SIZE, key)) {
            cmp_read_uint(context,  &config->application_size);
        }

        if (!strcmp(CONFIG_KEY_UPDATE_COUNT, key)) {
            cmp_read_uint(context,  &config->update_count);
        }
    }
}
