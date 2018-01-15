
#include <stdint.h>
#include <platform.h>

extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _ldata;
extern uint32_t _eram;

extern void platform_main(int);
extern void fault_handler(void);
extern void systick_handler(void);
extern void reset_handler(void);
extern void can2_rx0_isr(void);

__attribute__ ((section(".vectors")))
void (*const vector_table[]) (void) = {
    (void*)&_eram,  // stack pointer
    reset_handler,
    fault_handler,  // nmi_handler
    fault_handler,  // hard_fault_handler
    fault_handler,  // mem_manage_handler
    fault_handler,  // bus_fault_handler
    fault_handler,  // usage_fault_handler
    0,              // reserved
    0,              // reserved
    0,              // reserved
    0,              // reserved
    fault_handler,  // svcall_handler
    0,              // debug_handler
    0,              // reserved
    fault_handler,  // pendsv_handler
    systick_handler,  // systick_handler

    fault_handler,  // IRQ0
    fault_handler,  // IRQ1
    fault_handler,  // IRQ2
    fault_handler,  // IRQ3
    fault_handler,  // IRQ4
    fault_handler,  // IRQ5
    fault_handler,  // IRQ6
    fault_handler,  // IRQ7
    fault_handler,  // IRQ8
    fault_handler,  // IRQ9

    fault_handler,  // IRQ10
    fault_handler,  // IRQ11
    fault_handler,  // IRQ12
    fault_handler,  // IRQ13
    fault_handler,  // IRQ14
    fault_handler,  // IRQ15
    fault_handler,  // IRQ16
    fault_handler,  // IRQ17
    fault_handler,  // IRQ18
    fault_handler,  // IRQ19

    fault_handler,  // IRQ20
    fault_handler,  // IRQ21
    fault_handler,  // IRQ22
    fault_handler,  // IRQ23
    fault_handler,  // IRQ24
    fault_handler,  // IRQ25
    fault_handler,  // IRQ26
    fault_handler,  // IRQ27
    fault_handler,  // IRQ28
    fault_handler,  // IRQ29

    fault_handler,  // IRQ30
    fault_handler,  // IRQ31
    fault_handler,  // IRQ32
    fault_handler,  // IRQ33
    fault_handler,  // IRQ34
    fault_handler,  // IRQ35
    fault_handler,  // IRQ36
    fault_handler,  // IRQ37
    fault_handler,  // IRQ38
    fault_handler,  // IRQ39

    fault_handler,  // IRQ40
    fault_handler,  // IRQ41
    fault_handler,  // IRQ42
    fault_handler,  // IRQ43
    fault_handler,  // IRQ44
    fault_handler,  // IRQ45
    fault_handler,  // IRQ46
    fault_handler,  // IRQ47
    fault_handler,  // IRQ48
    fault_handler,  // IRQ49

    fault_handler,  // IRQ50
    fault_handler,  // IRQ51
    fault_handler,  // IRQ52
    fault_handler,  // IRQ53
    fault_handler,  // IRQ54
    fault_handler,  // IRQ55
    fault_handler,  // IRQ56
    fault_handler,  // IRQ57
    fault_handler,  // IRQ58
    fault_handler,  // IRQ59

    fault_handler,  // IRQ60
    fault_handler,  // IRQ61
    fault_handler,  // IRQ62
    fault_handler,  // IRQ63
    #ifdef CAN_USE_INTERRUPTS
    can2_rx0_isr,   // IRQ64
    #else
    fault_handler,m // IRQ64
    #endif
    fault_handler,  // IRQ65
    fault_handler,  // IRQ66
    fault_handler,  // IRQ67
    fault_handler,  // IRQ68
    fault_handler,  // IRQ69
};

void __attribute__ ((naked)) bootloader_startup(int arg)
{
    volatile uint32_t *p_ram, *p_flash;
    // clear .bss section
    p_ram = &_sbss;
    while (p_ram < &_ebss) {
        *p_ram++ = 0;
    }
    // copy .data section from flash to ram
    p_flash = &_ldata;
    p_ram = &_sdata;
    while (p_ram < &_edata) {
        *p_ram++ = *p_flash++;
    }

    platform_main(arg);

    while(1);
}
