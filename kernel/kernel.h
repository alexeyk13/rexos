/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KERNEL_H
#define KERNEL_H

/*
    kernel.h - MCU core specific functions. Kernel part
*/

/*! \mainpage Main index
 \tableofcontents
 RExOS documentation

    - \ref about
    - \ref gettingstarted
    - kernel
        - \ref process
        - \ref ipc
        - \ref stream
    - library
        - \ref lib_stdlib
        - \ref lib_stdio
    - userspace
        - \ref error
*/

#include "../userspace/core/core.h"
#include "kernel_config.h"

#ifndef KERNEL_GLOBAL_SIZE
//hack, but assembler safe
#define KERNEL_GLOBAL_SIZE                                  12
#endif

#define KERNEL_BASE                                         (SRAM_BASE + KERNEL_GLOBAL_SIZE)

#if !defined(LDS) && !defined(__ASSEMBLER__)

#include "kprocess_private.h"
#include "../lib/pool.h"
#include "../userspace/rb.h"
#include "../userspace/array.h"

#ifndef IRQ_VECTORS_COUNT
#error IRQ_VECTORS_COUNT is not decoded. Please specify it manually in Makefile
#endif

#ifdef ARM7
#include "core/arm7/core_arm7.h"
#elif defined(CORTEX_M)
#include "kcortexm.h"
#else
#error MCU core is not defined or not supported
#endif

// will be aligned to pass MPU requirements
typedef struct {
    //----------------------process specific-----------------------------
    //for context-switching
    //This values are used in asm context switching. Don't place them more than 128 bytes from start of KERNEL
    //now running process. (Active context).
    void* active_process;
    //next process to run, after leave. For context switch. If NULL - no context switch is required
    void* next_process;

    int kerror;
    //active processes
    KPROCESS* processes;
#if (KERNEL_PROCESS_STAT)
    KPROCESS* wait_processes;
#endif //(KERNEL_PROCESS_STAT)
#if (KERNEL_SVC_DEBUG)
    unsigned int num, param1, param2, param3;
    HANDLE call_process;
#endif //KERNEL_SVC_DEBUG
    //----------------------- printk support ---------------------------
    STDOUT stdout;
    void* stdout_param;

    //----------------------- IRQ related ------------------------------
    int context;
    //This values are used in asm. Don't place them more than 128 bytes from start of KERNEL
    KIRQ* irqs[IRQ_VECTORS_COUNT];
#ifdef SOFT_NVIC
    RB irq_pend_rb;
    int irq_pend_list[IRQ_VECTORS_COUNT];
    int irq_pend_list_size;
    char irq_pend_mask[(IRQ_VECTORS_COUNT + 7) / 8];
#endif
    //---------------------------- exodriver ---------------------------
    void* exo;

    //------------------------- timer specific -------------------------
    SYSTIME uptime;

    //callback for HPET timer
    CB_SVC_TIMER cb_ktimer;
    //callback param for HPET timer
    void* cb_ktimer_param;

    KTIMER* timers;
    //HPET value, set before call
    unsigned int hpet_value;
    //--------------------------- memory pools -------------------------
    ARRAY* pools;
    //-------------------------- kernel objects ------------------------
    HANDLE objects[KERNEL_OBJECTS_COUNT];
} KERNEL;

#define __KERNEL                                            ((KERNEL*)(KERNEL_BASE))
extern const char* const                                    __KERNEL_NAME;

extern const REX __APP;

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
extern void process_setup_context(KPROCESS* process, void (*fn)(void));

/**
    \brief fatal error handler - generally reset
    \param none
    \retval none
*/
extern void fatal();

/**
    \brief init exodriver related structures
    \param none
    \retval none
*/
extern void exodriver_init();

/**
    \brief process exodriver IPC
    \param ipc: user IPC
    \retval none
*/
extern void exodriver_post(IPC* ipc);

/**
    \brief delay in supervisor context
    \param us: time in us
    \retval none
*/
extern void exodriver_delay_us(unsigned int us);

/** \} */ // end of arch_porting group


/** \addtogroup kernel kernel part of core-related functionality
    \{
 */

/**
    \brief kernel panic. reset core or halt, depending on kernel_config
    \param none
    \retval none
*/
void panic();

/**
    \brief setup kernel dbg (if not already) from exodriver
    \param num: sys-call number
    \param param1: parameter 1. num-specific
    \param param2: parameter 2. num-specific
    \param param3: parameter 3. num-specific
    \retval none
*/
void kernel_setup_dbg(STDOUT stdout, void* param);

/**
    \brief main supervisor routine
    \details All calls to supervisor are implmented by this function

    \param num: sys-call number
    \param param1: parameter 1. num-specific
    \param param2: parameter 2. num-specific
    \param param3: parameter 3. num-specific
    \retval none
*/
void svc(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3);

/**
    \brief start point of kernel, that must be called after low-level initialization
    \param none
    \retval none
*/
void startup();

/** \} */ // end of core_kernel group

#endif //!defined(LDS) && !defined(__ASSEMBLER__)

#endif // KERNEL_H
