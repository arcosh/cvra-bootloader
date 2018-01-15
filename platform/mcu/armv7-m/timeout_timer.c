
#include <libopencm3/stm32/f4/nvic.h>
#include <libopencm3/cm3/systick.h>
#include "timeout_timer.h"
#include <led.h>

static volatile uint32_t time_ms = 0;
static uint32_t timer_freq;
static uint32_t bootloader_timeout_start_ms;
static uint32_t bootloader_timeout_ms;
static uint32_t datagram_timeout_start_ms;
static uint32_t datagram_timeout_ms;
static uint32_t sys_ticks = 0;


void timer_init(uint32_t f_cpu, uint32_t bootloader_timeout, uint32_t datagram_timeout)
{
    // Enable systick counter
    systick_counter_enable();
    // Systick counter running at 1/8 of the system clock
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    timer_freq = f_cpu;
    // The system tick timer shall reach zero every 1ms.
    systick_set_reload(f_cpu / 1000);

    bootloader_timeout_ms = bootloader_timeout;
    datagram_timeout_ms = datagram_timeout;

    systick_interrupt_enable();
}


void systick_handler()
{
    /*
     * This will work for more than 49 days before the counter overflows:
     *      2**32 / (1000 * 60 * 60 * 24) = 49.71
     */
    time_ms++;

    led_process(time_ms);
}


uint32_t get_time()
{
    return time_ms;
}


void update_systick_counter()
{
    static uint32_t systick_previous_value = STK_RVR_RELOAD;
    uint32_t meanwhile_elapsed_ticks = systick_previous_value - systick_get_value();

    // Increase number of elapsed systicks by the number of elapsed ticks since last update
    sys_ticks += meanwhile_elapsed_ticks;

    if (systick_get_countflag())
    {
        // Counter reached zero in the meantime and has reloaded
        sys_ticks += STK_RVR_RELOAD + 1;
    }
}


void bootloader_timeout_start()
{
    bootloader_timeout_start_ms = time_ms;
}


bool bootloader_timeout_reached()
{
//    update_systick_counter();
//    return (sys_ticks / (timer_freq / 1000)) > bootloader_timeout_ms;
    return ((time_ms - bootloader_timeout_start_ms) >= bootloader_timeout_ms);
}


void datagram_timeout_reset()
{
    datagram_timeout_start_ms = time_ms;
}


bool datagram_timeout_reached()
{
//    update_systick_counter();

    // TODO: Re-enable datagram timeout, as soon as transmission generally works again
//    return ((sys_ticks - datagram_start_time) / (timer_freq / 1000)) > datagram_timeout_ms;
    return ((time_ms - datagram_timeout_start_ms) >= datagram_timeout_ms);;
}
