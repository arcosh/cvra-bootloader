#ifndef TIMEOUT_TIMER_H
#define TIMEOUT_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>


/** Configures the timeout hardware timer.
 * @param [in] f_cpu                The CPU frequency, with which the timer runs.
 * @param [in] bootloader_timeout   Number of milliseconds for the bootloader to wait for invokation
 * @param [in] datagram_timeout     Number of milliseconds for the bootloader to wait for a datagram to complete
 */
void timer_init(uint32_t f_cpu, uint32_t bootloader_timeout, uint32_t datagram_timeout);

/**
 * Returns the number of milliseconds elapsed since platform startup
 */
uint32_t get_time();

/**
 * Remember the current time in milliseconds
 */
void bootloader_timeout_start();

/**
 * Determines, whether the configured bootloader timeout has been reached
 */
bool bootloader_timeout_reached();

/**
 * Remember the current time in milliseconds
 */
void datagram_timeout_reset();

/**
 * Determines, whether the datagram timeout has elapsed or not
 */
bool datagram_timeout_reached();


#ifdef __cplusplus
}
#endif

#endif /* TIMEOUT_TIMER_H */
