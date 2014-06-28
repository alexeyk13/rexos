/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SVC_PROCESS_H
#define SVC_PROCESS_H

#include "../userspace/process.h"
#include "svc_timer.h"
#include "kernel_config.h"
#include "dbg.h"

typedef struct {
    DLIST list;                                                        //list of processes - active, frozen, or owned by sync object
    HEAP* heap;                                                        //process heap pointer
    unsigned int* sp;                                                  //current sp(if saved)
    MAGIC;
    unsigned int size;
    unsigned long flags;
    unsigned base_priority;                                            //base priority
    unsigned current_priority;                                         //priority, adjusted by mutex
    TIMER timer;                                                       //timer for process sleep and sync objects timeouts
    void* sync_object;                                                 //sync object we are waiting for
    DLIST* owned_mutexes;                                              //owned mutexes list for nested mutex priority inheritance
#if (KERNEL_PROCESS_STAT)
    TIME uptime;
    TIME uptime_start;
#endif //KERNEL_PROCESS_STAT
}PROCESS;

//called from svc
void svc_process_create(const REX* rex, PROCESS** process);
void svc_process_get_flags(PROCESS* process, unsigned int* flags);
void svc_process_set_flags(PROCESS* process, unsigned int flags);
void svc_process_unfreeze(PROCESS* process);
void svc_process_freeze(PROCESS* process);
void svc_process_set_priority(PROCESS* process, unsigned int priority);
void svc_process_get_priority(PROCESS* process, unsigned int* priority);
void svc_process_destroy(PROCESS* process);
//cannot be called from init task, because init task is only task running, while other tasks are waiting or frozen
//this function can be call indirectly from any sync object.
//also called from sync objects
void svc_process_sleep(TIME* time, PROCESS_SYNC_TYPE sync_type, void *sync_object);

//called from other places in kernel
void svc_process_wakeup(PROCESS* process);
void svc_process_set_current_priority(PROCESS* process, unsigned int priority);
void svc_process_error(PROCESS* process, int error);
PROCESS* svc_process_get_current();
void svc_process_destroy_current();

//called from startup
void svc_process_init(const REX *rex);

#if (KERNEL_PROFILING)
void svc_process_switch_test();
void svc_process_info();
#endif //(KERNEL_PROFILING)

#endif // SVC_PROCESS_H
