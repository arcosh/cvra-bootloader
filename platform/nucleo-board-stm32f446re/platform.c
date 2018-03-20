
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <libopencm3/stm32/can.h>

#include <bootloader.h>
#include <boot_arg.h>
#include <can_interface.h>
#include <led.h>
#include <platform/mcu/armv7-m/timeout_timer.h>
#include <platform/mcu/stm32f4/clock.h>

#include "platform.h"


void fault_handler(void)
{
    // while(1); // debug
    reboot_system(BOOT_ARG_START_BOOTLOADER_NO_TIMEOUT);
}


void platform_main(int arg)
{
    // Run from internal RC oscillator
    rcc_clock_setup_in_hsi_out_36mhz();

    // Run from external 25 MHz quartz
//    rcc_clock_setup_in_hse_25mhz_out_36mhz();

    // Initialize the on-board LED(s)
    led_init();

    // Configure timeout according to platform-specific define (assuming 36 Mhz system clock, see above)
    timer_init(36000000, BOOTLOADER_TIMEOUT, DATAGRAM_TIMEOUT);

    // Blink on-board LED to indicate platform startup (must be after timer_init())
    led_on(LED_SUCCESS);

    // Configure and enable CAN peripheral
    can_interface_init();

    // Start bootloader program
    bootloader_main(arg);

    // We only arrive here, if something went wrong. Upon next reboot directly boot into the bootloader.
    reboot_system(BOOT_ARG_START_BOOTLOADER);
}
