
#include <led.h>
#include <platform.h>
#include <timeout_timer.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>


#ifdef LED1
// Must be some high number, so that led_on() at time==0 is possible, see function led_on().
static uint32_t led1_start_ms = 0xFFFF;
#endif

#ifdef LED2
static uint32_t led2_start_ms = 0xFFFF;
#endif


void led_init()
{
    #if (defined(LED1) && (GPIO_PORT_LED1 == GPIOA)) || (defined(LED2) && (GPIO_PORT_LED2 == GPIOA))
    rcc_periph_clock_enable(RCC_GPIOA);
    #endif
    #if (defined(LED1) && (GPIO_PORT_LED1 == GPIOB)) || (defined(LED2) && (GPIO_PORT_LED2 == GPIOB))
    rcc_periph_clock_enable(RCC_GPIOB);
    #endif
    #if (defined(LED1) && (GPIO_PORT_LED1 == GPIOC)) || (defined(LED2) && (GPIO_PORT_LED2 == GPIOC))
    rcc_periph_clock_enable(RCC_GPIOC);
    #endif
    #if (defined(LED1) && (GPIO_PORT_LED1 == GPIOD)) || (defined(LED2) && (GPIO_PORT_LED2 == GPIOD))
    rcc_periph_clock_enable(RCC_GPIOD);
    #endif

    #ifdef LED1
    gpio_mode_setup(
            GPIO_PORT_LED1,
            GPIO_MODE_OUTPUT,
            GPIO_PUPD_NONE,
            GPIO_PIN_LED1
            );
    gpio_set_output_options(
            GPIO_PORT_LED1,
            GPIO_OTYPE_PP,
            GPIO_OSPEED_2MHZ,
            GPIO_PIN_LED1
            );
    gpio_clear(
            GPIO_PORT_LED1,
            GPIO_PIN_LED1
            );
    #endif

    #ifdef LED2
    gpio_mode_setup(
            GPIO_PORT_LED2,
            GPIO_MODE_OUTPUT,
            GPIO_PUPD_NONE,
            GPIO_PIN_LED2
            );
    gpio_set_output_options(
            GPIO_PORT_LED2,
            GPIO_OTYPE_PP,
            GPIO_OSPEED_2MHZ,
            GPIO_PIN_LED2
            );
    gpio_clear(
            GPIO_PORT_LED2,
            GPIO_PIN_LED2
            );
    #endif
}


void led_blink(uint8_t led_index)
{
    #ifdef LED1
    if (led_index == 1)
    {
        // Blink onboard led
        gpio_clear(GPIO_PORT_LED1, GPIO_PIN_LED1);
        for (uint32_t i=0;i<2;i++)
        {
            for (uint32_t j=0;j<200000;j++)
            {
                asm("nop");
            }
            gpio_toggle(GPIO_PORT_LED1, GPIO_PIN_LED1);
        }
    }
    #endif

    #ifdef LED2
    if (led_index == 2)
    {
        // Blink onboard led
        gpio_clear(GPIO_PORT_LED2, GPIO_PIN_LED2);
        for (uint32_t i=0;i<2;i++)
        {
            for (uint32_t j=0;j<200000;j++)
            {
                asm("nop");
            }
            gpio_toggle(GPIO_PORT_LED2, GPIO_PIN_LED2);
        }
    }
    #endif
}


void led_on(uint8_t led_index)
{
    #ifdef LED1
    if ((led_index == 1)
     // Don't restart LED timeout, if LED is already on.
     && (!gpio_get(GPIO_PORT_LED1, GPIO_PIN_LED1))
     // Only re-enable LED, if it has been off for a while since last turn-on
     && ((get_time() - led1_start_ms) >= 2*LED_ON_DURATION_MS))
    {
        gpio_set(GPIO_PORT_LED1, GPIO_PIN_LED1);
        led1_start_ms = get_time();
    }
    #endif

    #ifdef LED2
    if ((led_index == 2)
     && (!gpio_get(GPIO_PORT_LED2, GPIO_PIN_LED2))
     && ((get_time() - led2_start_ms) >= 2*LED_ON_DURATION_MS))
    {
        gpio_set(GPIO_PORT_LED2, GPIO_PIN_LED2);
        led2_start_ms = get_time();
    }
    #endif
}


void led_process(uint32_t time)
{
    #ifdef LED1
    if ((time - led1_start_ms) >= LED_ON_DURATION_MS)
    {
        gpio_clear(GPIO_PORT_LED1, GPIO_PIN_LED1);
    }
    #endif

    #ifdef LED2
    if ((time - led2_start_ms) >= LED_ON_DURATION_MS)
    {
        gpio_clear(GPIO_PORT_LED2, GPIO_PIN_LED2);
    }
    #endif
}
