/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

//TODO!!!!!!!!!!!!
#define SVC_MODE                    0x13
#define IRQ_MODE                    0x12
#define FIQ_MODE                    0x11
#define ABORT_MODE              0x17
#define UNDEFINE_MODE           0x1b
#define SYS_MODE                    0x1f
#define USER_MODE                   0x10
	.equ USER_CONTEXT,													0x1
	.equ SYSTEM_CONTEXT,													0x2
	.equ SUPERVISOR_CONTEXT,											0x4
	.equ IRQ_CONTEXT,														0x8

/* exported global constant and functions */
	.global delay_us
	.global delay_ms
	.global get_context
	.global do_sys_call

/* imported global constant and functions */
	.extern _core_cycles_us
	.extern _core_cycles_ms

/* data segment */

    .section    .text, "ax"
    .code 32

/*
    void delay_us(unsinged int us)
 */

delay_us:
    ldr r1, =_core_cycles_us
    ldr r1, [r1]
    mul r2, r1, r0
    @2 cycles - calling and return, 4 instructions
    mov r0, #6
us_loop:
    add r2, r2, #3
    cmp r2, r0
    blo us_loop

    bx      lr

/*
    void delay_ms(unsinged int ms)
 */

delay_ms:
    ldr r2, =_core_cycles_ms
    ldr r2, [r2]
    mov r1, #7

ms_loop:
    nop
    add r1, r1, #4
    cmp r1, r2
    blo ms_loop

    sub r0, r0, #1
    mov r1, #4
    cmp r0, #0
    bne ms_loop

    bx      lr


/*
		CONTEXT get_context();
 */

get_context:


__attribute__( ( always_inline ) ) CONTEXT get_context()
{
	CONTEXT res = SUPERVISOR_CONTEXT;
	switch (__get_MODE() & 0x1f)
	{
	case SYS_MODE:
			res = _current_irq >= IRQ_NO_IRQ ? SYSTEM_CONTEXT : IRQ_CONTEXT;
			break;
	case IRQ_MODE:
	case FIQ_MODE:
			res = IRQ_CONTEXT;
			break;
	case USER_MODE:
			res = USER_CONTEXT;
			break;
	default:
			break;
	}

	return res;
}


	mrs	r0, ipsr
	cmp	r0, #0
	bne	irq_context
	mrs	r0, control
	cmp	r0, #3
	bne	system_context
	mov	r0, #USER_CONTEXT
	bx		lr
system_context:
	cmp	r0, #2
	bne	svc_context
	mov	r0, #SYSTEM_CONTEXT
	bx		lr

irq_context:
	cmp	r0, #11
	beq	svc_context
	cmp	r0, #14
	beq	svc_context
	mov	r0, #IRQ_CONTEXT
	bx		lr

svc_context:
	mov	r0, #SUPERVISOR_CONTEXT
	bx		lr

/*
	extern unsigned int do_sys_call(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3);
 */

do_sys_call:
	swi	0
	bx		lr
