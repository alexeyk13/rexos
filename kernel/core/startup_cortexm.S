/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

//if you've got error in this line, than this file is compiled wrong

#include "../kernel.h"
#include "../../userspace/svc.h"
#include "../dbg.h"

/* Define constants used in low-level initialization.  */

    /*
        context:

        r4-r11
        r0-r3, r12
        lr
        pc
        psr
      */

    .equ   CONTEXT_SIZE,          (16 * 4)
    .equ   R0_OFFSET_IN_CONTEXT,  (8 * 4)
    .equ   LR_OFFSET_IN_CONTEXT,  (13 * 4)
    .equ   PC_OFFSET_IN_CONTEXT,  (14 * 4)
    .equ   PSR_OFFSET_IN_CONTEXT, (15 * 4)
    .equ   KPROCESS_PROCESS,      8
    .equ   KPROCESS_SP,           12
    .equ   ACTIVE_PROCESS,        0
    .equ   NEXT_PROCESS,          4

    .equ   LR_TO_HANDLER,         0xfffffff1
    .equ   LR_TO_PROCESS_PSP,     0xfffffffd
    .equ   INITIAL_PSP_VALUE,     0x01000000
    .equ   SVC_PSP_VALUE,         0x0100000b

    .equ   ICSR,                  0xe000ed04
    .equ   CCR,                   0xe000ed14
    .equ   SHPR1,                 0xe000ed18
    .equ   SHPR2,                 0xe000ed1c
    .equ   SHPR3,                 0xe000ed20
    .equ   SHCSR,                 0xe000ed24
    .equ   ICPR0,                 0xe000e280

    .equ   PEND_SV_SET,           (1 << 28)
    .equ   PEND_SV_CLEAR,         (1 << 27)
    .equ   SHPR1_VALUE,           ((0x00 << 0) | (0x00 << 8) | (0x00 << 16))
    .equ   SHPR2_VALUE,           (0xff << 24)
    .equ   SHPR3_VALUE,           (0xff << 16)

/* imported global constants and functions */
    .extern on_hard_fault
    .extern on_mem_manage
    .extern on_bus_fault
    .extern on_usage_fault
    .extern svc
    .extern startup
    .extern kprocess_abnormal_exit
    .extern kirq_stub
    .extern kirq_enter
    .extern hardware_init

/* exported global constant and functions */
    .global pend_switch_context
    .global process_setup_context

/* interrupt vectors */
    .section  .isr_vectors, "ax"

int_vectors:
    .word  (SRAM_BASE + SRAM_SIZE)
    .word  Reset_Handler
    .word  NMI_Handler
    .word  HardFault_Handler
#if defined(CORTEX_M3) || defined(CORTEX_M4) || defined(CORTEX_M7)
    .word  MemManage_Handler
    .word  BusFault_Handler
    .word  UsageFault_Handler
#else
    .word  0
    .word  0
    .word  0
#endif
    .word  0
    .word  0
    .word  0
    .word  0
    .word  SVC_Handler
#if defined(CORTEX_M3) || defined(CORTEX_M4) || defined(CORTEX_M7)
    .word  DebugMon_Handler
#else
    .word  0
#endif
    .word  0
    .word  PendSV_Handler
    .word  SysTick_Handler

    .rept IRQ_VECTORS_COUNT
        .word  irq_handler
    .endr

/*********************** reset vector handler *********************/
        .section  .reset, "ax"
        .syntax unified
#if defined(CORTEX_M3)
    .cpu cortex-m3
#elif defined(CORTEX_M4)
    .cpu cortex-m4
#elif defined(CORTEX_M7)
    .cpu cortex-m7
#else
    .cpu cortex-m0
#endif
    .thumb

    .thumb_func
Reset_Handler:
    cpsid i

#ifdef STARTUP_HARDWARE_INIT
    bl    startup_hardware_init
#endif //STARTUP_HARDWARE_INIT

    @setup system and fault handlers priority
#if defined(CORTEX_M3) || defined(CORTEX_M4) || defined(CORTEX_M7)
    ldr   r1, =SHPR1
    ldr   r0, =SHPR1_VALUE
    str   r0, [r1]
#endif

    ldr   r1, =SHPR2
    ldr   r0, =SHPR2_VALUE
    str   r0, [r1]

    ldr   r1, =SHPR3
    ldr   r0, =SHPR3_VALUE
    str   r0, [r1]

#if defined(CORTEX_M3) || defined(CORTEX_M4) || defined(CORTEX_M7)
    @enable detailed faults
    ldr   r1, =SHCSR
    movs  r2, #7
    lsls  r2, r2, #16
    ldr   r0, [r1]
    orrs  r0, r0, r2
    str   r0, [r1]
    #endif

    @clear all pending interrupts
    ldr    r0, =0xffffffff
    ldr    r1, =ICPR0
    str    r0, [r1]
#if defined(CORTEX_M3) || defined(CORTEX_M4) || defined(CORTEX_M7)
    str    r0, [r1, #0x4]
    str    r0, [r1, #0x8]
    str    r0, [r1, #0xc]
    str    r0, [r1, #0x10]
    str    r0, [r1, #0x14]
    str    r0, [r1, #0x18]
    str    r0, [r1, #0x1c]
#endif

#if (KERNEL_PROFILING)
    ldr    r1, =(SRAM_BASE + SRAM_SIZE)
    ldr    r2, =KERNEL_STACK_MAX
    subs   r0, r1, r2
    ldr    r2, =MAGIC_UNINITIALIZED
profiling_loop:
#ifdef CORTEX_M0
    str    r2, [r0]
    adds   r0, r0, #4
#else
    str    r2, [r0], #4
#endif
    cmp    r0, r1
    bcc    profiling_loop
#endif

    bl     startup                @ to high-level initialization

    @make context and sp switch
    cpsie  i
        @never reach
    b      .

    /* code segment */

    .section  .text, "ax"
    .syntax unified
    .thumb

/*********************** exception vectors handlers *********************/
.macro exception_enter
    mov   r0, lr
#if defined(CORTEX_M0)
    movs  r1, #0
    subs  r1, r1, #3
    cmp   r0, r1
#else
    cmp   r0, 0xfffffffd
#endif
    bne   1f
    mrs   r1, psp
    b     2f
1:
    mrs   r1, msp
2:
.endm


    .thumb_func
HardFault_Handler:
    exception_enter
    b   on_hard_fault

#if defined(CORTEX_M3) || defined(CORTEX_M4) || defined(CORTEX_M7)
    .thumb_func
MemManage_Handler:
    exception_enter
    b   on_mem_manage

    .thumb_func
BusFault_Handler:
    exception_enter
    b   on_bus_fault

    .thumb_func
UsageFault_Handler:
    exception_enter
    b   on_usage_fault
#endif

    .thumb_func
SVC_Handler:
    mrs   r0, psp
    ldmia r0, {r0-r3}
    bl    svc                           @ call c handler

    //return to thread mode
#if defined(CORTEX_M0)
    ldr   r0, =0xfffffffd
    bx    r0
#else
    ldr   pc, SW
SW: .word LR_TO_PROCESS_PSP
#endif

    .thumb_func
PendSV_Handler:
    /*save*/
    cpsid i
    ldr   r2, =ICSR                                             @late arrival of pendSV can cause double-calling
    ldr   r0, =PEND_SV_CLEAR
    str   r0, [r2]
    ldr   r1, =KERNEL_BASE
    ldr   r3, [r1, ACTIVE_PROCESS]   @active_process will be NULL on startup/task destroy
    cmp   r3, #0
    beq   load_context

    mrs   r0, psp

#if defined(CORTEX_M0)
    subs  r0, r0, #(4 * 4)                         @save other context on stack
    stmia r0!, {r4-r7}
    mov    r4, r8
    mov   r5, r9
    mov   r6, r10
    mov   r7, r11
    subs  r0, r0, #(8 * 4)
    stmia r0!, {r4-r7}
    subs  r0, r0, #(4 * 4)
#else
    stmdb  r0!, {r4-r11}                           @save other context on stack
#endif
    str   r0, [r3, KPROCESS_SP]                     @save sp on process->sp

load_context:
    ldr   r2, [r1, NEXT_PROCESS]
    cmp   r2, #0
    beq   halt_core                              @halt core if no tasks

    ldr   r0, [r2, KPROCESS_PROCESS]
    ldr   r3, =SRAM_BASE
    str   r0, [r3]
    ldr   r0, [r2, KPROCESS_SP]                    @load sp from process->sp
#if defined(CORTEX_M0)
    ldmia r0!, {r4-r7}                             @load other context from stack
    mov   r8, r4
    mov   r9, r5
    mov   r10, r6
    mov   r11, r7
    ldmia r0!, {r4-r7}
#else
    ldmia r0!, {r4-r11}                           @load other context from stack
#endif

    msr   psp, r0

    str   r2, [r1, ACTIVE_PROCESS]
    movs  r0, #0
    str   r0, [r1, NEXT_PROCESS]

context_exit:
    cpsie i
    //return to thread mode
#if defined(CORTEX_M0)
    ldr   r0, =0xfffffffd
    bx    r0
#else
    ldr   pc, SW
#endif

halt_core:
    cpsie    i
#if !(KERNEL_DEVELOPER_MODE)
    wfi
#endif //!KERNEL_DEVELOPER_MODE
    cpsid i
    ldr   r2, =ICSR
    ldr   r0, =PEND_SV_CLEAR
    str   r0, [r2]
    b        load_context


    .thumb_func
default_handler:
    mrs   r0, ipsr
    subs  r0, r0, 16
    b     kirq_stub

    .thumb_func
irq_handler:
    mrs   r0, ipsr
    subs  r0, r0, #16
    b     kirq_enter

/*********************** context specific *********************/
/*
    void pend_switch_context(void)
*/
    .thumb_func
pend_switch_context:
    ldr   r1, =ICSR
    ldr   r0, =PEND_SV_SET
    str   r0, [r1]
    bx    lr

/*
    void process_setup_context(KPROCESS* process, PROCESS_FUNCTION fn);
*/
    .thumb_func
process_setup_context:
    ldr   r2, [r0, KPROCESS_SP]
    lsrs  r2, r2, #3                                        @8 byte stack align
    lsls  r2, r2, #3
    subs  r2, r2, #CONTEXT_SIZE
    ldr   r3, =kprocess_abnormal_exit
    str   r3, [r2, #LR_OFFSET_IN_CONTEXT]         @abnormal process exit
    str   r1, [r2, #PC_OFFSET_IN_CONTEXT]         @entry point
#if defined(CORTEX_M3)
    movs  r3, #INITIAL_PSP_VALUE                        @T-Bit only
#else
    ldr   r3, =INITIAL_PSP_VALUE                        @T-Bit only
#endif
    str   r3, [r2, #PSR_OFFSET_IN_CONTEXT]        @run in system context, interrupts are enabled
    str   r2, [r0, KPROCESS_SP]

    bx    lr

/*********************** weak unhandled vectors *********************/
.macro weak_vector vector
    .weak   \vector
    .thumb_set \vector,default_handler
.endm

weak_vector   NMI_Handler
#if defined(CORTEX_M3) || defined(CORTEX_M4) || defined(CORTEX_M7)
weak_vector   DebugMon_Handler
#endif
weak_vector   SysTick_Handler
