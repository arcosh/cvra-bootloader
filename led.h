/**
 * Function declarations for LED control
 *
 * Up to 2 LEDs can be enabled and configured in the respective platform.h
 */

#ifndef LED_H
#define LED_H


#include <stdint.h>


/**
 * Initialize the GPIO pins of all configured LEDs of a platform
 */
void led_init();


/**
 * Blink the corresponding LED
 *
 * Doesn't do anything, if the corresponding LED is not configured on a platform.
 */
void led_blink(uint8_t led_index);


/**
 * Switches the LED on and remembers the time.
 *
 * Doesn't do anything, if the corresponding LED is not configured on a platform.
 */
void led_on(uint8_t led_index);


/**
 * Checks, if an LED's on time has elapsed
 * and if so turn off the corresponding LEDs.
 *
 * Doesn't do anything, if the corresponding LED is not configured on a platform.
 */
void led_process(uint32_t time);


#endif
