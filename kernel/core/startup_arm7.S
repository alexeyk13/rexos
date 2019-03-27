/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2018, Alexey Kramarenko
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

//if you've got error in this line, than this file is compiled wrong

#include "../kernel.h"
#include "../../userspace/svc.h"

/* Define constants used in low-level initialization.  */

    .equ  CONTEXT_SIZE,           (16 * 4)
    .equ  SP_CUR_OFFSET,                0x0c
    .equ  R0_OFFSET_IN_CONTEXT,    (11 * 4)
    .equ  LR_OFFSET_IN_CONTEXT,    (10 * 4)
    .equ  PC_OFFSET_IN_CONTEXT,    (1 * 4)
    .equ  CPSR_OFFSET_IN_CONTEXT,    (0 * 4)
    .equ  PEND_SV_FLAG_ADDR,        (SRAM_BASE + GLOBAL_SIZE - 4)
    .equ  KPROCESS_PROCESS,      8
    .equ  KPROCESS_SP,           12
    .equ  ACTIVE_PROCESS,        0
    .equ  NEXT_PROCESS,          4

/* imported global constants and functions */

    .extern undefined_entry_arm7
    .extern prefetch_abort_entry_arm7
    .extern data_abort_entry_arm7

    .extern svc
    .extern startup

    .extern kprocess_abnormal_exit
    .extern kirq_enter

/* exported global constant and functions */
    .global pend_switch_context
    .global process_setup_context

/* interrupt vectors */

    .section  .isr_vectors, "ax"
    .code 32

    ldr    pc, (int_table + 0x0)
    ldr    pc, (int_table + 0x4)
    ldr    pc, (int_table + 0x8)
    ldr    pc, (int_table + 0xc)
    ldr    pc, (int_table + 0x10)
    ldr    pc, (int_table + 0x14)
    ldr    pc, (int_table + 0x18)
    ldr    pc, (int_table + 0x1c)

int_table:
    .word     reset_vector
    .word  undefined_vector
    .word  swi_vector
    .word  prefetch_abort_vector
    .word  data_abort_vector
    .word     0x00
    .word  irq_vector
    .word  fiq_vector

    .section    .reset, "ax"
/*********************** reset vector handler *********************/
reset_vector:
    /* switch to svc mode, if not yet */
    msr    cpsr_cxsf, #(SVC_MODE | I_BIT | F_BIT)
    msr    spsr_cxsf, #(SYS_MODE | I_BIT | F_BIT)

    /* setup initial stack pointers */
    msr    cpsr_c, #(IRQ_MODE | I_BIT | F_BIT)
    ldr   sp,=IRQ_STACK_END

    msr    cpsr_c, #(FIQ_MODE | I_BIT | F_BIT)
    ldr   sp,=FIQ_STACK_END

    msr    cpsr_c, #(ABORT_MODE | I_BIT | F_BIT)
    ldr   sp,=ABT_STACK_END

    msr    cpsr_c, #(UNDEFINE_MODE | I_BIT | F_BIT)
    ldr   sp,=UND_STACK_END

    msr    cpsr_c, #(SVC_MODE | I_BIT | F_BIT)
    ldr   sp,=SVC_STACK_END

    bl    startup                           @ to high-level initialization

    stmfd    sp!, {lr}                            @ our first context switch will be loaded here
    bl        thread_switch_context

    msr    cpsr_c, #(SYS_MODE | I_BIT | F_BIT)
    ldmfd    sp!, {r0-r3, r12}
    msr    cpsr_c, #(SVC_MODE | I_BIT | F_BIT)

    ldmfd    sp!, {pc}^


/*********************** exception vectors handlers *********************/
@save minimal context on caller's thread
.macro exception_enter src, mask
    stmfd    sp!, {lr}
    mrs    lr, spsr
    orr    lr, \mask
    msr    cpsr_c, lr
    stmfd    sp!, {r0-r3, r12}
    msr    cpsr_c, \src
.endm


@check for context switching, then load minimal context from caller's thread
.macro exception_exit src, mask
    bl        thread_switch_context

    mrs    lr, spsr
    orr    lr, \mask
    msr    cpsr_c, lr
    ldmfd    sp!, {r0-r3, r12}
    msr    cpsr_c, \src
    ldmfd    sp!, {pc}^
.endm

undefined_vector:
    exception_enter #(UNDEFINE_MODE | I_BIT | F_BIT), #(I_BIT | F_BIT)

    mov    r0, lr
    sub    r0, #4
    bl     undefined_entry_arm7                @ call c handler

    exception_exit #(UNDEFINE_MODE | I_BIT | F_BIT), #(I_BIT | F_BIT)

swi_vector:
    exception_enter #(SVC_MODE | I_BIT), #(I_BIT)
    bl        svc                            @ call c handler
    exception_exit #(SVC_MODE | I_BIT), #(I_BIT)

prefetch_abort_vector:
    subs    lr, lr, #4                            @ return to same instruction
    exception_enter #(ABORT_MODE | I_BIT | F_BIT), #(I_BIT | F_BIT)

    mov    r0, lr
    bl        prefetch_abort_entry_arm7        @ call c handler

    exception_exit #(ABORT_MODE | I_BIT | F_BIT), #(I_BIT | F_BIT)

data_abort_vector:
    subs    lr, lr, #8                            @ return to instruction, caused access violation
    exception_enter #(ABORT_MODE | I_BIT | F_BIT), #(I_BIT | F_BIT)

    mov    r0, lr
    bl        data_abort_entry_arm7            @ call c handler

    exception_exit #(ABORT_MODE | I_BIT | F_BIT), #(I_BIT | F_BIT)

irq_vector:
    subs    lr, lr, #4                            @ return to same instruction
    exception_enter #(IRQ_MODE | I_BIT), #(I_BIT)
    mrs    lr, spsr
    stmfd    sp!, {lr}                            @ save SPSR for nested interrupts

    /* nested call implementation */
    msr    cpsr_c, #SVC_MODE                    @ from now interrupts are enabled
    stmfd    sp!, {r0-r3, lr}

    bl        irq_get_vector
    bl        kirq_enter                            @ call handler

    ldmfd    sp!, {r0-r3, lr}
    msr    cpsr_c, #(IRQ_MODE | I_BIT)
    /* nested call done */

    ldmfd sp!, {lr}                            @ restore SPSR
    msr    spsr_cxsf, lr
    exception_exit #(IRQ_MODE | I_BIT), #(I_BIT)

fiq_vector:
    subs    lr, lr, #4                            @ return to same instruction
    exception_enter #(FIQ_MODE | I_BIT | F_BIT), #(I_BIT | F_BIT)

    bl        kirq_enter                            @ call handler

    exception_exit #(FIQ_MODE | I_BIT | F_BIT), #(I_BIT | F_BIT)

/* code segment */

    .section    .text, "ax"
    .code 32

/*********************** context specific *********************/
/*
    void pend_switch_context(void)
*/
pend_switch_context:
    ldr    r0, =PEND_SV_FLAG_ADDR
    mov    r1, #1
    str    r1, [r0]
    bx lr

/*
    void thread_setup_context(THREAD* thread, THREAD_FUNCTION fn, void* param);
*/

thread_setup_context:
    ldr    r12, [r0, #SP_CUR_OFFSET]
    lsr    r12, r12, #3                                        @8-byte stack align
    lsl    r12, r12, #3
    sub    r12, r12, #CONTEXT_SIZE
    str    r2, [r12, #R0_OFFSET_IN_CONTEXT]                @param
    ldr    r3, =kprocess_abnormal_exit
    str    r3, [r12, #LR_OFFSET_IN_CONTEXT]                @abnormal thread exit
    str    r1, [r12, #PC_OFFSET_IN_CONTEXT]                @entry point
    mov    r3, #SYS_MODE
    str    r3, [r12, #CPSR_OFFSET_IN_CONTEXT]            @run in system context, interrupts are enabled
    str    r12, [r0, #SP_CUR_OFFSET]

    bx        lr

/*
    thread_switch_context

    at entry point lr already on stack
*/
thread_switch_context:
    @on entry point, minimal context is saved, we are free to waste r0-r3, r12

    mrs    r0, spsr                                @call from SYS/USER context?
    add    r0, #1
    and    r0, r0, #0xf
    cmp    r0, #0x1
    bhi    no_switch
    ldr    r0, =PEND_SV_FLAG_ADDR          @switch pending?
    ldr    r1, [r0]
    cmp    r1, #0
    beq    no_switch
    mov    r1, #0
    str    r1, [r0]

    mrs    r2, cpsr                                @switch to user mode
    mrs    r0, spsr                                @r0 - spsr (current process cpsr)
    ldr    r1, [sp]                                @r1 - saved lr (current process pc)
    orr    r3, r0, #I_BIT
    msr    cpsr_c, r3

    /*save*/

    ldr   r3, =KERNEL_BASE
    ldr   r3, [r3, ACTIVE_PROCESS]

    cmp    r3, #0                                @_active_thread will be NULL on startup/task destroy
    beq    load_context

    stmfd    sp!, {r0, r1, r4-r11, lr}        @save other context on stack
    str    sp, [r3, KPROCESS_SP]    @save sp on thread sp_cur

load_context:

    ldr   r3, =KERNEL_BASE
    ldr   r3, [r3, NEXT_PROCESS]

    ldr    sp, [r3, KPROCESS_SP]                                   @load sp from thread sp_cur
    ldmfd    sp!, {r0, r1, r4-r11, lr}                               @load other context from stack

    msr    cpsr_cxsf, r2                        @back to exception mode
    str    r1, [sp]                                @r1 - saved lr (current process pc)
    msr    spsr_cxsf, r0                        @r0 - spsr (current process cpsr)

    ldr   r0, =KERNEL_BASE
    str   r3, [r0, ACTIVE_PROCESS]
    mov   r3, #0
    str   r3, [r0, NEXT_PROCESS]

    ldr   r0, =KERNEL_BASE
    ldr   r0, [r0, ACTIVE_PROCESS]
    ldr   r0, [r0, KPROCESS_PROCESS]
    ldr   r3, =SRAM_BASE
    str   r0, [r3]

no_switch:
    bx        lr
