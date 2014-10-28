/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
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

    SVC_TIMER_GET_UPTIME,
    SVC_TIMER_SECOND_PULSE,
    SVC_TIMER_HPET_TIMEOUT,
    SVC_TIMER_SETUP,

    SVC_IPC_POST,
    SVC_IPC_READ,
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
    SVC_STREAM_FLUSH,
    SVC_STREAM_DESTROY,

    SVC_DIRECT_READ,
    SVC_DIRECT_WRITE,

    SVC_BLOCK_CREATE,
    SVC_BLOCK_OPEN,
    SVC_BLOCK_CLOSE,
    SVC_BLOCK_SEND,
    SVC_BLOCK_SEND_IPC,
    SVC_BLOCK_RETURN,
    SVC_BLOCK_DESTROY,

    SVC_OBJECT_SET,
    SVC_OBJECT_GET,

    SVC_SETUP_SYSTEM,
    SVC_SETUP_DBG,
    SVC_TEST,

#if (KERNEL_MES)
    SVC_MUTEX_CREATE,
    SVC_MUTEX_LOCK,
    SVC_MUTEX_UNLOCK,
    SVC_MUTEX_DESTROY,

    SVC_EVENT_CREATE,
    SVC_EVENT_PULSE,
    SVC_EVENT_SET,
    SVC_EVENT_IS_SET,
    SVC_EVENT_CLEAR,
    SVC_EVENT_WAIT,
    SVC_EVENT_DESTROY,

    SVC_SEM_CREATE,
    SVC_SEM_WAIT,
    SVC_SEM_SIGNAL,
    SVC_SEM_DESTROY
#endif //KERNEL_MES

}SVC;

// will be aligned to pass MPU requirements
typedef struct {
    void* heap;
    void (*svc_irq)(unsigned int, unsigned int, unsigned int, unsigned int);
    const void** lib;
} GLOBAL;

#define __GLOBAL                                            ((GLOBAL*)(SRAM_BASE))

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
    \brief setup system process handle.
    \details Using system process handle you can communicate with system directly (in userspace).
     Implementation of protocol is system-based. All created processes will have __HEAP->system param setted up.
     For security reasons can be called only once after startup.
    \retval none
*/
__STATIC_INLINE void setup_system()
{
    svc_call(SVC_SETUP_SYSTEM, 0, 0, 0);
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
