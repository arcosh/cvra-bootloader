
#include <libopencm3/stm32/flash.h>
#include "flash_writer.h"

#include <platform.h>

/**
 * Configure how many bytes can be written to flash memory at once
 *
 * Default: 1 byte = FLASH_CR_PROGRAM_X8
 */
#define FLASH_PROGRAM_SIZE  FLASH_CR_PROGRAM_X8

#ifndef FLASH_SECTOR_INDEX_MAX
/**
 * Configure the highest possible flash sector index.
 * This value depends on the number of sectors
 * available in the embedded flash memory.
 *
 * Default: 11 sectors
 * This value can be overwritten in a platform.h.
 */
#define FLASH_SECTOR_INDEX_MAX  11
#endif

/**
 * Bitmask for the flash status register,
 * checking for any error flags
 *
 * Note:
 * OPERR is set only, if error interrupts
 * are enabled (ERRIE=1), see RM0390 p.82.
 */
#define FLASH_SR_ANY_ERROR  ( \
            FLASH_SR_PGSERR | \
            FLASH_SR_PGPERR | \
            FLASH_SR_PGAERR | \
            FLASH_SR_WRPERR | \
            FLASH_SR_OPERR  )

/**
 * Variable to store, if a given flash sector was erased already.
 * This variable shall prevent sectors being unnecessarily erased multiple times in a row.
 */
static bool flash_sector_is_erased[FLASH_SECTOR_INDEX_MAX];


/**
 * Return the flash sector number for a given address
 *
 * @param addr      Any address lying within flash memory address space
 * @return          Index of the flash sector containing the address
 */
static uint8_t flash_address_to_sector(uint32_t addr)
{
    uint8_t sector;
    uint32_t offset = addr & 0xFFFFFF;
    if (offset < 0x10000) {
        sector = offset / 0x4000; // 16K sectors, 0x08000000 to 0x0800FFFF
    } else if (offset < 0x20000) {
        sector = 3 + offset / 0x10000; // 64K sector, 0x08010000 to 0x0801FFFF
    } else {
        sector = 4 + offset / 0x20000; // 128K sectors, 0x08010000 to 0x080FFFFF
    }
    if (addr >= 0x08100000) {
        // Bank 2, same layout, starting at 0x08100000
        sector += 12;
    }
    return sector;
}


/**
 * Returns the start address of the flash memory sector this address lies within
 *
 * @param address   Any address lying within flash memory address space
 * @return          Start address of flash sector
 * @retval 0        Invalid address
 *
 * TODO: This function does not support dual bank flash memory.
 */
static uint32_t flash_get_sector_start(uint32_t address)
{
    // Check, if ddress is outside flash memory boundaries
    extern uint32_t flash_begin, flash_end;
    if (address < (uint32_t) (&flash_begin) || address > (uint32_t) (&flash_end))
        return 0;

    // Sector 7
    if (address > 0x08060000)
        return 0x08060000;
    // Sector 6
    if (address > 0x08040000)
        return 0x08040000;
    // Sector 5
    if (address > 0x08020000)
        return 0x08020000;
    // Sector 4
    if (address > 0x08010000)
        return 0x08010000;
    // Sector 3
    if (address > 0x0800C000)
        return 0x0800C000;
    // Sector 2
    if (address > 0x08008000)
        return 0x08008000;
    // Sector 1
    if (address > 0x08004000)
        return 0x08004000;
    // Sector 0
    return 0x08000000;
}


/**
 * Returns the size of the flash memory sector for this address
 *
 * @param address   Any address lying within flash memory address space
 * @return          Flash sector size
 * @retval 0        Invalid address
 *
 * TODO: This function does not support dual bank flash memory.
 */
static uint32_t flash_get_sector_size(uint32_t address)
{
    // Check, if ddress is outside flash memory boundaries
    extern uint32_t flash_begin, flash_end;
    if (address < (uint32_t)(&flash_begin) || address > (uint32_t)(&flash_end))
        return 0;

    // Sectors 0-3: 16K
    if (address < 0x08010000)
        return 0x4000;
    // Sector 4: 64K
    if (address < 0x08020000)
        return 0x10000;
    // Sectors 5-7: 128K
    return 0x20000;
}


void flash_writer_unlock(void)
{
    flash_unlock();
}


void flash_writer_lock(void)
{
    flash_lock();
}


void flash_writer_page_erase(void *page)
{
    // Get sector index for this address
    uint8_t sector = flash_address_to_sector((uint32_t)page);

    // Check, if the sector is erased already
    if (flash_sector_is_erased[sector])
        // No need to erase it again
        return;

    // Enable error interrupts
    // This is required for OPERR to be set by hardware, see RM0390 p.82.
    FLASH_CR |= FLASH_CR_ERRIE;

    // Clear error flags
    FLASH_SR &= ~FLASH_SR_ANY_ERROR;

    /*
     * TODO:
     * There is an error in flash_set_program_size() in libopencm3 commit acbae651.
     * psize must actually be shifted << 8.
     * Valid arguments for psize are FLASH_CR_PROGRAM_X8 through FLASH_CR_PROGRAM_X64.
     * The error is corrected in recent versions of libopencm3 (at least in 668c7c50).
     *
     * TODO:
     * It is unclear, why the parallel programming size should be configured for
     * flash sector erasing. According to the RM p.69 this is not required.
     */
    flash_erase_sector(sector, FLASH_PROGRAM_SIZE);

    // Check FLASH_SR for success
    if (FLASH_SR & FLASH_SR_ANY_ERROR)
    {
        // Clear error flags
        FLASH_SR &= ~FLASH_SR_ANY_ERROR;
        // TODO: Erasing failed. Deal with it.
        return;
    }

    // Mark flash sector as erased
    flash_sector_is_erased[sector] = true;
}


void flash_writer_page_write(void *page, void *data, size_t len)
{
    // Mark target sectors as not-erased
    uint32_t a = (uint32_t) page;
    while (a < ((uint32_t) page) + len)
    {
        uint8_t sector = flash_address_to_sector(a);
        flash_sector_is_erased[sector] = false;
        a += flash_get_sector_size(a);
    }

    // Enable error interrupts
    // This is required for OPERR to be set by hardware, see RM0390 p.82.
    FLASH_CR |= FLASH_CR_ERRIE;

    // Clear error flags
    FLASH_SR &= ~FLASH_SR_ANY_ERROR;

    // Write data to flash
    flash_program((uint32_t)page, data, len);

    // Check FLASH_SR for success
    if (FLASH_SR & FLASH_SR_ANY_ERROR)
    {
        // Clear error flags
        FLASH_SR &= ~FLASH_SR_ANY_ERROR;
        // TODO: Programming failed. Deal with it.
        return;
    }
}

