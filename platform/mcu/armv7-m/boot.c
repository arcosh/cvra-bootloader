
#include <stdint.h>
#include <libopencm3/cm3/scb.h>
#include "boot_arg.h"
#include <bootloader.h>


/**
 * Start main firmware
 */
static void start_app()
{
    // Import beginning of application section from linker file
    extern uint32_t application_address;

    // Relocate vector table to beginning of application section
    SCB_VTOR = (uint32_t) (&application_address);

    // Set stack pointer and execute application's reset handler
    asm("dsb;"
        "ldr     r0, =application_address;" \
        "ldr     sp, [r0, #0];" \
        "ldr     r0, [r0, #4];" \
        "bx      r0;");
}


/**
 * Start ST's bootloader
 */
static void start_st_bootloader()
{
    // Set stack pointer and execute the ST bootloader's reset handler
    asm("ldr     r0, =0x1FFF0000;" \
        "ldr     sp, [r0, #0];" \
        "ldr     r0, [r0, #4];" \
        "bx      r0;");
}


/**
 * The first code invoked upon system startup and reboot/reset
 */
void reset_handler()
{
    // Import beginning of RAM section from linker file
    extern uint32_t ram_begin;

    // Extract the first two words in RAM
    const uint32_t magic_low  = ((uint32_t*) (&ram_begin))[0];
    const uint32_t magic_high = ((uint32_t*) (&ram_begin))[1];
    const uint8_t argv = magic_high >> 24;
    magic_high &= 0x00FFFFFF;

    // Initialize global variables with compile-time values
    extern uint32_t _etext;
    extern uint32_t _sdata;
    extern uint32_t _edata;
    uint32_t* src = &_etext;
    uint32_t* dst = &_sdata;
    while (dst <= &_edata)
        *(dst++) = *(src++);

    // Fill uninitialized variable memory with zeroes
    extern uint32_t _sbss;
    extern uint32_t _ebss;
    dst = &_sbss;
    while (dst < &_ebss)
        *(dst++) = 0;

    // Import magic words from boot_arg.c
    extern uint32_t boot_arg_magic_value_lo;
    extern uint32_t boot_arg_magic_value_hi;

    // Check magic words
    if (magic_low  != boot_arg_magic_value_lo
     || magic_high != boot_arg_magic_value_hi)
    {
        // Invalid magic value: Start bootloader with timeout
        bootloader_main(BOOT_ARG_START_BOOTLOADER);
    }

    // Proceed according to the argument byte encountered in RAM
    if (argv == BOOT_ARG_START_APPLICATION)
        start_app();
    if (argv == BOOT_ARG_START_ST_BOOTLOADER)
        start_st_bootloader();
    bootloader_main(argv);
}
