
#include <led.h>
#include <platform.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>


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
