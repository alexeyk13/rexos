/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

//if you've got error in this line, than this file is compiled wrong

#include "vectors_cortex_m3.h"
#include "../kernel.h"
#include "asm_defines.h"
#include "../../userspace/sys.h"

/* Define constants used in low-level initialization.  */

    /*
        context:

        r4-r11
        r0-r3, r12
        lr
        pc
        psr
      */

	 .equ    CONTEXT_SIZE,                 (16 * 4)
	 .equ    R0_OFFSET_IN_CONTEXT,			(8 * 4)
	 .equ    LR_OFFSET_IN_CONTEXT,			(13 * 4)
	 .equ    PC_OFFSET_IN_CONTEXT,			(14 * 4)
	 .equ    PSR_OFFSET_IN_CONTEXT,			(15 * 4)

    .equ    LR_TO_HANDLER,                0xfffffff1
	 .equ    LR_TO_PROCESS_PSP,            0xfffffffd
	 .equ    INITIAL_PSP_VALUE,				0x01000000
    .equ    SVC_PSP_VALUE,                0x0100000b

	 .equ    ICSR,                         0xe000ed04
	 .equ    CCR,                          0xe000ed14
    .equ    SHPR1,                        0xe000ed18
    .equ    SHPR2,                        0xe000ed1c
    .equ    SHPR3,                        0xe000ed20
    .equ    SHCSR,                        0xe000ed24

	 .equ    PEND_SV_BIT,                  (1 << 28)
	 .equ    SHPR1_VALUE,                  ((0x02 << 0) | (0x02 << 8) | (0x02 << 16))
	 .equ    SHPR2_VALUE,                  (0xff << 24)
	 .equ    SHPR3_VALUE,                  (0xff << 16)

/* imported global constants and functions */
    .extern on_hard_fault
    .extern on_mem_manage
    .extern on_bus_fault
    .extern on_usage_fault
	 .extern svc
    .extern startup
	 .extern kprocess_abnormal_exit

/* exported global constant and functions */
    .global do_sys_call
    .global pend_switch_context
	 .global process_setup_context

/* data segment */

    .section    .sys_data, "aw"

t_flag: .word 0

/* interrupt vectors */
    .section  .reset, "ax"

int_vectors:
	 .word  (SRAM_BASE + KERNEL_SIZE)
	 .word  Reset_Handler
    .word  NMI_Handler
    .word  HardFault_Handler
    .word  MemManage_Handler
    .word  BusFault_Handler
    .word  UsageFault_Handler
    .word  0
    .word  0
    .word  0
    .word  0
    .word  SVC_Handler
    .word  DebugMon_Handler
    .word  0
    .word  PendSV_Handler
    .word  SysTick_Handler

	 @defined in macro TODO remove this
    external_vectors

/*********************** reset vector handler *********************/
    .syntax unified
    .cpu cortex-m3
    .thumb

    .thumb_func
Reset_Handler:
    cpsid    i

    @setup system and fault handlers priority
    ldr    r1, =SHPR1
    ldr    r0, =SHPR1_VALUE
    str    r0, [r1]

    ldr    r1, =SHPR2
    ldr    r0, =SHPR2_VALUE
    str    r0, [r1]

    ldr    r1, =SHPR3
    ldr    r0, =SHPR3_VALUE
    str    r0, [r1]

    @enable detailed faults
    ldr    r1, =SHCSR
    mov    r2, #7
    lsl    r2, r2, #16
    ldr    r0, [r1]
    orr    r0, r0, r2
    str    r0, [r1]

	 bl    startup                           @ to high-level initialization

    @make context and sp switch
    cpsie    i
    @never reach
    b        .

    /* code segment */

    .section  .text, "ax"
    .syntax unified
    .thumb

/*********************** exception vectors handlers *********************/
.macro exception_enter
    mov    r0, lr
    cmp    r0, 0xfffffffd
    bne    1f
    mrs    r1, psp
    b        2f
1:
    mrs    r1, msp
2:
.endm


    .thumb_func
HardFault_Handler:
    exception_enter
    b    on_hard_fault

    .thumb_func
MemManage_Handler:
    exception_enter
    b    on_mem_manage

    .thumb_func
BusFault_Handler:
    exception_enter
    b    on_bus_fault

    .thumb_func
UsageFault_Handler:
    exception_enter
    b    on_usage_fault

    .thumb_func
SVC_Handler:
    mrs    r12, psp
    ldmia    r12, {r0-r3}
	 bl     svc					                            @ call c handler

    @patch registers in minimal context
    mrs    r12, psp
    str    r0, [r12]

    //return to thread mode
    ldr    pc, =0xfffffffd

    .thumb_func
PendSV_Handler:
    /*save*/
    cpsid    i
	 ldr	  r1, =KERNEL_BASE
	 ldr    r2, [r1, offsetof_KERNEL_next_process]
	 cmp    r2, #0													@late arrival of pendSV can cause double-calling
    beq    context_exit
	 ldr    r3, [r1, offsetof_KERNEL_active_process]	@active_process will be NULL on startup/task destroy
    cmp    r3, #0
    beq    load_context

    mrs    r0, psp
	 stmdb  r0!, {r4-r11}										@save other context on stack
	 str    r0, [r3, offsetof_PROCESS_sp]					@save sp on process->sp

load_context:
	 ldr    r0, [r2, offsetof_PROCESS_sp]					@load sp from process->sp
	 ldmia  r0!, {r4-r11}										@load other context from stack
    msr    psp, r0

	 str    r2, [r1, offsetof_KERNEL_active_process]
	 mov    r0, #0
	 str    r0, [r1, offsetof_KERNEL_next_process]

	 ldr	  r2, [r2, offsetof_PROCESS_heap]			   @ save heap in GLOBAL
	 ldr	  r1, =SRAM_BASE
	 str	  r2, [r1]

context_exit:
    cpsie i
    //return to thread mode
    ldr    pc, =0xfffffffd


	.thumb_func
default_handler:
	 mrs	  r0, ipsr
    ldr    pc, =default_irq_handler


/*********************** context specific *********************/
/*
    void pend_switch_context(void)
*/
    .thumb_func
pend_switch_context:
    ldr    r1, =ICSR
    ldr    r0, =PEND_SV_BIT
    str    r0, [r1]
    bx        lr

/*
	 void process_setup_context(PROCESS* process, PROCESS_FUNCTION fn);
*/
    .thumb_func
process_setup_context:
	 ldr    r12, [r0, offsetof_PROCESS_sp]
	 lsr    r12, r12, #3                                 @8 byte stack align
    lsl    r12, r12, #3
    sub    r12,    r12, #CONTEXT_SIZE
	 ldr    r3, =kprocess_abnormal_exit
	 str    r3, [r12, #LR_OFFSET_IN_CONTEXT]             @abnormal process exit
	 str    r1, [r12, #PC_OFFSET_IN_CONTEXT]             @entry point
	 mov    r3, #INITIAL_PSP_VALUE                       @T-Bit only
    str    r3, [r12, #PSR_OFFSET_IN_CONTEXT]            @run in system context, interrupts are enabled
	 str    r12, [r0, offsetof_PROCESS_sp]

    bx        lr

/*********************** weak unhandled vectors *********************/
.macro weak_vector vector
    .weak    \vector
    .thumb_set \vector,default_handler
.endm

weak_vector    NMI_Handler
weak_vector    DebugMon_Handler
weak_vector    SysTick_Handler

external_weak_vectors
