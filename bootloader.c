
#include <bootloader.h>

#include <stdint.h>
#include <string.h>

#include <led.h>

#include "command.h"
#include "can_datagram.h"
#include "config.h"
#include "boot_arg.h"
#include "timeout.h"
#include "can_interface.h"

#include <cmp_mem_access/cmp_mem_access.h>


/**
 * List of commands supported by this firmware
 * as defined in command.c
 */
extern command_t commands[COMMAND_COUNT];


static void return_datagram(uint8_t source_id, uint8_t dest_id, uint8_t *data, size_t len)
{
    can_datagram_t dt;
    uint8_t dest_nodes[1];
    uint8_t buf[8];

    can_datagram_init(&dt);
    can_datagram_set_address_buffer(&dt, dest_nodes);
    can_datagram_set_data_buffer(&dt, data, len);
    dt.destination_nodes[0] = dest_id;
    dt.destination_nodes_len = 1;
    dt.data_len = len;
    dt.crc = can_datagram_compute_crc(&dt);

    bool start_of_datagram = true;

    // prevent infinite loop, e.g. when the datagram is broken
    uint8_t transmission_count = 100;
    while (transmission_count-- > 0) {
        uint8_t dlc = can_datagram_output_bytes(&dt, (char *)buf, sizeof(buf));

        if (dlc == 0) {
            break;
        }

        if (start_of_datagram) {
            if (!can_interface_send_message(source_id | ID_START_MASK, buf, dlc,
                                            CAN_SEND_RETRIES)) {
                break;  // failed
            }
            start_of_datagram = false;
        } else {
            if (!can_interface_send_message(source_id, buf, dlc, CAN_SEND_RETRIES)) {
                break;  // failed
            }
        }
    }
}


void bootloader_main(int arg)
{
    /**
     * Switch determining whether the bootloader should
     * automatically proceed with starting the app after a timeout
     * or remain listening for commands on the CAN bus forever
     */
    bool timeout_enabled;
    #ifdef BOOTLOADER_TIMEOUT_DISABLE
    timeout_enabled = false;
    #else
    timeout_enabled = !(arg == BOOT_ARG_START_BOOTLOADER_NO_TIMEOUT);
    #endif

    if (arg == BOOT_ARG_START_BOOTLOADER_NO_TIMEOUT)
    {
        /*
         * The bootloader was started with the don't-timeout-argument.
         * In the typical scenario this means the payload application CRC check failed.
         */
        led_on(LED_ERROR);
    }

    /**
     * Struct holding the bootloader's configuration
     */
    bootloader_config_t config;
    if (config_is_valid(memory_get_config1_addr(), CONFIG_PAGE_SIZE)) {
        config = config_read(memory_get_config1_addr(), CONFIG_PAGE_SIZE);
    } else if (config_is_valid(memory_get_config2_addr(), CONFIG_PAGE_SIZE)) {
        config = config_read(memory_get_config2_addr(), CONFIG_PAGE_SIZE);
    } else {
        // exact behaviour at invalid config is not yet defined.
        strcpy(config.device_class, PLATFORM_DEVICE_CLASS);
        strcpy(config.board_name, PLATFORM_DEFAULT_NAME);
        config.ID = PLATFORM_DEFAULT_ID;
        config.application_crc = 0xDEADC0DE;
        config.application_size = 0;
        config.update_count = 1;
    }

    /**
     * Struct to store the properties of an incoming datagram
     */
    can_datagram_t dt;
    /**
     * Buffer storing the list of destination nodes of a datagram
     */
    uint8_t addr_buf[128];
    /**
     * Buffer to store an incoming datagram
     */
    uint8_t data_buf[INPUT_BUFFER_SIZE];
    /**
     * Buffer to store the (at max.) 8 data bytes of the received CAN frame
     */
    uint8_t data[8];
    /**
     * Number of actually received data bytes in this CAN frame
     */
    uint8_t data_length;
    /**
     * Destination ID of the received CAN frame
     */
    uint32_t id;
    /**
     * Buffer for the construction of the response datagram
     */
    uint8_t output_buf[OUTPUT_BUFFER_SIZE];
    /**
     * Size of the response datagram in bytes
     */
    uint8_t reply_length;
    /**
     * Flag to indicate, whether the previously received datagram was received completely and intact
     */
    bool previous_datagram_was_valid = true;

    can_datagram_init(&dt);
    can_datagram_set_address_buffer(&dt, addr_buf);
    can_datagram_set_data_buffer(&dt, data_buf, sizeof(data_buf));
    can_datagram_start(&dt);

    /*
     * Remain in the bootloader until either timeout is reached or a jump
     * to the main application is requested via the appropriate CAN command.
     */
    while (true) {
        if (timeout_enabled && timeout_reached()) {
            command_jump_to_application(0, NULL, NULL, &config);
        }

        #ifdef BOOTLOADER_SLEEP_UNTIL_INTERRUPT
        /*
         * Conserve energy by putting the processor to sleep until a CAN frame is received
         *
         * Note:
         *  - Requires CAN RX0 (reception in FIFO0) interrupt to be enabled
         *  - Might interfer with the above timeout functionality,
         *    except if the timeout is realized via a timer peripheral i.e. interrupt
         */
        asm("wfi");
        #endif

        // Poll CAN reception FIFO for incoming frames
        if (!can_interface_read_message(&id, data, &data_length, CAN_RECEIVE_TIMEOUT)) {
            // No frames were received
            continue;
        }

        // Begin constructing a new, empty reception datagram
        if ((id & ID_START_MASK) != 0) {
            if (previous_datagram_was_valid)
            {
                led_blink(LED_SUCCESS);
            }
            else
            {
                led_blink(LED_ERROR);
            }

            can_datagram_start(&dt);
            previous_datagram_was_valid = false;
        }

        // Append frame bytes to current reception datagram
        for (uint8_t i = 0; i < data_length; i++) {
            can_datagram_input_byte(&dt, data[i]);
        }

        if (can_datagram_is_complete(&dt)) {
            if (can_datagram_is_valid(&dt)) {

                // Don't flash the red LED upon beginning of next datagram
                previous_datagram_was_valid = true;

                // Check, if this nodes's ID is amongst the datagram's target IDs
                bool addressed = false;
                int i;
                for (i = 0; i < dt.destination_nodes_len; i++) {
                    if (dt.destination_nodes[i] == config.ID) {
                        // Disable bootloader timeout
                        timeout_enabled = false;
                        addressed = true;
                        break;
                    }
                }

                if (addressed) {
                    led_blink(LED_SUCCESS);

                    // we were addressed
                    reply_length = execute_datagram_commands(
                            (char*) dt.data,
                            dt.data_len,
                            &commands[0],
                            sizeof(commands)/sizeof(command_t),
                            (char*) output_buf,
                            sizeof(output_buf),
                            &config
                            );

                    if (reply_length > 0) {
                        // The reply's CAN frame ID must not occupy start mask bits.
                        uint8_t return_id = id & ~ID_START_MASK;

                        // Send our reply via CAN
                        return_datagram(
                                config.ID,
                                return_id,
                                output_buf,
                                (size_t) reply_length
                                );
                    }
                }
            }

            // Begin a new datagram, regardsless of whether the current datagram was valid or not
            can_datagram_start(&dt);
        }
    }
}
