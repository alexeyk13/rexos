/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

//if you've got error in this line, than this file is compiled wrong

#include "memmap.h"
#include "hw_config.h"
#include "kernel_config.h"
#include "magic.h"
#include "irq_arm7.h"

/* Define constants used in low-level initialization.  */

    .equ    CONTEXT_SIZE,            (16 * 4)
	.equ	SP_CUR_OFFSET,				0x0c
	.equ	R0_OFFSET_IN_CONTEXT,	(11 * 4)
	.equ	LR_OFFSET_IN_CONTEXT,	(10 * 4)
	.equ	PC_OFFSET_IN_CONTEXT,	(1 * 4)
	.equ	CPSR_OFFSET_IN_CONTEXT,	(0 * 4)

	.equ	IRQn_NO_IRQ,				0x20

/* imported global constants and functions */
	.extern _etext
	.extern _data
	.extern _edata
	.extern _bss
	.extern _bss_end
	.extern _etext_data

	.extern undefined_entry_arm7
	.extern prefetch_abort_entry_arm7
	.extern data_abort_entry_arm7

	.extern _fiq_vector
	.extern fiq_hw_exit

	.extern _irq_priorities
	.extern _irq_pending_list_size
	.extern _irq_vectors
	.extern _current_irq
	.extern irq_push_pending
	.extern irq_pop_pending
	.extern irq_hw_get_vector
	.extern irq_hw_exit

	.extern sys_handler
	.extern startup

	.extern _active_thread
	.extern _next_thread
	.extern abnormal_exit

/* exported global constant and functions */
	.global reset_vector

	.global pend_switch_context
	.global thread_setup_context

/* data segment */

	.section	.sys_bss, "aw"

pend_sv_flag:	.word	0x00000000

/* interrupt vectors */

	.section	.reset, "ax"
	.code 32

	ldr	pc, (int_table + 0x0)
	ldr	pc, (int_table + 0x4)
	ldr	pc, (int_table + 0x8)
	ldr	pc, (int_table + 0xc)
	ldr	pc, (int_table + 0x10)
	ldr	pc, (int_table + 0x14)
	ldr	pc, (int_table + 0x18)
	ldr	pc, (int_table + 0x1c)

int_table:
	.word	 reset_vector
	.word  undefined_vector
	.word  swi_vector
	.word  prefetch_abort_vector
	.word  data_abort_vector
	.word	 0x00
	.word  irq_vector
	.word  fiq_vector

/*********************** reset vector handler *********************/
reset_vector:
	/* switch to svc mode, if not yet */
	msr	cpsr_cxsf, #(SVC_MODE | I_BIT | F_BIT)
	msr	spsr_cxsf, #(SYS_MODE | I_BIT | F_BIT)

#ifdef STARTUP_HARDWARE_INIT
	bl		startup_hardware_init
#endif //STARTUP_HARDWARE_INIT

	/* initialize memory */
	@ copy data from flash to ram
	ldr	r0, =_etext
	ldr	r1, =_data
	ldr	r2, =_edata
data_loop:
	cmp	r1, r2
	ldrlo	r3, [r0], #4
	strlo	r3, [r1], #4
	blo	data_loop

	@ zero bss area
	mov	r0, #0
	ldr	r1, =_bss
	ldr	r2, =_bss_end
bss_loop:
	cmp	r1, r2
	strlo	r0, [r1], #4
	blo	bss_loop

#if (KERNEL_PROFILING)
	ldr	r0, =MAGIC_UNINITIALIZED
	ldr	r1, =SVC_STACK_TOP
	ldr	r2, =SVC_STACK_END
svc_stack_fill:
	cmp	r1, r2
	strlo	r0, [r1], #4
	blo	svc_stack_fill

	ldr	r1, =IRQ_STACK_TOP
	ldr	r2, =IRQ_STACK_END
irq_stack_fill:
	cmp	r1, r2
	strlo	r0, [r1], #4
	blo	irq_stack_fill

	ldr	r1, =FIQ_STACK_TOP
	ldr	r2, =FIQ_STACK_END
fiq_stack_fill:
	cmp	r1, r2
	strlo	r0, [r1], #4
	blo	fiq_stack_fill

	ldr	r1, =ABT_STACK_TOP
	ldr	r2, =ABT_STACK_END
abt_stack_fill:
	cmp	r1, r2
	strlo	r0, [r1], #4
	blo	abt_stack_fill

	ldr	r1, =UND_STACK_TOP
	ldr	r2, =UND_STACK_END
und_stack_fill:
	cmp	r1, r2
	strlo	r0, [r1], #4
	blo	und_stack_fill
#endif //KERNEL_PROFILING

	/* setup initial stack pointers */
	msr	cpsr_c, #(IRQ_MODE | I_BIT | F_BIT)
	ldr   sp,=IRQ_STACK_END

	msr	cpsr_c, #(FIQ_MODE | I_BIT | F_BIT)
	ldr   sp,=FIQ_STACK_END

	msr	cpsr_c, #(ABORT_MODE | I_BIT | F_BIT)
	ldr   sp,=ABT_STACK_END

	msr	cpsr_c, #(UNDEFINE_MODE | I_BIT | F_BIT)
	ldr   sp,=UND_STACK_END

	msr	cpsr_c, #(SVC_MODE | I_BIT | F_BIT)
	ldr   sp,=SVC_STACK_END

	bl    startup			               @ to high-level initialization

	stmfd	sp!, {lr}							@ our first context switch will be loaded here
	bl		thread_switch_context

	msr	cpsr_c, #(SYS_MODE | I_BIT | F_BIT)
	ldmfd	sp!, {r0-r3, r12}
	msr	cpsr_c, #(SVC_MODE | I_BIT | F_BIT)

	ldmfd	sp!, {pc}^


/*********************** exception vectors handlers *********************/
@save minimal context on caller's thread
.macro exception_enter src, mask
	stmfd	sp!, {lr}
	mrs	lr, spsr
	orr	lr, \mask
	msr	cpsr_c, lr
	stmfd	sp!, {r0-r3, r12}
	msr	cpsr_c, \src
.endm


@check for context switching, then load minimal context from caller's thread
.macro exception_exit src, mask
	bl		thread_switch_context

	mrs	lr, spsr
	orr	lr, \mask
	msr	cpsr_c, lr
	ldmfd	sp!, {r0-r3, r12}
	msr	cpsr_c, \src
	ldmfd	sp!, {pc}^
.endm

undefined_vector:
	exception_enter #(UNDEFINE_MODE | I_BIT | F_BIT), #(I_BIT | F_BIT)

	mov	r0, lr
	sub	r0, #4
	bl		undefined_entry_arm7				@ call c handler

	exception_exit #(UNDEFINE_MODE | I_BIT | F_BIT), #(I_BIT | F_BIT)

swi_vector:
	exception_enter #(SVC_MODE | I_BIT), #(I_BIT)

	bl		sys_handler							@ call c handler
	@patch registers in minimal context
	mrs	lr, spsr
	orr	lr, #I_BIT
	msr	cpsr_c, lr
	str	r0, [sp, #(0x0 * 4)]
	msr	cpsr_c, #(SVC_MODE | I_BIT)

	exception_exit #(SVC_MODE | I_BIT), #(I_BIT)

prefetch_abort_vector:
	subs	lr, lr, #4							@ return to same instruction
	exception_enter #(ABORT_MODE | I_BIT | F_BIT), #(I_BIT | F_BIT)

	mov	r0, lr
	bl		prefetch_abort_entry_arm7		@ call c handler

	exception_exit #(ABORT_MODE | I_BIT | F_BIT), #(I_BIT | F_BIT)

data_abort_vector:
	subs	lr, lr, #8							@ return to instruction, caused access violation
	exception_enter #(ABORT_MODE | I_BIT | F_BIT), #(I_BIT | F_BIT)

	mov	r0, lr
	bl		data_abort_entry_arm7			@ call c handler

	exception_exit #(ABORT_MODE | I_BIT | F_BIT), #(I_BIT | F_BIT)

irq_vector:
	subs	lr, lr, #4							@ return to same instruction
	exception_enter #(IRQ_MODE | I_BIT), #(I_BIT)
	mrs	lr, spsr
	stmfd	sp!, {r4, r5, r6, lr}			@ save SPSR for nested interrupts

	bl		irq_hw_get_vector
	ldr	r5, =_current_irq					@ r5 - ptr to runninh IRQn
	ldr	r6, =_irq_priorities				@ r6 - ptr to table of priorities

	@ while (_irq_priorities[irq_now] < _irq_priorities[_current_irq])
nested_call_loop:
	ldr	r2, [r5]
	cmp	r2, #IRQ_NO_IRQ
	beq	nested_enter
	ldr	r2, [r6, r2, LSL #2]
	ldr	r1, [r6, r0, LSL #2]
	cmp	r1, r2
	bhi	call_done
nested_enter:
	ldr	r4, [r5]								@ r4 - saved_irq
	str	r0, [r5]

	ldr	r1, =_irq_vectors
	ldr	r1, [r1, r0, LSL #2]

	/* nested call implementation */
	msr	cpsr_c, #SVC_MODE			@ from now interrupts are enabled
	stmfd	sp!, {lr}

	mov	lr, pc
	bx		r1								@ call handler
	ldr	r0, [r5]
	bl		irq_hw_exit

	ldmfd	sp!, {lr}
	msr	cpsr_c, #(IRQ_MODE | I_BIT)
	/* nested call done */

	str	r4, [r5]								@ restore current irq after call


	@if (_irq_pending_list_size && _irq_priorities[_irq_pending_list[0]] < _irq_priorities[_current_irq])
	ldr	r1, =_irq_pending_list_size
	ldr	r1, [r1]
	cmp	r1, #0
	beq	call_done_all

	ldr	r1, =_irq_pending_list
	ldr	r1, [r1]
	ldr	r1, [r6, r1, LSL #2]
	ldr	r2, [r5]
	ldr	r2, [r6, r2, LSL #2]
	cmp	r1, r2
	bhi	call_done_all

	bl		irq_pop_pending
	b		nested_call_loop

call_done_all:
	mov	r0, #IRQ_NO_IRQ

call_done:

	cmp	r0, #IRQ_NO_IRQ
	beq	irq_exit
	bl		irq_push_pending

irq_exit:
	ldmfd sp!, {r4, r5, r6, lr}			@ restore SPSR
	msr	spsr_cxsf, lr
	exception_exit #(IRQ_MODE | I_BIT), #(I_BIT)

fiq_vector:
	subs	lr, lr, #4							@ return to same instruction
	exception_enter #(FIQ_MODE | I_BIT | F_BIT), #(I_BIT | F_BIT)

	ldr	r0, =_fiq_vector					@call handler
	ldr	r0, [r0]
	mov	lr, pc
	bx		r0

	bl		fiq_hw_exit

	exception_exit #(FIQ_MODE | I_BIT | F_BIT), #(I_BIT | F_BIT)

/* code segment */

	.section	.text, "ax"
	.code 32

/*********************** context specific *********************/
/*
	void pend_switch_context(void)
*/
pend_switch_context:
	ldr	r0, =pend_sv_flag
	mov	r1, #1
	str	r1, [r0]
	bx lr

/*
	void thread_setup_context(THREAD* thread, THREAD_FUNCTION fn, void* param);
*/

thread_setup_context:
	ldr	r12, [r0, #SP_CUR_OFFSET]
	lsr	r12, r12, #3										@8-byte stack align
	lsl	r12, r12, #3
	sub	r12, r12, #CONTEXT_SIZE
	str	r2, [r12, #R0_OFFSET_IN_CONTEXT]				@param
	ldr	r3, =abnormal_exit
	str	r3, [r12, #LR_OFFSET_IN_CONTEXT]				@abnormal thread exit
	str	r1, [r12, #PC_OFFSET_IN_CONTEXT]				@entry point
	mov	r3, #SYS_MODE
	str	r3, [r12, #CPSR_OFFSET_IN_CONTEXT]			@run in system context, interrupts are enabled
	str	r12, [r0, #SP_CUR_OFFSET]

	bx		lr

/*
	thread_switch_context

	at entry point lr already on stack
*/
thread_switch_context:
	@on entry point, minimal context is saved, we are free to waste r0-r3, r12

	mrs	r0, spsr								@call from SYS/USER context?
	add	r0, #1
	and	r0, r0, #0xf
	cmp	r0, #0x1
	bhi	no_switch
	ldr	r0, =pend_sv_flag					@switch pending?
	ldr	r1, [r0]
	cmp	r1, #0
	beq	no_switch
	mov	r1, #0
	str	r1, [r0]

	mrs	r2, cpsr								@switch to user mode
	mrs	r0, spsr								@r0 - spsr (current process cpsr)
	ldr	r1, [sp]								@r1 - saved lr (current process pc)
	orr	r3, r0, #I_BIT
	msr	cpsr_c, r3

	/*save*/

	ldr	r3, =_active_thread				@_active_thread will be NULL on startup/task destroy
	ldr	r3, [r3]
	cmp	r3, #0
	beq	load_context

	stmfd	sp!, {r0, r1, r4-r11, lr}		@save other context on stack
	str	sp, [r3, #SP_CUR_OFFSET]		@save sp on thread sp_cur

load_context:

	ldr	r3, =_next_thread
	ldr	r3, [r3]

	ldr	sp, [r3, #SP_CUR_OFFSET]		@load sp from thread sp_cur
	ldmfd	sp!, {r0, r1, r4-r11, lr}		@load other context from stack

	msr	cpsr_cxsf, r2						@back to exception mode
	str	r1, [sp]								@r1 - saved lr (current process pc)
	msr	spsr_cxsf, r0						@r0 - spsr (current process cpsr)

	ldr	r0, =_active_thread
	str	r3, [r0]
	ldr	r0, =_next_thread
	mov	r3, #0
	str	r3, [r0]

no_switch:
	bx		lr
