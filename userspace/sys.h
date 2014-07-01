/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef CORE_H
#define CORE_H

/*
    core.h - MCU core specific functions
*/

#include "core/stm32.h"

#ifdef ARM7
#include "core/arm7/core_arm7.h"
#elif defined(CORTEX_M3) || defined(CORTEX_M0) || defined(CORTEX_M1) || defined(CORTEX_M4)
#include "core/cortexm.h"
#else
#error MCU core is not defined or not supported
#endif

#if !defined(LDS) && !defined(__ASSEMBLER__)

#include "cc_macro.h"
#include "lib/types.h"
#include "../lib/lib.h"

/*
    List of all calls to supervisor
 */

typedef enum {
    SVC_PROCESS = 0x0,
    SVC_PROCESS_CREATE,
    SVC_PROCESS_GET_FLAGS,
    SVC_PROCESS_SET_FLAGS,
    SVC_PROCESS_GET_PRIORITY,
    SVC_PROCESS_SET_PRIORITY,
    SVC_PROCESS_DESTROY,
    SVC_PROCESS_SLEEP,
    //profiling
    SVC_PROCESS_SWITCH_TEST,
    SVC_PROCESS_INFO,

    SVC_MUTEX = 0x100,
    SVC_MUTEX_CREATE,
    SVC_MUTEX_LOCK,
    SVC_MUTEX_UNLOCK,
    SVC_MUTEX_DESTROY,

    SVC_EVENT = 0x200,
    SVC_EVENT_CREATE,
    SVC_EVENT_PULSE,
    SVC_EVENT_SET,
    SVC_EVENT_IS_SET,
    SVC_EVENT_CLEAR,
    SVC_EVENT_WAIT,
    SVC_EVENT_DESTROY,

    SVC_SEM = 0x300,
    SVC_SEM_CREATE,
    SVC_SEM_WAIT,
    SVC_SEM_SIGNAL,
    SVC_SEM_DESTROY,

    SVC_TIMER = 0x500,
    SVC_TIMER_GET_UPTIME,
    SVC_TIMER_SECOND_PULSE,
    SVC_TIMER_HPET_TIMEOUT,
    SVC_TIMER_SETUP,

    SVC_OTHER = 0x700,
    SVC_SETUP_STDOUT,
    SVC_SETUP_STDIN,
    SVC_SETUP_DBG
}SVC;

// will be aligned to pass MPU requirements
typedef struct {
    void* heap;
    void (*svc_irq)(unsigned int, unsigned int, unsigned int, unsigned int);
    const LIB* lib;
} GLOBAL;

#define __GLOBAL                                            ((GLOBAL*)(SRAM_BASE))
#define __HEAP                                              ((HEAP*)(((GLOBAL*)(SRAM_BASE))->heap))


#define NON_REENTERABLE_ENTER(var)                          int __state = disable_interrupts(); \
                                                            if (!var) \
                                                            {   var = true; \
                                                                restore_interrupts(__state);

#define NON_REENTERABLE_EXIT(var)                               disable_interrupts(); \
                                                                var = false;\
                                                            }\
                                                            restore_interrupts(__state);

#define CRITICAL_ENTER                                      int __state = disable_interrupts()
#define CRITICAL_ENTER_AGAIN                                disable_interrupts()
#define CRITICAL_LEAVE                                      restore_interrupts(__state);

/** \addtogroup core_porting core porting
    \{
 */

/**
    \brief core-dependent interrupt disabler
    \details Save interrupt state and disable interrupts

    \retval saved interrupt states. core-specific
*/
extern int disable_interrupts();

/**
    \brief core-dependent interrupt restorer
    \details Restore previously save interrupts

    \param state: core-specific interrupts state
    \retval none
*/
extern void restore_interrupts(int state);

/**
    \brief arch-dependent stack pointer query
    \details Same for every ARM, so defined here
    \retval stack pointer
*/
__STATIC_INLINE void* get_sp()
{
  void* result;
  __ASM volatile ("mov %0, sp" : "=r" (result));
  return result;
}

/**
    \brief core-dependent context raiser
    \details If current contex is not enough during sys_call, context is raised, using
    this core-specific function.

    For example, for cortex-m3 "svc 0x12" instruction is used.

    \param num: sys-call number
    \param param1: parameter 1. num-specific
    \param param2: parameter 2. num-specific
    \param param3: parameter 3. num-specific
    \retval none
*/
extern void sys_call(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3);

/** \} */ // end of core_porting group

/** \addtogroup sys sys
    \{
 */

/**
    \brief setup kernel stdout for debug reasons
    \details It's seconde exception, where kernel make direct call to userspace. Make sure call is safe.
     For security reasons can be called only once after startup.
    \param stdout: pointer to function, called on printf/putc
    \param param: param, sended during calll
    \retval none
*/
__STATIC_INLINE void setup_dbg(STDOUT stdout, void* param)
{
    sys_call(SVC_SETUP_DBG, (unsigned int)stdout, (unsigned int)param, 0);
}


/**
    \brief setup global stdout for newly created processes
    \details stdout must be in LIB address space, or MPU protection will cause exception
    \param stdout: pointer to function, called on printf/putc
    \param param: param, sended during calll
    \retval none
*/
__STATIC_INLINE void setup_stdout(STDOUT stdout, void* param)
{
    sys_call(SVC_SETUP_STDOUT, (unsigned int)stdout, (unsigned int)param, 0);
}

/**
    \brief setup global stdin for newly created processes
    \details stdin must be in LIB address space, or MPU protection will cause exception
    \param stdin: pointer to function, called on scanf/getc
    \param param: param, sended during calll
    \retval none
*/
__STATIC_INLINE void setup_stdin(STDIN stdin, void* param)
{
    sys_call(SVC_SETUP_STDIN, (unsigned int)stdin, (unsigned int)param, 0);
}

/** \} */ // end of sys group

#endif //!defined(LDS) && !defined(__ASSEMBLER__)

#endif // CORE_H
