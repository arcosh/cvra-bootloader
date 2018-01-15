
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/can.h>

#include <bootloader.h>
#include <boot_arg.h>
#include <can_interface.h>
#include <led.h>
#include <platform/mcu/armv7-m/timeout_timer.h>
#include "platform.h"


void fault_handler(void)
{
    // while(1); // debug
    reboot_system(BOOT_ARG_START_BOOTLOADER_NO_TIMEOUT);
}


/**
 * This function is only necessary,
 * whilst using an old version of libopencm3.
 * Beginning with libopencm3 commit 08aac0
 * rcc_clock_setup_in_hsi_out_36mhz() should
 * be modified to instead use rcc_is_osc_ready().
 */
void rcc_wait_for_osc_not_ready(enum rcc_osc osc)
{
    switch (osc) {
    case PLL:
        while ((RCC_CR & RCC_CR_PLLRDY) != 0);
        break;
    case HSE:
        while ((RCC_CR & RCC_CR_HSERDY) != 0);
        break;
    case HSI:
        while ((RCC_CR & RCC_CR_HSIRDY) != 0);
        break;
    case LSE:
        while ((RCC_BDCR & RCC_BDCR_LSERDY) != 0);
        break;
    case LSI:
        while ((RCC_CSR & RCC_CSR_LSIRDY) != 0);
        break;
    }
}


/**
 * TODO:
 * This definition is only necessary,
 * as long as no such symbol is
 * present/being defined in libopencm3.
 */
#ifndef RCC_PLLCFGR_PLLSRC_HSI
#define RCC_PLLCFGR_PLLSRC_HSI  0
#endif

/**
 * Configure the processor to run at 36MHz from the internal resonator
 */
void rcc_clock_setup_in_hsi_out_36mhz(void)
{
    // Enable internal high-speed resonator (16 MHz)
    rcc_osc_on(HSI);
    // Wait for internal resonator to become ready
    rcc_wait_for_osc_ready(HSI);
    // While configuring the clock, the processor must run directly from HSI
    rcc_set_sysclk_source(RCC_CFGR_SW_HSI);
    rcc_wait_for_sysclk_status(HSI);

    // Disable PLL (obligatory before configuring PLL)
    rcc_osc_off(PLL);
    rcc_wait_for_osc_not_ready(PLL);
    // Configure PLL source multiplexer to use internal resonator (HSI)
    rcc_set_pll_source(RCC_PLLCFGR_PLLSRC_HSI);
    // Configure PLL to scale input to 72 MHz:
    // HSI=16 MHz / M=8 * N=72 / P=2 = 36 MHz, Q and R are irrelevant
    rcc_set_main_pll_hsi(8, 72, 2, 2);
    // Re-enable PLL
    rcc_osc_on(PLL);
    rcc_wait_for_osc_ready(PLL);

    // Set flash latency
    flash_set_ws(1);

    // Configure AHB prescaler
    rcc_set_hpre(2);

    // Set system clock source to PLL
    rcc_set_sysclk_source(RCC_CFGR_SW_PLL);
    rcc_wait_for_sysclk_status(PLL);

    // Configure APB1 and APB2 prescaler
    rcc_set_ppre1(5);
    rcc_set_ppre2(4);
}


void platform_main(int arg)
{
    // Run from internal RC oscillator
    rcc_clock_setup_in_hsi_out_36mhz();

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
