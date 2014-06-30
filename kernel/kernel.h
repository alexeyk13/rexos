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

#include "svc_process.h"
#include "../lib/pool.h"

#include "../arch/cortex_m3/stm/uart_stm32.h"
#include "../drv_if/gpio.h"
#include "../arch/cortex_m3/stm/timer_stm32.h"
#include "../userspace/timer.h"

// will be aligned to pass MPU requirements
typedef struct {
    //first 5 params same as userspace for library calls error handling
    //header size including name
    int struct_size;
    POOL pool;
    int error;
    STDOUT stdout;
    void* stdout_param;
    bool dbg_locked;

    STDOUT stdout_global;
    void* stdout_global_param;
    STDIN stdin_global;
    void* stdin_global_param;

    //----------------------process specific-----------------------------
    //for context-switching
    //This values are used in asm context switching. Don't place them more than 128 bytes from start of KERNEL
    //now running process. (Active context).
    volatile PROCESS* active_process;
    //next process to run, after leave. For context switch. If NULL - no context switch is required
    volatile PROCESS* next_process;

    //active processes
    PROCESS* processes;
#if (KERNEL_PROCESS_STAT)
    PROCESS* wait_processes;
#endif //(KERNEL_PROCESS_STAT)
    //init process. Always active.
    PROCESS* init;

    //------------------------- timer specific -------------------------
    TIME uptime;

    //callback for HPET timer
    CB_SVC_TIMER cb_svc_timer;

    TIMER* timers;
    volatile bool timer_executed;
    //HPET value, set before call
    unsigned int hpet_value;
    //----------------------- paged memory related ---------------------
    POOL paged;

    //--- move this shit later to userspace
    unsigned long core_freq;
    unsigned long fs_freq;
    unsigned long ahb_freq;
    unsigned long apb1_freq;
    unsigned long apb2_freq;
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

    - if (_active_process) is not NULL, save current process context on _active_process stack
    - load current process context from _next_process stack
    - adjust stack pointer with _next_process.cur_sp
    - move _next_process - > _active_process, _next_process = NULL
    \retval none
*/
extern void pend_switch_context(void);
/**
    \brief setup initial context
    \details Just setup context. Real switching will be performed on pend_switch_context, when
    _next_process == process.
    \param process: process handle
    \param fn: process start point
    \retval none
*/
extern void process_setup_context(PROCESS* process, void (*fn)(void));

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
