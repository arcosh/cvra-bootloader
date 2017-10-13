
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/can.h>

#include <bootloader.h>
#include <boot_arg.h>
#include <platform/mcu/armv7-m/timeout_timer.h>
#include "platform.h"


// page buffer used by config commands.
uint8_t config_page_buffer[CONFIG_PAGE_SIZE];


void led_init()
{
    #if (GPIO_PORT_LED2 == GPIOA)
    rcc_periph_clock_enable(RCC_GPIOA);
    #endif
    #if (GPIO_PORT_LED2 == GPIOB)
    rcc_periph_clock_enable(RCC_GPIOB);
    #endif

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
    gpio_set(
            GPIO_PORT_LED2,
            GPIO_PIN_LED2
            );
}

void led_blink()
{
    // Blink onboard led
    for (uint32_t i=0;i<3;i++)
    {
        for (uint32_t j=0;j<150000;j++)
        {
            asm("nop");
        }
        gpio_toggle(GPIO_PORT_LED2, GPIO_PIN_LED2);
    }
}

void can_interface_init(void)
{
    // Enable clock to GPIO port A and B
    #if (GPIO_PORT_CAN_TX == GPIOA) || (GPIO_PORT_CAN_RX == GPIOA) || (GPIO_PORT_CAN_ENABLE == GPIOA)
    rcc_periph_clock_enable(RCC_GPIOA);
    #endif
    #if (GPIO_PORT_CAN_TX == GPIOB) || (GPIO_PORT_CAN_RX == GPIOB) || (GPIO_PORT_CAN_ENABLE == GPIOB)
    rcc_periph_clock_enable(RCC_GPIOB);
    #endif

    /*
     * In microcontrollers with dual-bxCAN (master+slave) configurations
     * it is obligatory to enable the master CAN's clock even if the master CAN isn't used,
     * because the master is managing the slave's access to the dual-bxCAN's SRAM.
     */
    rcc_periph_clock_enable(RCC_CAN1);
#ifdef USE_CAN2
    rcc_periph_clock_enable(RCC_CAN2);
#endif

    // Setup CAN pins
    gpio_mode_setup(
            GPIO_PORT_CAN_TX,
            GPIO_MODE_AF,
            GPIO_PUPD_PULLUP,
            GPIO_PIN_CAN_TX
            );
    gpio_set_af(
            GPIO_PORT_CAN_TX,
            GPIO_AF_CAN,
            GPIO_PIN_CAN_TX
            );

    gpio_mode_setup(
            GPIO_PORT_CAN_RX,
            GPIO_MODE_AF,
            GPIO_PUPD_PULLUP,
            GPIO_PIN_CAN_RX
            );
    gpio_set_af(
            GPIO_PORT_CAN_RX,
            GPIO_AF_CAN,
            GPIO_PIN_CAN_RX
            );

    #ifdef USE_CAN_ENABLE
    gpio_mode_setup(
            GPIO_PORT_CAN_ENABLE,
            GPIO_MODE_OUTPUT,
            GPIO_PUPD_NONE,
            GPIO_PIN_CAN_ENABLE
            );
    gpio_set_output_options(
            GPIO_PORT_CAN_ENABLE,
            GPIO_OTYPE_PP,
            GPIO_OSPEED_2MHZ,
            GPIO_PIN_CAN_ENABLE
            );
    #ifndef CAN_ENABLE_INVERTED
    gpio_set(
            GPIO_PORT_CAN_ENABLE,
            GPIO_PIN_CAN_ENABLE
            );
    #else
    gpio_clear(
            GPIO_PORT_CAN_ENABLE,
            GPIO_PIN_CAN_ENABLE
            );
    #endif // CAN_ENABLE_INVERTED
    #endif // USE_CAN_ENABLE

    /*
    CAN1 and CAN2 are running from APB1 clock (18MHz).
    Therefore 500kbps can be achieved with prescaler=2 like this:
    18MHz / 2 = 9MHz
    9MHz / (1tq + 10tq + 7tq) = 500kHz => 500kbit
    */
    can_init(CAN,            // Interface
             false,           // Time triggered communication mode.
             true,            // Automatic bus-off management.
             false,           // Automatic wakeup mode.
             false,           // No automatic retransmission.
             false,           // Receive FIFO locked mode.
             true,            // Transmit FIFO priority.
             CAN_BTR_SJW_1TQ, // Resynchronization time quanta jump width
             CAN_BTR_TS1_10TQ,// Time segment 1 time quanta width
             CAN_BTR_TS2_7TQ, // Time segment 2 time quanta width
             1,               // Prescaler
             false,           // Loopback
             false);          // Silent

    // filter to match any standard id
    // mask bits: 0 = Don't care, 1 = mute match corresponding id bit
    can_filter_id_mask_32bit_init(
        CAN,
        0,      // filter nr
        0,      // id: only std id, no rtr
        6 | (7<<29), // mask: match only std id[10:8] = 0 (bootloader frames)
        0,      // assign to fifo0
        true    // enable
    );

//    nvic_enable_irq(NVIC_IRQ_CAN);
}

void fault_handler(void)
{
    // while(1); // debug
    reboot_system(BOOT_ARG_START_BOOTLOADER_NO_TIMEOUT);
}

/**
 * Configure the processor to run at 36MHz from the internal resonator
 */
void rcc_clock_setup_in_hsi_out_36mhz(void)
{
    // Enable internal high-speed resonator (16 MHz)
    rcc_osc_on(HSI);
    // Wait for internal resonator to become ready
    rcc_wait_for_osc_ready(HSI);
    // Configure AHB, APB1 and APB2 prescalers
    rcc_set_hpre(2);
    rcc_set_ppre1(4);
    rcc_set_ppre2(2);
    // Configure PLL source multiplexer to use internal resonator
    rcc_set_pll_source(HSI);
    // Configure PLL to scale input to 72 MHz
    rcc_set_main_pll_hsi(8, 72, 2, 2);
    // Set system clock source to PLL
    rcc_set_sysclk_source(PLL);
}

void platform_main(int arg)
{
    // Run from internal RC oscillator
    rcc_clock_setup_in_hsi_out_36mhz();

    // Initialize the on-board LED
    led_init();

    // Blink on-board LED to indicate platform startup
    led_blink();

    // Configure timeout of 10000 milliseconds (assuming 36 Mhz system clock, see above)
    timeout_timer_init(36000000, 10000);
    can_interface_init();
    bootloader_main(arg);

    // We only arrive here, if something went wrong. Upon next reboot directly boot into the bootloader.
    reboot_system(BOOT_ARG_START_BOOTLOADER);
}
