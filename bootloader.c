
#include <bootloader.h>

#include <stdint.h>
#include <string.h>

#include <led.h>
#include <timeout_timer.h>

#include "error.h"
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


/**
 * Send response datagram to bootloader client
 */
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

        #ifdef CAN_INTER_FRAME_DELAY
        // Artificial delay allowing slow communication partners to keep up
        uint32_t start = get_time();
        while ((get_time() - start) < CAN_INTER_FRAME_DELAY)
        {
            #ifdef BOOTLOADER_SLEEP_UNTIL_INTERRUPT
            asm("wfi");
            #endif
        }
        #endif

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


/**
 * Send complaint datagram to bootloader client about malformed received datagram
 */
static void return_error_datagram(uint8_t source_id, uint8_t* output_buf, uint8_t error_code)
{
    cmp_mem_access_t out_cma;
    cmp_ctx_t out_writer;

    // Prepare output buffer for reponse
    cmp_mem_access_init(&out_writer, &out_cma, output_buf, OUTPUT_BUFFER_SIZE);

    // Return success (boolean): false
    cmp_write_uint(&out_writer, error_code);

    // Get size written to output buffer
    int reply_length = cmp_mem_access_get_pos(&out_cma);

    // Send our reply via CAN
    return_datagram(
            source_id & (~ID_START_MASK),
            0,
            output_buf,
            (size_t) reply_length
            );
}


void bootloader_main(int arg)
{
    /**
     * Switch determining whether the bootloader should
     * automatically proceed with starting the app after a timeout
     * or remain listening for commands on the CAN bus forever
     */
    volatile bool bootloader_timeout_enabled;
    #ifdef BOOTLOADER_TIMEOUT_DISABLE
    bootloader_timeout_enabled = false;
    #else
    bootloader_timeout_enabled = !(arg == BOOT_ARG_START_BOOTLOADER_NO_TIMEOUT);
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
     * Switch to enable/disable datagram timeout timer
     * This timeout is running, as soon as a start frame was received.
     */
    bool datagram_timeout_running = false;

    can_datagram_init(&dt);
    can_datagram_set_address_buffer(&dt, addr_buf);
    can_datagram_set_data_buffer(&dt, data_buf, INPUT_BUFFER_SIZE);
    can_datagram_start(&dt);

    /**
     * Configure CAN peripheral to receive only broadcast frames
     * and frames addressed specifically to this device
     */
    can_set_filters(config.ID);

    /*
     * Start the timeout timer for the bootloader to jump to the application
     */
    bootloader_timeout_start();

    /*
     * Remain in the bootloader until either timeout is reached or a jump
     * to the main application is requested via the appropriate CAN command.
     */
    while (true) {
        if (bootloader_timeout_enabled && bootloader_timeout_reached()) {
            command_jump_to_application(0, NULL, NULL, &config);
        }

        if (datagram_timeout_running && datagram_timeout_reached()) {
            // Inform client about timeout
            return_error_datagram(config.ID, output_buf, ERROR_DATAGRAM_TIMEOUT);
            set_status(ERROR_DATAGRAM_TIMEOUT);
            // Begin new empty datagram in order to avoid possible datagram duplication
            can_datagram_start(&dt);
            // Stop timeout timer; will be started by next datagram start frame
            datagram_timeout_running = false;

            led_on(LED_ERROR);
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

        if (id != ID_START_MASK
         && id != 0x000
         && id != (config.ID | ID_START_MASK)
         && id != config.ID) {
            // The frame is not a bootloader broadcast, nor is it addressed to this device's ID.
            continue;
        }

        // This frame was for us, so the current datagram didn't time out
        datagram_timeout_reset();

        // Datagram start frame received: Begin a new, empty reception datagram
        if ((id & ID_START_MASK) != 0) {
            can_datagram_start(&dt);
            datagram_timeout_running = true;
            set_status(ERROR_UNSPECIFIED);
        }

        // Append frame bytes to current reception datagram
        for (uint8_t i = 0; i < data_length; i++) {
            can_datagram_input_byte(&dt, data[i]);
        }

        // Frames with fewer than 8 bytes can only mean end of datagram
        if (can_datagram_is_complete(&dt)
         || (data_length < 8)) {
            if (can_datagram_is_valid(&dt)) {

                // Stop waiting for more frames to this datagram
                datagram_timeout_running = false;
                set_status(SUCCESS);

                // Check, if this nodes's ID is amongst the datagram's target IDs
                bool addressed = false;
                int i;
                for (i = 0; i < dt.destination_nodes_len; i++) {
                    if (dt.destination_nodes[i] == config.ID) {
                        addressed = true;
                        break;
                    }
                }

                if (addressed) {
                    // Disable bootloader timeout
                    bootloader_timeout_enabled = false;

                    // we were addressed
                    reply_length = execute_datagram_commands(
                            (char*) dt.data,
                            dt.data_len,
                            &commands[0],
                            sizeof(commands)/sizeof(command_t),
                            (char*) output_buf,
                            OUTPUT_BUFFER_SIZE,
                            &config
                            );

                    if (reply_length > 0) {
                        // The reply's CAN frame ID must not occupy start mask bits.
                        uint8_t return_id = id & ~ID_START_MASK;

                        // Send the reply as generated by the corresponding command function
                        return_datagram(
                                config.ID,
                                return_id,
                                output_buf,
                                (size_t) reply_length
                                );
                        set_status(SUCCESS);
                        led_on(LED_SUCCESS);

                    } else {
                        // A negative return value represents an error code.
                        return_error_datagram(
                                config.ID,
                                output_buf,
                                (-reply_length)
                                );
                        set_status(-reply_length);
                        led_on(LED_ERROR);
                    }
                }
            } else {
                // The received datagram could not be decoded.
                return_error_datagram(
                        config.ID,
                        output_buf,
                        ERROR_CORRUPT_DATAGRAM
                        );
                set_status(ERROR_CORRUPT_DATAGRAM);
                led_on(LED_ERROR);
            }

            // Begin a new datagram, regardsless of whether the current datagram was valid or not
            can_datagram_start(&dt);
            // Stop timeout timer; will be started by next datagram start frame
            datagram_timeout_running = false;
        }
    }
}
