/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SVC_H
#define SVC_H

/*
    svc.h - supervisor and core related part in userspace
*/

#include "core/core.h"

#if !defined(LDS) && !defined(__ASSEMBLER__)

#include "cc_macro.h"

/*
    List of all calls to supervisor
 */

typedef enum {
    SVC_PROCESS_CREATE,
    SVC_PROCESS_GET_CURRENT,
    SVC_PROCESS_GET_FLAGS,
    SVC_PROCESS_SET_FLAGS,
    SVC_PROCESS_GET_PRIORITY,
    SVC_PROCESS_SET_PRIORITY,
    SVC_PROCESS_DESTROY,
    SVC_PROCESS_SLEEP,
    //profiling
    SVC_PROCESS_SWITCH_TEST,
    SVC_PROCESS_INFO,

    SVC_IRQ_REGISTER,
    SVC_IRQ_UNREGISTER,

    SVC_SYSTIME_GET_UPTIME,
    SVC_SYSTIME_SECOND_PULSE,
    SVC_SYSTIME_HPET_TIMEOUT,
    SVC_SYSTIME_HPET_SETUP,
    SVC_SYSTIME_SOFT_TIMER_CREATE,
    SVC_SYSTIME_SOFT_TIMER_START,
    SVC_SYSTIME_SOFT_TIMER_STOP,
    SVC_SYSTIME_SOFT_TIMER_DESTROY,

    SVC_IPC_POST,
    SVC_IPC_WAIT,
    SVC_IPC_CALL,

    SVC_STREAM_CREATE,
    SVC_STREAM_OPEN,
    SVC_STREAM_CLOSE,
    SVC_STREAM_GET_SIZE,
    SVC_STREAM_GET_FREE,
    SVC_STREAM_LISTEN,
    SVC_STREAM_STOP_LISTEN,
    SVC_STREAM_WRITE,
    SVC_STREAM_READ,
    SVC_STREAM_WRITE_NO_BLOCK,
    SVC_STREAM_READ_NO_BLOCK,
    SVC_STREAM_FLUSH,
    SVC_STREAM_DESTROY,

    SVC_IO_CREATE,
    SVC_IO_DESTROY,

    SVC_OBJECT_SET,
    SVC_OBJECT_GET,

    SVC_HEAP_CREATE,
    SVC_HEAP_DESTROY,
    SVC_HEAP_MALLOC,
    SVC_HEAP_FREE,

    SVC_ADD_POOL,
    SVC_SETUP_DBG,
    SVC_PRINTD,
    SVC_TEST
}SVC;


typedef void (*STDOUT)(const char *const buf, unsigned int size, void* param);

/** \addtogroup core_porting core porting
    \{
 */

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
    \details If current contex is not enough during svc_call, context is raised, using
    this core-specific function.

    For example, for cortex-m3 "svc 0x12" instruction is used.

    \param num: sys-call number
    \param param1: parameter 1. num-specific
    \param param2: parameter 2. num-specific
    \param param3: parameter 3. num-specific
    \retval none
*/
extern void svc_call(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3);

/** \} */ // end of core_porting group

/** \addtogroup sys sys
    \{
 */

/**
    \brief add pool
    \param base: base address
    \param size: pool size
    \retval none
*/
__STATIC_INLINE void svc_add_pool(unsigned int base, unsigned int size)
{
    svc_call(SVC_ADD_POOL, base, size, 0);
}


/**
    \brief setup kernel stdout for debug reasons
    \details It's second exception, where kernel make direct call to userspace. Make sure call is safe.
     For security reasons can be called only once after startup.
    \param stdout: pointer to function, called on printf/putc
    \param param: param, sended during calll
    \retval none
*/
__STATIC_INLINE void setup_dbg(STDOUT stdout, void* param)
{
    svc_call(SVC_SETUP_DBG, (unsigned int)stdout, (unsigned int)param, 0);
}

/**
    \brief test kernel call
    \details Call to kernel and immediate return
    \retval none
*/
__STATIC_INLINE void svc_test()
{
    svc_call(SVC_TEST, 0, 0, 0);
}

/** \} */ // end of sys group

#endif //!defined(LDS) && !defined(__ASSEMBLER__)

#endif // SVC_H
