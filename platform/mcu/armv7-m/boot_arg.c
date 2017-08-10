#include <stdint.h>
#include <libopencm3/cm3/scb.h>
#include <boot_arg.h>

const uint32_t boot_arg_magic_value_lo = 0x01234567;
const uint32_t boot_arg_magic_value_hi = 0x0089abcd;

void reboot_system(uint8_t arg)
{
    // The address marking the beginning of RAM is defined in the linker file.
    extern uint32_t ram_begin;
    uint32_t* ram_start = (uint32_t*) &ram_begin;

    // Write a magic sequence and the argument to RAM
    ram_start[0] = boot_arg_magic_value_lo;
    ram_start[1] = boot_arg_magic_value_hi | (arg << 24);

    // Reboot with RAM content retention
    scb_reset_system();
}
