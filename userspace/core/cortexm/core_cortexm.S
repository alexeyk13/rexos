/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

	 .equ USER_CONTEXT,                                                 0x1
	 .equ SUPERVISOR_CONTEXT,                                           0x4
	 .equ IRQ_CONTEXT,                                                  0x8

/* exported global constant and functions */
    .global get_context
    .global do_sys_call

/* imported global constant and functions */

    .section    .text, "ax"
    .syntax unified
    .cpu cortex-m3
    .thumb

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
