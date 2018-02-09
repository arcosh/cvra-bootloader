
#ifndef PLATFORM_H
#define PLATFORM_H

#define PLATFORM_DEFAULT_NAME   "dummy"
#define PLATFORM_DEFAULT_ID     1
#define PLATFORM_DEVICE_CLASS   "Dummy"

#define CONFIG_PAGE_SIZE        512

#define FLASH_ERASE_RETRIES     1

#define ADDRESS_BOUNDARY_CHECK_DISABLED

#define CAN_SEND_RETRIES        1
#define CAN_RECEIVE_TIMEOUT     10

/**
 * The incoming datagram buffer must support largest flash sector size + datagram overhead.
 */
#define INPUT_BUFFER_SIZE       32768

/**
 * The outgoing datagram buffer must support config page size + datagram overhead.
 */
#define OUTPUT_BUFFER_SIZE      8192

#define LED_SUCCESS     1
#define LED_ERROR       2
#define led_on(x)
#define led_off(x)

#endif
