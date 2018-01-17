/**
 * This file contains customizable switches and parameters
 * for the STM32F446RE platform
 */

#ifndef PLATFORM_CONFIG_H
#define PLATFORM_CONFIG_H

/**
 * Generate the default board name from the MCU's serial number
 *
 * This feature is currently not enabled, because the resulting
 * bootloader image does not fit into flash anymore.
 */
//#define PLATFORM_ENABLE_UUID

/**
 * Number of milliseconds to remain in the bootloader
 * waiting for a CAN frame before booting to the payload application
 */
#define BOOTLOADER_TIMEOUT      4000

/**
 * Prevents bootloader from booting the app after a timeout occured;
 * instead wait for CAN input forever and only boot the app,
 * when being instructed to do so via the appropriate CAN command.
 */
//#define BOOTLOADER_TIMEOUT_DISABLE

/**
 * The incoming datagram buffer must support largest flash sector size + datagram overhead.
 */
#define INPUT_BUFFER_SIZE       32768

/**
 * The outgoing datagram buffer must support config page size + datagram overhead.
 */
#define OUTPUT_BUFFER_SIZE      8192

/**
 * Number of milliseconds to wait for a datagram to complete
 * before sending an error reply
 */
#define DATAGRAM_TIMEOUT        500

/**
 * Number of milliseconds to wait between CAN frame transmissions
 */
#define CAN_INTER_FRAME_DELAY   1

/**
 * Boots the application image (upon bootloader timeout or CAN command)
 * regardless of whether the app image checksum equals the configured checksum
 */
//#define COMMAND_JUMP_DISABLE_CRC_CHECKING

/**
 * Conserve energy by putting the processor to sleep
 * while waiting for interrupts (CAN or timer)
 *
 * This switch may interfer with timing, see also
 *  https://electronics.stackexchange.com/questions/186409/wfi-instruction-slowing-down-systick-interrupt
 */
//#define BOOTLOADER_SLEEP_UNTIL_INTERRUPT


/**
 * Configure onboard LEDs
 */
// Use LED1 with default pin configuration
#define LED1


/**
 * Configure which CAN peripheral to use
 */
#define USE_CAN2
//#define CAN_USE_PINS_PB5_PB6
//#define CAN_USE_PINS_PB12_PB13
#define CAN_USE_PINS_ST_BOOTLOADER

/**
 * Use interrupts to receive CAN frames
 */
#define CAN_USE_INTERRUPTS

/**
 * Configure whether to buffer incoming frames or not
 */
#define CAN_RX_BUFFER_ENABLED

/**
 * A datagram can easily consist of 300 CAN frames and more
 */
#define CAN_FRAMES_BUFFERED     500

/**
 * Only receive frames addressed to this platform's ID
 */
#define CAN_FILTERS_ENABLED

/**
 * Many CAN transceivers have an enable input,
 * which needs to be driven HIGH or LOW in order
 * for the transceiver to become operational.
 * Define USE_CAN_ENABLE to set the pin
 * defined below to HIGH upon startup.
 */
//#define USE_CAN_ENABLE
//#define GPIO_PORT_CAN_ENABLE    GPIOA
//#define GPIO_PIN_CAN_ENABLE     GPIO8

/**
 * Additionally to USE_CAN_ENABLE
 * CAN_ENABLE_INVERTED can be defined
 * to drive the CAN enable signal as active low.
 */
//#define CAN_ENABLE_INVERTED

#endif

