/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

	 .equ USER_CONTEXT,                                                 0x1
	 .equ SUPERVISOR_CONTEXT,                                           0x4
	 .equ IRQ_CONTEXT,                                                  0x8

/* exported global constant and functions */
    .global delay_us
    .global delay_ms
    .global get_context
    .global do_sys_call

/* imported global constant and functions */
#    .extern _core_cycles_us
#    .extern _core_cycles_ms
	 .equ _core_cycles_us,		0
	 .equ _core_cycles_ms,		0

    .section    .text, "ax"
    .syntax unified
    .cpu cortex-m3
    .thumb

/*
    void delay_us(unsinged int us)
 */

    .thumb_func
delay_us:
    ldr    r1, =_core_cycles_us
    ldr    r1, [r1]
    mul    r1, r1, r0
    @2 cycles - calling and return, 4 instructions
    mov    r0, #6
us_loop:
    add    r1, r1, #3
    cmp    r1, r0
    blo    us_loop

    bx        lr

    .thumb_func
delay_ms:
    ldr    r2, =_core_cycles_ms
    ldr    r2, [r2]
    mov    r1, #7

ms_loop:
    nop
    add    r1, r1, #4
    cmp    r1, r2
    blo    ms_loop

    sub    r0, r0, #1
    mov    r1, #4
    cmp    r0, #0
    bne    ms_loop

    bx        lr



/*
        CONTEXT get_context();
 */

    .thumb_func
get_context:
    mrs    r0, ipsr
	 cmp    r0, #14
	 ble	  less_then_irq
	 mov    r0, #IRQ_CONTEXT
	 bx     lr
less_then_irq:
	 cmp	  r0, #0
	 beq	  no_irq
svc_context:
	 mov    r0, #SUPERVISOR_CONTEXT
	 bx     lr
no_irq:
    mrs    r0, control
	 /* ignore FPU bit */
	 and	  r0, r0, #3
    cmp    r0, #3
	 bne    svc_context
    mov    r0, #USER_CONTEXT
	 bx     lr

/*
    extern unsigned int do_sys_call(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3);
 */

    .thumb_func
do_sys_call:
	 svc		0x12
	 bx      lr
