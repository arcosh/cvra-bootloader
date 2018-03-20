
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


/**
 * Configure the processor to run from the internal resonator
 */
void rcc_clock_setup_in_hsi_out_36mhz();

/**
 * Configure clock to run from external 25 MHz quartz
 */
void rcc_clock_setup_in_hse_25mhz_out_36mhz();


/**
 * Configure pin PC9 as master clock output 2
 */
void rcc_set_pc9_mco2();

void rcc_mco2_output_pll();

void rcc_mco2_output_sysclock();

void rcc_mco2_output_quartz();

#endif
