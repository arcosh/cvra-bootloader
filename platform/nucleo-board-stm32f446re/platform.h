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

#define PLATFORM_DEVICE_CLASS "nucleo-board-stm32f446re"
#define FLASH_PAGE_SIZE 0x0800 // 2K
#define CONFIG_PAGE_SIZE FLASH_PAGE_SIZE

// Onboard LED
#define GPIO_PORT_LED2  GPIOA
#define GPIO_PIN_LED2   GPIO5

/*
 * Configure which CAN peripheral to use
 */
//#define USE_CAN1
#define USE_CAN2

/*
 * Pin configuration when using CAN1:
 *   CAN1_RX <-> PB8
 *   CAN1_TX <-> PB9
 */
#ifdef USE_CAN1
#define CAN                 CAN1
#define GPIO_PORT_CAN_RX    GPIOB
#define GPIO_PIN_CAN_RX     GPIO8
#define GPIO_PORT_CAN_TX    GPIOB
#define GPIO_PIN_CAN_TX     GPIO9
#define GPIO_AF_CAN         GPIO_AF9
#endif

/*
 * Pin configuration when using CAN2:
 *   CAN2_RX <-> PB5
 *   CAN2_TX <-> PB13
 *
 * This pin configuration is compatible with ST's bootloader,
 * which is present in the microcontroller's ROM.
 */
#ifdef USE_CAN2
#define CAN                 CAN2
#define GPIO_PORT_CAN_RX    GPIOB
#define GPIO_PIN_CAN_RX     GPIO5
#define GPIO_PORT_CAN_TX    GPIOB
#define GPIO_PIN_CAN_TX     GPIO13
#define GPIO_AF_CAN         GPIO_AF9
#endif

/*
 * Many CAN transceivers have an enable input,
 * which needs to be driven HIGH or LOW in order
 * for the transceiver to become operational.
 * Define USE_CAN_ENABLE to set the pin
 * defined below to HIGH upon startup.
 * Additionally define CAN_ENABLE_INVERTED
 * to drive it LOW instead.
 */
//#define USE_CAN_ENABLE
//#define CAN_ENABLE_INVERTED

/*
 * Pin configuration for the CAN_ENABLE signal:
 *    CAN_ENABLE <-> PA8
 */
#ifdef USE_CAN_ENABLE
#define GPIO_PORT_CAN_ENABLE    GPIOA
#define GPIO_PIN_CAN_ENABLE     GPIO8
#endif

// Import symbols from linker script
extern uint8_t config_page_buffer[CONFIG_PAGE_SIZE];
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


#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_H */
