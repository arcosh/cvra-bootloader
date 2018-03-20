
#include "clock.h"
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>


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


void rcc_mco2_output_pll()
{
    uint32_t tmp = RCC_CFGR & ~(RCC_CFGR_MCO2_MASK << RCC_CFGR_MCO2_SHIFT);
    RCC_CFGR = tmp | (RCC_CFGR_MCO2_PLL << RCC_CFGR_MCO2_SHIFT);
}


void rcc_mco2_output_sysclock()
{
    uint32_t tmp = RCC_CFGR & ~(RCC_CFGR_MCO2_MASK << RCC_CFGR_MCO2_SHIFT);
    RCC_CFGR = tmp | (RCC_CFGR_MCO2_SYSCLK << RCC_CFGR_MCO2_SHIFT);
}


void rcc_mco2_output_quartz()
{
    uint32_t tmp = RCC_CFGR & ~(RCC_CFGR_MCO2_MASK << RCC_CFGR_MCO2_SHIFT);
    RCC_CFGR = tmp | (RCC_CFGR_MCO2_HSE << RCC_CFGR_MCO2_SHIFT);
}


void rcc_set_pc9_mco2()
{
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
    gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO9);
    gpio_set_af(GPIOC, GPIO_AF0, GPIO9);
}


/**
 * Configure the processor to run at 36MHz from the internal resonator
 */
void rcc_clock_setup_in_hsi_out_36mhz(void)
{
//    rcc_set_mco2_pll();
    rcc_set_pc9_mco2();

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
//    flash_set_ws(1);
    flash_set_ws(2);

    // Configure AHB prescaler
    rcc_set_hpre(2);

    // Set system clock source to PLL
    rcc_set_sysclk_source(RCC_CFGR_SW_PLL);
    rcc_wait_for_sysclk_status(PLL);

    // Configure APB1 and APB2 prescaler
    rcc_set_ppre1(5);
    rcc_set_ppre2(4);
}


/**
 * Configure clock to run from external 25 MHz quartz
 */
void rcc_clock_setup_in_hse_25mhz_out_36mhz(void)
{
    /*
     * PLLM = 25
     * PLLN = 144
     * PLLP = 2
     * PLLQ = doesn't matter
     * PLLR = doesn't matter
     */
    const uint8_t pllm = 25;
    const uint8_t plln = 144;
    // TODO: Why does it work with P=20, but not with P=2?
    const uint8_t pllp = 20;
    const uint8_t pllq = 2;

    /*
     * Enable internal RC oscillator
     */
    rcc_osc_on(HSI);
    rcc_wait_for_osc_ready(HSI);

    /*
     * Select internal oscillator as SYSCLK source
     */
    rcc_set_sysclk_source(RCC_CFGR_SW_HSI);
    rcc_wait_for_sysclk_status(HSI);

    /*
     * Enable external oscillator
     */
    rcc_osc_on(HSE);
    rcc_wait_for_osc_ready(HSE);

    /*
     * Disable PLL before configuring it
     */
    rcc_osc_off(PLL);
    rcc_wait_for_osc_not_ready(PLL);

    /*
     * Configure PLL
     * This also configures HSE as PLL source.
     */
    rcc_set_main_pll_hse(pllm, plln, pllp, pllq);

    /*
     * Enable PLL oscillator and wait for it to stabilize
     */
    rcc_osc_on(PLL);
    rcc_wait_for_osc_ready(PLL);

    /*
     * Set the bus prescalers
     *
     * AHB prescaler = 2
     * APB1 prescaler = 1
     * APB2 prescaler = 1
     */
    rcc_set_hpre(RCC_CFGR_HPRE_DIV_2);
    rcc_set_ppre1(RCC_CFGR_PPRE_DIV_NONE);
    rcc_set_ppre2(RCC_CFGR_PPRE_DIV_NONE);

//    rcc_set_hpre(1 << 2);
//    rcc_set_ppre1(0);
//    rcc_set_ppre2(0);

    /*
     * Save the configured clock frequencies
     */
//    rcc_ahb_frequency = 36000000;
//    rcc_apb1_frequency = 36000000;
//    rcc_apb2_frequency = 36000000;

    /*
     * Adjust flash writer wait states
     *
     * 0WS from 0-24MHz
     * 1WS from 24-48MHz
     * 2WS from 48-72MHz
     */
    flash_set_ws(FLASH_ACR_LATENCY_2WS);

    /*
     * Select PLL as SYSCLK source
     */
    rcc_set_sysclk_source(RCC_CFGR_SW_PLL);
    rcc_wait_for_sysclk_status(PLL);
}
