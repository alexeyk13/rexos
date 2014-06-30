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

#ifdef ARM7
#include "arm7/core_arm7.h"
#elif defined(CORTEX_M3) || defined(CORTEX_M0) || defined(CORTEX_M1) || defined(CORTEX_M4)
#define CORTEXM
#define NVIC_PRESENT
#include "cortexm/core_cortexm.h"
#else
#error MCU core is not defined or not supported
#endif

#if !defined(LDS) && !defined(__ASSEMBLER__)

#include "../cc_macro.h"
#include "../types.h"
#include "sys_calls.h"

typedef enum {
    USER_CONTEXT =                                          0x1,
    SUPERVISOR_CONTEXT =                                    0x4,
    IRQ_CONTEXT =                                           0x8
} CONTEXT;

// will be aligned to pass MPU requirements
typedef struct {
    void* heap;
    void (*svc_irq)(unsigned int, unsigned int, unsigned int, unsigned int);
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
    \brief core-dependent context query
    \details Return value of current context.

    USER_CONTEXT =                                          0x1,
    SUPERVISOR_CONTEXT =                                    0x4,
    IRQ_CONTEXT =                                           0x8

    \retval result context
*/
extern CONTEXT get_context();

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
    this core-specific function. After calling, return value is provided.

    For example, for cortex-m3 "svc 0x12" instruction is used.

    \param num: sys-call number
    \param param1: parameter 1. num-specific
    \param param2: parameter 2. num-specific
    \param param3: parameter 3. num-specific
    \retval result value. num-specific
*/
void do_sys_call(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3);
/** \} */ // end of core_porting group

void sys_call(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3);

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
