/**
 * Platform-specific definitions for the STM32-Nucleo64 Board F446RE
 * http://www.st.com/en/evaluation-tools/nucleo-f446re.html
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "platform_config.h"


#ifndef PLATFORM_DEVICE_CLASS
#define PLATFORM_DEVICE_CLASS       "nucleo-board-stm32f446re"
#endif

#ifdef PLATFORM_ENABLE_UUID
#include <libopencm3/cm3/common.h>  // MMIO
#include <libopencm3/stm32/f4/memorymap.h>  // Unique ID
#include <stdio.h>  // sprintf()

#define PLATFORM_DEFAULT_NAME       platform_get_default_name()
#endif


/**
 * Number of times, the flash sector erase command
 * should retry upon errors, before giving up
 */
#define FLASH_ERASE_RETRIES         20

/**
 * The STM32F446RE has eight flash sectors,
 * see RM0390 rev.3 on page 64.
 */
#define FLASH_SECTOR_INDEX_MAX      8

/**
 * The amount of flash memory (in bytes)
 * reserved for a configuration struct,
 * which is located at the beginning of
 * the corresponding config page flash sector.
 */
#define CONFIG_PAGE_SIZE            512


/**
 * Configure onboard LEDs
 */
#ifndef LED_SUCCESS
#define LED_SUCCESS     1
#endif
#ifndef LED_ERROR
#define LED_ERROR       2
#endif

#if defined(LED1) && (!defined(GPIO_PIN_LED1))
#define GPIO_PORT_LED1  GPIOA
#define GPIO_PIN_LED1   GPIO5
#endif

#ifndef LED_ON_DURATION_MS
#define LED_ON_DURATION_MS  50
#endif

/**
 * CAN peripheral default configuration
 */
#if (!defined(USE_CAN1)) && (!defined(USE_CAN2))
//#define USE_CAN1
#define USE_CAN2
#endif

#ifndef CAN_PRESCALER
#define CAN_PRESCALER   1
#endif

#ifndef CAN_SJW
#define CAN_SJW         CAN_BTR_SJW_1TQ
#endif

#ifndef CAN_TS1
#define CAN_TS1         CAN_BTR_TS1_15TQ
#endif

#ifndef CAN_TS2
#define CAN_TS2         CAN_BTR_TS2_2TQ
#endif

/**
 * Configure which GPIO pins to use as CAN RX and TX
 */
#if     defined(USE_CAN1) \
    && (!defined(CAN_USE_PINS_PB8_PB9)) \
    && (!defined(CAN_USE_PINS_PA11_PA12)) \
    && (!defined(CAN_USE_PINS_PD0_PD1))
#define CAN_USE_PINS_PB8_PB9
//#define CAN_USE_PINS_PA11_PA12
//#define CAN_USE_PINS_PD0_PD1
#endif

#if     defined(USE_CAN2) \
    && (!defined(CAN_USE_PINS_PB5_PB6)) \
    && (!defined(CAN_USE_PINS_PB12_PB13)) \
    && (!defined(CAN_USE_PINS_ST_BOOTLOADER))
//#define CAN_USE_PINS_PB5_PB6
#define CAN_USE_PINS_PB12_PB13
//#define CAN_USE_PINS_ST_BOOTLOADER
#endif

/**
 * Alternate function configuration
 * for the GPIOs in order to be used as CAN RX/TX
 */
#define GPIO_AF_CAN         GPIO_AF9

/*
 * Pin configuration when using CAN1
 */
#ifdef USE_CAN1
#define CAN                 CAN1

/**
 * CAN1_RX <- PB8
 * CAN1_TX -> PB9
 */
#ifdef CAN_USE_PINS_PB8_PB9
#define GPIO_PORT_CAN_RX    GPIOB
#define GPIO_PIN_CAN_RX     GPIO8
#define GPIO_PORT_CAN_TX    GPIOB
#define GPIO_PIN_CAN_TX     GPIO9
#endif

/**
 * CAN1_RX <- PA11
 * CAN1_TX -> PA12
 */
#ifdef CAN_USE_PINS_PA11_PA12
#define GPIO_PORT_CAN_RX    GPIOA
#define GPIO_PIN_CAN_RX     GPIO11
#define GPIO_PORT_CAN_TX    GPIOA
#define GPIO_PIN_CAN_TX     GPIO12
#endif

/**
 * CAN1_RX <- PD0
 * CAN1_TX -> PD1
 */
#ifdef CAN_USE_PINS_PD0_PD1
#define GPIO_PORT_CAN_RX    GPIOD
#define GPIO_PIN_CAN_RX     GPIO0
#define GPIO_PORT_CAN_TX    GPIOD
#define GPIO_PIN_CAN_TX     GPIO1
#endif
#endif // USE_CAN1

/*
 * Pin configuration when using CAN2
 */
#ifdef USE_CAN2
#define CAN                 CAN2

/**
 * CAN2_RX <- PB5
 * CAN2_TX -> PB6
 */
#ifdef CAN_USE_PINS_PB5_PB6
#define GPIO_PORT_CAN_RX    GPIOB
#define GPIO_PIN_CAN_RX     GPIO5
#define GPIO_PORT_CAN_TX    GPIOB
#define GPIO_PIN_CAN_TX     GPIO6
#endif

/**
 * CAN2_RX <- PB12
 * CAN2_TX -> PB13
 */
#ifdef CAN_USE_PINS_PB12_PB13
#define GPIO_PORT_CAN_RX    GPIOB
#define GPIO_PIN_CAN_RX     GPIO12
#define GPIO_PORT_CAN_TX    GPIOB
#define GPIO_PIN_CAN_TX     GPIO13
#endif

/**
 * The following pin configuration is compatible with ST's CAN bootloader,
 * which is present in the microcontroller's ROM.
 *
 * CAN2_RX <- PB5
 * CAN2_TX -> PB13
 */
#ifdef CAN_USE_PINS_ST_BOOTLOADER
#define GPIO_PORT_CAN_RX    GPIOB
#define GPIO_PIN_CAN_RX     GPIO5
#define GPIO_PORT_CAN_TX    GPIOB
#define GPIO_PIN_CAN_TX     GPIO13
#endif
#endif // USE_CAN2

/**
 * Maximum number of transmission retries
 * upon CAN transmission failure
 */
#ifndef CAN_SEND_RETRIES
#define CAN_SEND_RETRIES    100
#endif

/**
 * Maximum number of times, the CAN RX FIFO
 * is polled (in immediate successsion)
 * for new, incoming messages
 */
#ifndef CAN_RECEIVE_TIMEOUT
#define CAN_RECEIVE_TIMEOUT 10
#endif


// Import symbols from linker script
extern int application_address, application_size, config_page1, config_page2;

static inline void *memory_get_app_addr(void)
{
    return (void *) &application_address;
}

static inline size_t memory_get_app_size(void)
{
    return (size_t)&application_size;
}

static inline void *memory_get_config1_addr(void)
{
    return (void *) &config_page1;
}

static inline void *memory_get_config2_addr(void)
{
    return (void *) &config_page2;
}


#ifdef PLATFORM_ENABLE_UUID
/**
 * The default board name to set in a brand new config page
 */
static inline char* platform_get_default_name()
{
    static char s[64];
    sprintf(s, "Nucleo-F446 Serial No. %x-%x-%x", DESIG_UNIQUE_ID0, DESIG_UNIQUE_ID1, DESIG_UNIQUE_ID2);
    return s;
}
#endif // PLATFORM_ENABLE_UUID

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_H */
