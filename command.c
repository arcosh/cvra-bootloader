#include <string.h>
#include <crc/crc32.h>
#include <cmp_mem_access/cmp_mem_access.h>
#include <platform.h>
#include "flash_writer.h"
#include "boot_arg.h"
#include "config.h"
#include "command.h"
#include "error.h"


/**
 * Supported command set
 * Associates command codes and appropriate command handlers
 *
 * This set must be compatible with the client,
 * i.e. with the values in class CommandType
 * in file client/cvra_bootloader/commands.py
 */
command_t commands[COMMAND_COUNT] = {
    {.index = 1, .callback = command_jump_to_application},
    {.index = 2, .callback = command_crc_region},
    {.index = 3, .callback = command_erase_flash_page},
    {.index = 4, .callback = command_write_flash},
    {.index = 5, .callback = command_ping},
    {.index = 6, .callback = command_read_flash},
    {.index = 7, .callback = command_config_update},
    {.index = 8, .callback = command_config_write_to_flash},
    {.index = 9, .callback = command_config_read},
    {.index = 10, .callback = command_get_status},
};


command_t* get_command_by_index(uint8_t index)
{
    for (uint8_t i=0; i<COMMAND_COUNT; i++)
        if (commands[i].index == index)
            return &commands[i];

    // Search was unsuccessful: No such command
    return 0;
}


void command_erase_flash_page(int argc, cmp_ctx_t *args, cmp_ctx_t *out, bootloader_config_t *config)
{
    void *address;
    uint64_t tmp = 0;
    char device_class[64];

    // Read address (unsigned 64-bit integer) from MessagePack
    cmp_read_uinteger(args, &tmp);
    address = (void *)(uintptr_t)tmp;

    // Refuse to overwrite bootloader or config pages
    if (address < memory_get_app_addr()) {
        cmp_write_uint(out, FLASH_ERASE_ERROR_BEFORE_APP);
        return;
    }

    // Refuse to erase past end of flash memory
    if (address >= memory_get_app_addr() + memory_get_app_size()) {
        cmp_write_uint(out, FLASH_ERASE_ERROR_AFTER_APP);
        return;
    }

    // Read device class (string) from MessagePack
    uint32_t size = 64;
    cmp_read_str(args, device_class, &size);

    // Check device class
    if (strcmp(device_class, config->device_class) != 0) {
        cmp_write_uint(out, FLASH_ERASE_ERROR_DEVICE_CLASS_MISMATCH);
        return;
    }

    // Erase flash at specified address
    flash_writer_unlock();
    flash_writer_page_erase(address);
    flash_writer_lock();

    // Erasure succeeded
    cmp_write_uint(out, FLASH_ERASE_SUCCESS);
}


void command_write_flash(int argc, cmp_ctx_t *args, cmp_ctx_t *out, bootloader_config_t *config)
{
    void *address;
    void *src;
    uint64_t tmp = 0;
    uint32_t size;
    char device_class[64];

    // Read address (unsigned 64-bit integer) from MessagePack
    cmp_read_uinteger(args, &tmp);
    address = (void *)(uintptr_t)tmp;

    // Refuse to overwrite bootloader or config pages
    if (address < memory_get_app_addr()) {
        cmp_write_uint(out, FLASH_WRITE_ERROR_BEFORE_APP);
        return;
    }

    // Refuse to erase past end of flash memory
    if (address >= memory_get_app_addr() + memory_get_app_size()) {
        cmp_write_uint(out, FLASH_WRITE_ERROR_AFTER_APP);
        return;
    }

    // Read device class (string) from MessagePack
    size = 64;
    cmp_read_str(args, device_class, &size);

    // Check device class
    if (strcmp(device_class, config->device_class) != 0) {
        cmp_write_uint(out, FLASH_WRITE_ERROR_DEVICE_CLASS_MISMATCH);
        return;
    }

    // Read data size (integer) from MessagePack
    if (!cmp_read_bin_size(args, &size)) {
        cmp_write_uint(out, FLASH_WRITE_ERROR_UNKNOWN_SIZE);
        return;
    }

    /*
     * Get pointer to received data buffer.
     * This is ugly, yet required to achieve zero copy.
     */
    cmp_mem_access_t *cma = (cmp_mem_access_t *)(args->buf);
    src = cmp_mem_access_get_ptr_at_pos(cma, cmp_mem_access_get_pos(cma));

    /*
     * Make sure the target area is erased.
     * TODO: Verify that the code below is working.
     */
    uint8_t* p = address;
    for (uint32_t i=0; i<size; i++) {
        if (*(p++) != 0xFF) {
            // Not erased
            cmp_write_uint(out, FLASH_WRITE_ERROR_NOT_ERASED);
            return;
        }
    }

    // Write received data to flash
    flash_writer_unlock();
    flash_writer_page_write(address, src, size);
    flash_writer_lock();

    // Writing to flash succeeded
    cmp_write_bool(out, FLASH_WRITE_SUCCESS);
    return;
}


void command_read_flash(int argc, cmp_ctx_t *args, cmp_ctx_t *out, bootloader_config_t *config)
{
    void *address;
    uint64_t tmp;
    uint32_t size;

    // Read address (unsigned 64-bit integer) from MessagePack
    cmp_read_uinteger(args, &tmp);
    address = (void *)(uintptr_t)tmp;

    // Read size (unsigned 32-bit integer) from MessagePack
    cmp_read_u32(args, &size);

    // Return MessagePack with binary data
    cmp_write_bin(out, address, size);
}


void command_jump_to_application(int argc, cmp_ctx_t *args, cmp_ctx_t *out, bootloader_config_t *config)
{
#ifndef COMMAND_JUMP_DISABLE_CRC_CHECKING
    // Compare the CRC of the flashed application with the CRC stored in the config
    if (crc32(0, memory_get_app_addr(), config->application_size) == config->application_crc) {
        // CRC is valid: run application
        reboot_system(BOOT_ARG_START_APPLICATION);
    } else {
        // CRC is invalid: reboot and remain in bootloader
        reboot_system(BOOT_ARG_START_BOOTLOADER_NO_TIMEOUT);
    }
#else
    // Run the flashed application regardless of whether it's CRC is valid or not
    reboot_system(BOOT_ARG_START_APPLICATION);
#endif
}

void command_crc_region(int argc, cmp_ctx_t *args, cmp_ctx_t *out, bootloader_config_t *config)
{
    uint32_t crc = 0;
    void *address;
    uint32_t size;
    uint64_t tmp;

    // Read first command argument from MessagePack: address (unsigned 64-bit integer)
    if (!cmp_read_uinteger(args, &tmp))
    {
        cmp_write_uint(out, CRC_ERROR_ADDRESS_UNSPECIFIED);
        return;
    }
    address = (void *)(uintptr_t)tmp;

    // Second command argument: size (unsigned 32-bit integer)
    if (!cmp_read_uint(args, &size))
    {
        cmp_write_uint(out, CRC_ERROR_LENGTH_UNSPECIFIED);
        return;
    }

    // Import flash boundaries from linker script
    extern uint32_t flash_begin;
    extern uint32_t flash_end;

    // Check if the provided arguments are acceptable
    uint32_t address1 = (uint32_t) address;
    uint32_t address2 = address1 + size;
    if (address1 < (uint32_t) (&flash_begin) || address1 >= (uint32_t) (&flash_end)
     || address2 < (uint32_t) (&flash_begin) || address2 >= (uint32_t) (&flash_end))
    {
        // TODO: Test, whether the above statement can become true, even if it should return false.
        cmp_write_uint(out, CRC_ERROR_ILLEGAL_ADDRESS);
        return;
    }

    // Calculate checksum over the requested address range
    crc = crc32(0, address, size);

    // Return calculated checksum value
    cmp_write_uint(out, crc);
}


void command_config_update(int argc, cmp_ctx_t *args, cmp_ctx_t *out, bootloader_config_t *config)
{
    config_update_from_serialized(config, args);
    cmp_write_bool(out, 1);
}


void command_ping(int argc, cmp_ctx_t *args, cmp_ctx_t *out, bootloader_config_t *config)
{
    // Reply: 1
    cmp_write_bool(out, 1);
}


static bool flash_write_and_verify(void *addr, void *data, size_t len)
{
    flash_writer_unlock();
    flash_writer_page_erase(addr);
    flash_writer_page_write(addr, data, len);
    flash_writer_lock();
    return config_is_valid(addr, len);
}


void command_config_write_to_flash(int argc, cmp_ctx_t *args, cmp_ctx_t *out, bootloader_config_t *config)
{
    // Increment the number of times, this board has been flashed
    config->update_count += 1;

    // Prepare a zero-padded configuration page buffer
    static uint8_t config_page_buffer[CONFIG_PAGE_SIZE];
    memset(config_page_buffer, 0, CONFIG_PAGE_SIZE);
    config_write(config_page_buffer, config, CONFIG_PAGE_SIZE);

    // Get pointers to configuration pages in flash memory
    void *config1 = memory_get_config1_addr();
    void *config2 = memory_get_config2_addr();

    // Update both config pages
    // The update order shall prevent a valid configuration from being overwritten, if one update fails.
    bool success = false;
    if (config_is_valid(config2, CONFIG_PAGE_SIZE)) {
        if (flash_write_and_verify(config1, config_page_buffer, CONFIG_PAGE_SIZE)) {
            if (flash_write_and_verify(config2, config_page_buffer, CONFIG_PAGE_SIZE)) {
                success = true;
            }
        }
    } else if (config_is_valid(config1, CONFIG_PAGE_SIZE)) {
        if (flash_write_and_verify(config2, config_page_buffer, CONFIG_PAGE_SIZE)) {
            if (flash_write_and_verify(config1, config_page_buffer, CONFIG_PAGE_SIZE)) {
                success = true;
            }
        }
    } else {
        success = flash_write_and_verify(config1, config_page_buffer, CONFIG_PAGE_SIZE);
        success &= flash_write_and_verify(config2, config_page_buffer, CONFIG_PAGE_SIZE);
    }

    // Return whether updating both config pages was successful or not
    if (success) {
        cmp_write_bool(out, 1);
    } else {
        cmp_write_bool(out, 0);
    }
}


void command_config_read(int argc, cmp_ctx_t *args, cmp_ctx_t *out, bootloader_config_t *config)
{
    // Return currently active configuration
    config_write_messagepack(out, config);
}


int execute_datagram_commands(char *data, size_t data_len, const command_t *commands, int command_len, char *out_buf, size_t out_len, bootloader_config_t *config)
{
    cmp_mem_access_t command_cma;
    cmp_ctx_t command_reader;
    int32_t command_index;
    uint32_t argc;
    int32_t command_version;
    bool read_success;

    cmp_mem_access_t out_cma;
    cmp_ctx_t out_writer;

    // Prepare for reading commands from input buffer
    cmp_mem_access_ro_init(&command_reader, &command_cma, data, data_len);
    cmp_read_int(&command_reader, &command_version);

    // Prepare output buffer for reponse
    cmp_mem_access_init(&out_writer, &out_cma, out_buf, out_len);

    // Make sure, client used compatible command set version
    if (command_version != COMMAND_SET_VERSION) {
        return -ERR_INVALID_COMMAND_SET_VERSION;
    }

    // Read requested command index from MessagePack
    read_success = cmp_read_int(&command_reader, &command_index);

    if (!read_success) {
        return -ERR_INVALID_COMMAND;
    }

    // Read the list of arguments for this command from MessagePack
    read_success = cmp_read_array(&command_reader, &argc);

    // If we cannot read the array size, we assume it is because no arguments were provided.
    if (!read_success) {
        argc = 0;
    }

    // Depending on the command type, invoke the corresponding command handler
    command_t* cmd;
    cmd = get_command_by_index(command_index);
    if (cmd != 0)
    {
        cmd->callback(argc, &command_reader, &out_writer, config);
        return cmp_mem_access_get_pos(&out_cma);
    }
    return -ERR_COMMAND_NOT_FOUND;
}


static volatile uint8_t status;

void set_status(uint8_t code)
{
    status = code;
}

void command_get_status(int argc, cmp_ctx_t *args, cmp_ctx_t *out, bootloader_config_t *config)
{
    cmp_write_u8(out, status);
}
