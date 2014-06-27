/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TASK_KERNEL_H
#define TASK_KERNEL_H

#include "../userspace/thread.h"
#include "../userspace/cc_macro.h"
#include "svc_timer.h"
#include "kernel_config.h"
#include "dbg.h"

typedef enum {
    THREAD_SYNC_TIMER_ONLY =    (0x0 << 4),
    THREAD_SYNC_MUTEX =         (0x1 << 4),
    THREAD_SYNC_EVENT =         (0x2 << 4),
    THREAD_SYNC_SEM =           (0x3 << 4)
}THREAD_SYNC_TYPE;

typedef struct {
    DLIST list;                                                        //list of threads - active, frozen, or owned by sync object
    HEAP* heap;                                                        //process heap pointer
    unsigned int* sp;                                                  //current sp(if saved)
    MAGIC;
    unsigned long flags;
    unsigned base_priority;                                            //base priority
    unsigned current_priority;                                         //priority, adjusted by mutex
    TIMER timer;                                                       //timer for thread sleep and sync objects timeouts
    void* sync_object;                                                 //sync object we are waiting for
    DLIST* owned_mutexes;                                              //owned mutexes list for nested mutex priority inheritance
    //TODO all to userspace
#if (KERNEL_PROFILING)
    TIME uptime;
    TIME uptime_start;
    int heap_size;
#endif //KERNEL_PROFILING
}THREAD;

void svc_thread_handler(unsigned int num, unsigned int param1, unsigned int param2);
void thread_init();

//for sync-object internal calls (usually mutexes)
void svc_thread_set_current_priority(THREAD* thread, unsigned int priority);
void svc_thread_restore_current_priority(THREAD* thread);
//cannot be call from IRQ context, because this will freeze current system thread
//also cannot be called from idle task, because idle task is only task running, while other tasks are waiting or frozen
//this function can be call indirectly from any sync object.
void svc_thread_sleep(TIME* time, THREAD_SYNC_TYPE sync_type, void* sync_object);
void svc_thread_wakeup(THREAD* thread);

void svc_thread_error(THREAD* thread, int error);
THREAD* svc_thread_get_current();
void svc_thread_destroy_current();

/** \addtogroup user_provided user provided functions
    \{
 */
/**
    \brief user-provided idle task
    \details This task runs, while other are frozen or waiting.
    It can't be destroyed, can't be frozen or put in wait state -
    direct or indirect.

    minimal implementation will looks like:

    void idle_task(void)
    {
        for (;;) {}
    }

    however, for power saving for cortex-m3 recommended minimal following task:

    void idle_task(void)
    {
        for (;;) {__WFI();}
    }

    \retval none
*/
extern void idle_task(void);
/** \} */ // end of user_provided group

#endif // TASK_KERNEL_H
