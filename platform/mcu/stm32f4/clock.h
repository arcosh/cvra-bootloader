
#ifndef CLOCK_H
#define CLOCK_H

#include <libopencm3/stm32/rcc.h>

/**
 * TODO:
 * This definition is only necessary,
 * as long as no such symbol is
 * present/being defined in libopencm3.
 */
#ifndef RCC_PLLCFGR_PLLSRC_HSI
#define RCC_PLLCFGR_PLLSRC_HSI  0
#define RCC_PLLCFGR_PLLSRC_HSE  1
#endif

#ifndef RCC_CFGR_MCO2_MASK
#define RCC_CFGR_MCO2_MASK      RCC_CFGR_MCO1_MASK
#endif


void rcc_clock_setup_in_hsi_out_36mhz(void);

void rcc_clock_setup_in_hse_25mhz_out_36mhz(void);

#endif
