@ boot code for bootloader
@ This is the first code executed after reset. It checks the start of RAM for a
@ 7 byte magic value followed a 1 byte argument.
@ boot arguments:
@   0 : bootloader, with timeout (default)
@   1 : bootloader, without timeout
@   2 : application, RAM content is not altered
@   3 : internal ST bootloader from system memeory
@
@ This is has several purposes:
@ - Start the bootloader with an argument (such as disable the timeout)
@ - Start the application from the bootloader itself without having to manually
@   reset every peripheral that was used.
@ - The application can omit the bootloader after a software reset to preserve
@   the RAM contents (with exception to the first two words).
@ - Start the internal bootloader from ST to flash via UART/CAN/USB_DFU...

.extern bootloader_startup
.extern application_address

@
@ The RAM address is defined in the platform's linker file.
@
.extern ram_begin

@
@ Control register for vector table relocation
@
@ Note: SCB_VTOR could also be included from libopencm3/include/libopencm3/cm3/scb.h,
@       although it apparently has the same address on all Cortex MCUs
@       (which possess a vector table offset register).
@
.equ SCB_VTOR, 0xE000ED08

@
@ Import magic words from boot_arg.c
@
.extern boot_arg_magic_value_lo
.extern boot_arg_magic_value_hi


.thumb
.thumb_func
.syntax unified
.section .text
.global reset_handler
reset_handler:
    ldr     r5, =ram_begin
    ldr     r1, [r5, #0]    @ load magic value from RAM
    ldr     r2, [r5, #4]
    ldr     r4, =0x00FFFFFF
    and     r2, r2, r4

    eor     r0, r0          @ clear argument register

    @ verify magic values
    ldr     r4, =boot_arg_magic_value_lo
    ldr     r3, [r4]
    cmp     r1, r3
    bne     bootloader_startup
    ldr     r4, =boot_arg_magic_value_hi
    ldr     r3, [r4]
    cmp     r2, r3
    bne     bootloader_startup

    @ magic value was correct
    ldrb    r0, [r5, #7]    @ load argument byte

    eor     r1, r1
    str     r1, [r5, #0]    @ clear ram_begin
    str     r1, [r5, #4]

    cmp     r0, #2
    beq     _app_jmp
    cmp     r0, #3
    beq     _st_bootloader

    @ default: launch bootloader with argument 0
    b       bootloader_startup

_app_jmp:
    ldr     r0, =application_address
    ldr     r1, =SCB_VTOR
    str     r0, [r1]        @ relocate vector table
    dsb
    ldr     sp, [r0, #0]    @ initialize stack pointer
    ldr     r0, [r0, #4]    @ load reset address
    bx      r0              @ jump to application

_st_bootloader:
    ldr     r0, =0x1FFF0000
    ldr     sp, [r0, #0]
    ldr     r0, [r0, #4]
    bx      r0
