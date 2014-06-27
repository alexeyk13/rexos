/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KERNEL_H
#define KERNEL_H

/*
    core_kernel.h - MCU core specific functions. Kernel part
*/

#include "../userspace/core/core.h"
#include "kernel_config.h"

#define KERNEL_BASE                                         (SRAM_BASE + KERNEL_GLOBAL_SIZE)

#if !defined(LDS) && !defined(__ASSEMBLER__)

#include "thread_kernel.h"
#include "../lib/pool.h"

#include "../mod/console/console.h"
#include "../drv_if/uart.h"
#include "../drv_if/gpio.h"
#include "../arch/cortex_m3/stm/timer_stm32.h"
#include "../userspace/timer.h"

// will be aligned to pass MPU requirements
typedef struct {
    //first 3 params same as userspace for library calls error handling
    //header size including name
    int struct_size;
    POOL pool;
    int error;

    //----------------------process specific-----------------------------
    //for context-switching
    //This values are used in asm context switching. Don't place them more than 128 bytes from start of KERNEL
    //now running thread. (Active context).
    volatile THREAD* active_thread;
    //next thread to run, after leave. For context switch. If NULL - no context switch is required
    volatile THREAD* next_thread;

    THREAD* active_threads[THREAD_CACHE_SIZE];
    THREAD* threads_uncached;
    int thread_list_size;

    //Current thread. If there is no active tasks, idle_task will be run
    THREAD* current_thread;
    THREAD* idle_thread;

    //------------------------- timer specific -------------------------
    TIME uptime;

    //refactor me later
    CB_SVC_TIMER cb_svc_timer;

    TIMER* timers;
    volatile bool timer_inside_isr;
    unsigned int hpet_value;
    //----------------------- paged memory related ---------------------
    POOL paged;

    //--- move this shit later to userspace
    unsigned long core_cycles_us;
    unsigned long core_cycles_ms;
    unsigned long core_freq;
    unsigned long fs_freq;
    unsigned long ahb_freq;
    unsigned long apb1_freq;
    unsigned long apb2_freq;
    CONSOLE* dbg_console;
    UART_HW* uart_handlers[6];

    char used_pins[8];
    char exti_5_9_active;
    char exti_10_15_active;
    EXTI_HANDLER exti_handlers[15];

    char afio_remap_count;
    char syscfg_count;

    char shared1;
    char shared8;
    UTIMER_HANDLER timer_handlers[15];
    unsigned int rand;

    int killme;
    POOL killme_pool;

    //name is following
} KERNEL;

#define __KERNEL                                            ((KERNEL*)(KERNEL_BASE))
#define __KERNEL_NAME                                       ((char*)(KERNEL_BASE + sizeof(KERNEL)))


/** \addtogroup arch_porting architecture porting
        For architecture support, all these functions must be implemented.
        Prototypes of hw-dependend functions. In most cases they are implemented in asm
    \{
 */
/**
    \brief pend switch context
    \details Pends context switching and return immediatly.

    On SVC/IRQ leaving, function implementation is called and do following:

    - if (_active_thread) is not NULL, save current thread context on _active_thread stack
    - load current thread context from _next_thread stack
    - adjust stack pointer with _next_thread.cur_sp
    - move _next_thread - > _active_thread, _next_thread = NULL
    \retval none
*/
extern void pend_switch_context(void);
/**
    \brief setup initial context
    \details Just setup context. Real switching will be performed on pend_switch_context, when
    _next_thread == thread.
    \param thread: thread handle
    \param fn: thread start point
    \retval none
*/
extern void thread_setup_context(THREAD* thread, void (*fn)(void));

/**
    \brief reset system core.
    \param none
    \retval none
*/
extern void reset();

/** \} */ // end of arch_porting group


/** \addtogroup kernel kernel part of core-related functionality
    \{
 */

/**
    \brief default handler for irq
    \details This function should be called on unhandled irq exception
    \param vector: vector number
    \retval none
*/
void default_irq_handler(int vector);


/**
    \brief kernel panic. reset core or halt, depending on kernel_config
    \param none
    \retval none
*/
void panic();

/** \} */ // end of core_kernel group

#endif //!defined(LDS) && !defined(__ASSEMBLER__)

#endif // KERNEL_H
