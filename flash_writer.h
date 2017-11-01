#ifndef FLASH_WRITER_H
#define FLASH_WRITER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unlocks the flash for programming
 *
 * In order to prevent accidental flash operations,
 * due to e.g. electrical disturbances,
 * the flash memory must be unlocked
 * using magic bytes before it can be written.
 *
 */
void flash_writer_unlock(void);

/**
 * Locks the flash thus disabling
 * all further flash write operations
 */
void flash_writer_lock(void);

/**
 * Erase flash page at the given address
 *
 * @param page      Pointer to any address within the flash memory sector, that shall be erased.
 */
void flash_writer_page_erase(void *page);

/**
 * Writes data to given location in flash
 *
 * @param page      Pointer i.e. target flash address to write to
 * @param data      Pointer to data buffer to write
 * @param len       Number of bytes to write to flash
 */
void flash_writer_page_write(void *page, void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_WRITER_H */
