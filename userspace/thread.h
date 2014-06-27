/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef THREAD_H
#define THREAD_H

/** \addtogroup thread thread
    thread is the main object in kernel.

    thread can be in one of the following states:
    - active
    - waiting (for sync object)
    - frozen
    - waiting frozen

    After creation thread is in the frozen state

    Thread is also specified by:
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

typedef void (*THREAD_FUNCTION)(void*);

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
    unsigned int heap_size;
    unsigned int priority;
    THREAD_FUNCTION fn;
}THREAD_CALL;

/**
    \brief creates thread object. By default, thread is frozen after creation
    \param name: thread name
    \param stack_size: thread stack size in words. Must have enough space for context saving.
    Please, be carefull with this parameter: providing small stack may leave to overflows, providing
    large - to memory waste. Exact stack usage can be determined by turning on KERNEL_PROFILING.
    \param priority: thread priority. 0 - is highest priority, -idle- lowest. Only IDLE task can run on
    -idle- priority. Can be adjusted dynamically at any time.
    \param fn: thread function. Thread can't leave this function. In case of need of thread termination
    inside fn, thread_exit should be called.
    \param param: parameter for thread. Can be NULL

    \retval thread HANDLE on success, or INVALID_HANDLE on failure
*/
__STATIC_INLINE HANDLE thread_create(const char* name, unsigned int heap_size, unsigned int priority, THREAD_FUNCTION fn)
{
    THREAD_CALL tc;
    tc.name = name;
    tc.priority = priority;
    tc.heap_size = heap_size;
    tc.fn = fn;
    HANDLE handle;
    sys_call(SVC_THREAD_CREATE, (unsigned int)&tc, (unsigned int)&handle, 0);
    return handle;
}

/**
    \brief unfreeze thread
    \param thread: handle of created thread
    \retval none
*/
__STATIC_INLINE void thread_unfreeze(HANDLE thread)
{
    sys_call(SVC_THREAD_UNFREEZE, (unsigned int)thread, 0, 0);
}

/**
    \brief freeze thread
    \param thread: handle of created thread
    \retval none
*/
__STATIC_INLINE void thread_freeze(HANDLE thread)
{
    sys_call(SVC_THREAD_FREEZE, (unsigned int)thread, 0, 0);
}

/**
    \brief creates thread object and run it. See \ref thread_create for details
    \retval thread HANDLE on success, or INVALID_HANDLE on failure
*/
__STATIC_INLINE HANDLE thread_create_and_run(const char* name, int stack_size, unsigned int priority, THREAD_FUNCTION fn)
{
    HANDLE thread = thread_create(name, stack_size, priority, fn);
    thread_unfreeze(thread);
    return thread;
}

/**
    \brief return handle to currently running thread
    \retval handle to current thread
*/
__STATIC_INLINE HANDLE thread_get_current()
{
    return __HEAP->handle;
}

/**
    \brief get priority
    \param thread: handle of created thread
    \retval thread base priority
*/
__STATIC_INLINE unsigned int thread_get_priority(HANDLE thread)
{
    unsigned int priority;
    sys_call(SVC_THREAD_GET_PRIORITY, (unsigned int)thread, (unsigned int)&priority, 0);
    return priority;
}

/**
    \brief get current thread priority
    \param thread: handle of created thread
    \retval thread base priority
*/
__STATIC_INLINE unsigned int thread_get_current_priority()
{
    return thread_get_priority(__HEAP->handle);
}

/**
    \brief set priority
    \param thread: handle of created thread
    \param priority: new priority. 0 - highest, -idle- -1 - lowest. Cannot be call for IDLE thread.
    \retval none
*/
__STATIC_INLINE void thread_set_priority(HANDLE thread, unsigned int priority)
{
    sys_call(SVC_THREAD_SET_PRIORITY, (unsigned int)thread, priority, 0);
}

/**
    \brief set currently running thread priority
    \param priority: new priority. 0 - highest, -idle- -1 - lowest. Cannot be call for IDLE thread.
    \retval none
*/
__STATIC_INLINE void thread_set_current_priority(unsigned int priority)
{
    thread_set_priority(__HEAP->handle, priority);
}

/**
    \brief destroys thread
    \param thread: previously created thread
    \retval none
*/
__STATIC_INLINE void thread_destroy(HANDLE thread)
{
    sys_call(SVC_THREAD_DESTROY, (unsigned int)thread, 0, 0);
}

/**
    \brief destroys current thread
    \details this function should be called instead of leaving thread function
    \retval none
*/
__STATIC_INLINE void thread_exit()
{
    thread_destroy(__HEAP->handle);
}

/**
    \brief put current thread in waiting state
    \param time: pointer to TIME structure
    \retval none
*/
__STATIC_INLINE void sleep(TIME* time)
{
    sys_call(SVC_THREAD_SLEEP, (unsigned int)time, 0, 0);
}

/**
    \brief put current thread in waiting state
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
    \brief put current thread in waiting state
    \param us: time to sleep in microseconds
    \retval none
*/
__STATIC_INLINE void sleep_us(unsigned int us)
{
    TIME time;
    us_to_time(us, &time);
    sleep(&time);
}

/** \} */ // end of thread group

//TODO: move to core dbg
#if (KERNEL_PROFILING)
/** \addtogroup profiling profiling
  \ref KERNEL_PROFILING option should be set to 1
    \{
 */

/**
    \brief thread switch test
    \details simulate thread switching. This function can be used for
    perfomance measurement
    \retval none
*/
__STATIC_INLINE void thread_switch_test()
{
    sys_call(SVC_THREAD_SWITCH_TEST, 0, 0, 0);
}

/**
    \brief thread statistics
    \details print statistics over debug console for all active threads:
    - names
    - priority, active priority (can be temporally raised for sync objects)
    - stack usage: current/max/defined
    \retval none
*/
__STATIC_INLINE void thread_stat()
{
    sys_call(SVC_THREAD_STAT, 0, 0, 0);
}

/**
    \brief system stacks statistics
    \details list of stacks depends on architecture.
    print statistics over debug console for all active threads:
    - names
    - stack usage: current/max/defined
    \retval none
*/
__STATIC_INLINE void stack_stat()
{
    sys_call(SVC_STACK_STAT, 0, 0, 0);
}

/** \} */ // end of profiling group

#endif //(KERNEL_PROFILING)

#endif // THREAD_H
