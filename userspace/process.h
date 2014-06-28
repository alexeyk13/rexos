/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef PROCESS_H
#define PROCESS_H

/** \addtogroup process process
    process is the main object in kernel.

    process can be in one of the following states:
    - active
    - waiting (for sync object)
    - frozen
    - waiting frozen

    After creation process is in the frozen state

    Process is also specified by:
    - it's name
    - priority
    - stack size
    \{
 */

#include "kernel_config.h"
#include "core/core.h"
#include "core/sys_calls.h"
#include "../lib/pool.h"
#include "../lib/time.h"

typedef struct {
    //header size including name
    int struct_size;
    POOL pool;
    int error;
    //self handle
    HANDLE handle;
    //name is following
} HEAP;

#define PROCESS_NAME(heap)                              ((char*)((unsigned int)heap + sizeof(HEAP)))

#define __PROCESS_NAME                                  (PROCESS_NAME(__HEAP))

typedef struct {
    const char* name;
    unsigned int size;
    unsigned int priority;
    unsigned int flags;
    void (*fn) (void);
}REX;

/**
    \brief creates process object. By default, process is frozen after creation
    \param rex: pointer to process header
    \retval process HANDLE on success, or INVALID_HANDLE on failure
*/
__STATIC_INLINE HANDLE process_create(const REX* rex)
{
    HANDLE handle;
    sys_call(SVC_PROCESS_CREATE, (unsigned int)rex, (unsigned int)&handle, 0);
    return handle;
}

/**
    \brief unfreeze process
    \param process: handle of created process
    \retval none
*/
__STATIC_INLINE void process_unfreeze(HANDLE process)
{
    sys_call(SVC_PROCESS_UNFREEZE, (unsigned int)process, 0, 0);
}

/**
    \brief freeze process
    \param process: handle of created process
    \retval none
*/
__STATIC_INLINE void process_freeze(HANDLE process)
{
    sys_call(SVC_PROCESS_FREEZE, (unsigned int)process, 0, 0);
}

/**
    \brief creates process object and run it. See \ref process_create for details
    \retval process HANDLE on success, or INVALID_HANDLE on failure
*/
__STATIC_INLINE HANDLE process_create_and_run(const REX* rex)
{
    HANDLE process = process_create(rex);
    process_unfreeze(process);
    return process;
}

/**
    \brief return handle to currently running process
    \retval handle to current process
*/
__STATIC_INLINE HANDLE process_get_current()
{
    return __HEAP->handle;
}

/**
    \brief get priority
    \param process: handle of created process
    \retval process base priority
*/
__STATIC_INLINE unsigned int process_get_priority(HANDLE process)
{
    unsigned int priority;
    sys_call(SVC_PROCESS_GET_PRIORITY, (unsigned int)process, (unsigned int)&priority, 0);
    return priority;
}

/**
    \brief get current process priority
    \param process: handle of created process
    \retval process base priority
*/
__STATIC_INLINE unsigned int process_get_current_priority()
{
    return process_get_priority(__HEAP->handle);
}

/**
    \brief set priority
    \param process: handle of created process
    \param priority: new priority. 0 - highest, init -1 - lowest. Cannot be called for init process.
    \retval none
*/
__STATIC_INLINE void process_set_priority(HANDLE process, unsigned int priority)
{
    sys_call(SVC_PROCESS_SET_PRIORITY, (unsigned int)process, priority, 0);
}

/**
    \brief set currently running process priority
    \param priority: new priority. 0 - highest, init -1 - lowest. Cannot be called for init process.
    \retval none
*/
__STATIC_INLINE void process_set_current_priority(unsigned int priority)
{
    process_set_priority(__HEAP->handle, priority);
}

/**
    \brief destroys process
    \param process: previously created process
    \retval none
*/
__STATIC_INLINE void process_destroy(HANDLE process)
{
    sys_call(SVC_PROCESS_DESTROY, (unsigned int)process, 0, 0);
}

/**
    \brief destroys current process
    \details this function should be called instead of leaving process function
    \retval none
*/
__STATIC_INLINE void process_exit()
{
    process_destroy(__HEAP->handle);
}

/**
    \brief put current process in waiting state
    \param time: pointer to TIME structure
    \retval none
*/
__STATIC_INLINE void sleep(TIME* time)
{
    sys_call(SVC_PROCESS_SLEEP, (unsigned int)time, 0, 0);
}

/**
    \brief put current process in waiting state
    \param ms: time to sleep in milliseconds
    \retval none
*/
__STATIC_INLINE void sleep_ms(unsigned int ms)
{
    TIME time;
    ms_to_time(ms, &time);
    sleep(&time);
}

/**
    \brief put current process in waiting state
    \param us: time to sleep in microseconds
    \retval none
*/
__STATIC_INLINE void sleep_us(unsigned int us)
{
    TIME time;
    us_to_time(us, &time);
    sleep(&time);
}

/** \} */ // end of process group

#endif // PROCESS_H
