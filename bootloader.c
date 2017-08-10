#include <stdint.h>
#include <string.h>

#include <platform.h>
#include "command.h"
#include "can_datagram.h"
#include "config.h"
#include "boot_arg.h"
#include "timeout.h"
#include "can_interface.h"

#define BUFFER_SIZE         FLASH_PAGE_SIZE + 128
#define DEFAULT_ID          0x01
#define CAN_SEND_RETRIES    100
#define CAN_RECEIVE_TIMEOUT 1000

uint8_t output_buf[BUFFER_SIZE];
uint8_t data_buf[BUFFER_SIZE];
uint8_t addr_buf[128];


/**
 * List of commands supported by this firmware
 * defined in command.c
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
    bool timeout_active = !(arg == BOOT_ARG_START_BOOTLOADER_NO_TIMEOUT);

    bootloader_config_t config;
    if (config_is_valid(memory_get_config1_addr(), CONFIG_PAGE_SIZE)) {
        config = config_read(memory_get_config1_addr(), CONFIG_PAGE_SIZE);
    } else if (config_is_valid(memory_get_config2_addr(), CONFIG_PAGE_SIZE)) {
        config = config_read(memory_get_config2_addr(), CONFIG_PAGE_SIZE);
    } else {
        // exact behaviour at invalid config is not yet defined.
        strcpy(config.device_class, PLATFORM_DEVICE_CLASS);
        strcpy(config.board_name, "foobar2000");
        config.ID = DEFAULT_ID;
        config.application_crc = 0xDEADC0DE;
        config.application_size = 0;
        config.update_count = 1;
    }

    can_datagram_t dt;
    can_datagram_init(&dt);
    can_datagram_set_address_buffer(&dt, addr_buf);
    can_datagram_set_data_buffer(&dt, data_buf, sizeof(data_buf));
    can_datagram_start(&dt);

    uint8_t data[8];
    uint32_t id;
    uint8_t data_length;
    uint8_t reply_length;

    /*
     * Remain in the bootloader until timeout is reached or respectively
     * until a jump to the main application is requested via the appropriate CAN command.
     */
    while (true) {
        // Until flash writes are working with the STM32F334R8, do not timeout / jump to main application
//        if (timeout_active && timeout_reached()) {
//            command_jump_to_application(0, NULL, NULL, &config);
//        }

        // TODO: Conserve energy by sleeping until a CAN frame is received; requires CAN interrupts to be enabled
//        asm("wfi");

        if (!can_interface_read_message(&id, data, &data_length, CAN_RECEIVE_TIMEOUT)) {
            continue;
        }

        // Begin a new, empty datagram
        if ((id & ID_START_MASK) != 0) {
            can_datagram_start(&dt);
        }

        // Append frame bytes to current datagram
        int i;
        for (i = 0; i < data_length; i++) {
            can_datagram_input_byte(&dt, data[i]);
        }

        if (can_datagram_is_complete(&dt)) {
            if (can_datagram_is_valid(&dt)) {
                timeout_active = false;

                // Check, if this nodes's ID is amongst the datagram's target IDs
                bool addressed = false;
                int i;
                for (i = 0; i < dt.destination_nodes_len; i++) {
                    if (dt.destination_nodes[i] == config.ID) {
                        timeout_active = false;
                        addressed = true;
                        break;
                    }
                }

                if (addressed) {
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
