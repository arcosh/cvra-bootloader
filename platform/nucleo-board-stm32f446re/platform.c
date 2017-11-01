
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/can.h>

#include <bootloader.h>
#include <boot_arg.h>
#include <platform/mcu/armv7-m/timeout_timer.h>
#include "platform.h"


void led_init()
{
    #if (GPIO_PORT_LED2 == GPIOA)
    rcc_periph_clock_enable(RCC_GPIOA);
    #endif
    #if (GPIO_PORT_LED2 == GPIOB)
    rcc_periph_clock_enable(RCC_GPIOB);
    #endif

    gpio_mode_setup(
            GPIO_PORT_LED2,
            GPIO_MODE_OUTPUT,
            GPIO_PUPD_NONE,
            GPIO_PIN_LED2
            );
    gpio_set_output_options(
            GPIO_PORT_LED2,
            GPIO_OTYPE_PP,
            GPIO_OSPEED_2MHZ,
            GPIO_PIN_LED2
            );
    gpio_set(
            GPIO_PORT_LED2,
            GPIO_PIN_LED2
            );
}

void led_blink()
{
    // Blink onboard led
    for (uint32_t i=0;i<3;i++)
    {
        for (uint32_t j=0;j<150000;j++)
        {
            asm("nop");
        }
        gpio_toggle(GPIO_PORT_LED2, GPIO_PIN_LED2);
    }
}

void can_interface_init(void)
{
    /*
     * Enable clock on required GPIO ports
     */
    #if (GPIO_PORT_CAN_TX == GPIOA) || (GPIO_PORT_CAN_RX == GPIOA) || (defined(USE_CAN_ENABLE) && GPIO_PORT_CAN_ENABLE == GPIOA)
    rcc_periph_clock_enable(RCC_GPIOA);
    #endif
    #if (GPIO_PORT_CAN_TX == GPIOB) || (GPIO_PORT_CAN_RX == GPIOB) || (defined(USE_CAN_ENABLE) && GPIO_PORT_CAN_ENABLE == GPIOB)
    rcc_periph_clock_enable(RCC_GPIOB);
    #endif
    #if (GPIO_PORT_CAN_TX == GPIOC) || (GPIO_PORT_CAN_RX == GPIOC) || (defined(USE_CAN_ENABLE) && GPIO_PORT_CAN_ENABLE == GPIOC)
    rcc_periph_clock_enable(RCC_GPIOC);
    #endif
    #if (GPIO_PORT_CAN_TX == GPIOD) || (GPIO_PORT_CAN_RX == GPIOD) || (defined(USE_CAN_ENABLE) && GPIO_PORT_CAN_ENABLE == GPIOD)
    rcc_periph_clock_enable(RCC_GPIOD);
    #endif

    /*
     * In microcontrollers with dual-bxCAN (master+slave) configurations
     * it is obligatory to enable the master CAN's clock even if the master CAN isn't used,
     * because the master is managing the slave's access to the dual-bxCAN's SRAM.
     */
    rcc_periph_clock_enable(RCC_CAN1);
#ifdef USE_CAN2
    rcc_periph_clock_enable(RCC_CAN2);
#endif

    // Setup CAN pins
    gpio_mode_setup(
            GPIO_PORT_CAN_TX,
            GPIO_MODE_AF,
            GPIO_PUPD_PULLUP,
            GPIO_PIN_CAN_TX
            );
    gpio_set_af(
            GPIO_PORT_CAN_TX,
            GPIO_AF_CAN,
            GPIO_PIN_CAN_TX
            );

    gpio_mode_setup(
            GPIO_PORT_CAN_RX,
            GPIO_MODE_AF,
            GPIO_PUPD_PULLUP,
            GPIO_PIN_CAN_RX
            );
    gpio_set_af(
            GPIO_PORT_CAN_RX,
            GPIO_AF_CAN,
            GPIO_PIN_CAN_RX
            );

    #ifdef USE_CAN_ENABLE
    gpio_mode_setup(
            GPIO_PORT_CAN_ENABLE,
            GPIO_MODE_OUTPUT,
            GPIO_PUPD_NONE,
            GPIO_PIN_CAN_ENABLE
            );
    gpio_set_output_options(
            GPIO_PORT_CAN_ENABLE,
            GPIO_OTYPE_PP,
            GPIO_OSPEED_2MHZ,
            GPIO_PIN_CAN_ENABLE
            );
    #ifndef CAN_ENABLE_INVERTED
    gpio_set(
            GPIO_PORT_CAN_ENABLE,
            GPIO_PIN_CAN_ENABLE
            );
    #else
    gpio_clear(
            GPIO_PORT_CAN_ENABLE,
            GPIO_PIN_CAN_ENABLE
            );
    #endif // CAN_ENABLE_INVERTED
    #endif // USE_CAN_ENABLE

    /*
     * CAN1 and CAN2 are running from APB1 clock (18MHz).
     * Therefore 500kbps can be achieved with prescaler=2 like this:
     * 18MHz / 2 = 9MHz
     * 9MHz / (1tq + 10tq + 7tq) = 500kHz => 500kbit
     */
    can_init(CAN,             // Interface
             false,           // Time triggered communication mode.
             true,            // Automatic bus-off management.
             false,           // Automatic wakeup mode.
             false,           // No automatic retransmission.
             false,           // Receive FIFO locked mode.
             true,            // Transmit FIFO priority.
             CAN_BTR_SJW_1TQ, // Resynchronization time quanta jump width
             CAN_BTR_TS1_10TQ,// Time segment 1 time quanta width
             CAN_BTR_TS2_7TQ, // Time segment 2 time quanta width
             1,               // Prescaler
             false,           // Loopback
             false);          // Silent


    /*
     * A filter's bank number i.e. slave start bank number
     * defines, at which bank number the filter banks
     * should route incoming and matching frames to
     * slave (CAN2) FIFOs instead of master (CAN1) FIFOs.
     */
    #ifdef USE_CAN1
        // Filters should route to CAN1 FIFOs
        uint8_t bank_number = 27;
    #endif
    #ifdef USE_CAN2
        // Filters should route to CAN2 FIFOs
        uint8_t bank_number = 0;
    #endif

    /*
     * TODO:
     * Workaround for incorrect shift value in libopencm3
     * Fixed since libopencm3 commit 5fc4f4
     */
    #ifdef CAN_FMR_CAN2SB_SHIFT
    #undef CAN_FMR_CAN2SB_SHIFT
    #endif
    #define CAN_FMR_CAN2SB_SHIFT    8

    /*
     * Configure the start slave bank
     * by writing to the filter master register (CAN_FMR)
     */
    CAN_FMR(CAN1) &= ~((uint32_t) CAN_FMR_CAN2SB_MASK);
    CAN_FMR(CAN1) |= (uint32_t) (bank_number << CAN_FMR_CAN2SB_SHIFT);

    // TODO: When using a slave CAN, the master CAN may have to be configured to accept the desired frames, too.
    #ifdef CAN1_SET_PROMISCUOUS
    can_filter_id_mask_32bit_init(
        CAN1,
        0,      // filter nr
        0,      // id: only std id, no rtr
        0,      // mask: match only std id[10:8] = 0 (bootloader frames)
        0,      // assign to fifo0
        true    // enable
    );
    #endif

    // filter to match any standard id
    // mask bits: 0 = Don't care, 1 = mute match corresponding id bit
    can_filter_id_mask_32bit_init(
        CAN,
        1,      // filter nr
        0,      // id: only std id, no rtr
        6 | (7<<29), // mask: match only std id[10:8] = 0 (bootloader frames)
        0,      // assign to fifo0
        true    // enable
    );

//    nvic_enable_irq(NVIC_IRQ_CAN);
}

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

    // Initialize the on-board LED
    led_init();

    // Blink on-board LED to indicate platform startup
    led_blink();

    // Configure timeout of 10000 milliseconds (assuming 36 Mhz system clock, see above)
    timeout_timer_init(36000000, 10000);
    can_interface_init();
    bootloader_main(arg);

    // We only arrive here, if something went wrong. Upon next reboot directly boot into the bootloader.
    reboot_system(BOOT_ARG_START_BOOTLOADER);
}
