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

typedef enum {
    PROCESS_SYNC_TIMER_ONLY =    (0x0 << 4),
    PROCESS_SYNC_MUTEX =         (0x1 << 4),
    PROCESS_SYNC_EVENT =         (0x2 << 4),
    PROCESS_SYNC_SEM =           (0x3 << 4)
}PROCESS_SYNC_TYPE;

typedef struct {
    DLIST list;                                                        //list of processes - active, frozen, or owned by sync object
    HEAP* heap;                                                        //process heap pointer
    unsigned int* sp;                                                  //current sp(if saved)
    MAGIC;
    unsigned long flags;
    unsigned base_priority;                                            //base priority
    unsigned current_priority;                                         //priority, adjusted by mutex
    TIMER timer;                                                       //timer for process sleep and sync objects timeouts
    void* sync_object;                                                 //sync object we are waiting for
    DLIST* owned_mutexes;                                              //owned mutexes list for nested mutex priority inheritance
    //TODO all to userspace
#if (KERNEL_PROFILING)
    TIME uptime;
    TIME uptime_start;
    int heap_size;
#endif //KERNEL_PROFILING
}PROCESS;

void svc_process_handler(unsigned int num, unsigned int param1, unsigned int param2);
void svc_process_init(const REX *rex);

//for sync-object internal calls (usually mutexes)
void svc_process_set_current_priority(PROCESS* process, unsigned int priority);
void svc_process_restore_current_priority(PROCESS* process);
//cannot be call from IRQ context, because this will freeze current system process
//also cannot be called from idle task, because idle task is only task running, while other tasks are waiting or frozen
//this function can be call indirectly from any sync object.
void svc_process_sleep(TIME* time, PROCESS_SYNC_TYPE sync_type, void* sync_object);
void svc_process_wakeup(PROCESS* process);

void svc_process_error(PROCESS* process, int error);
PROCESS* svc_process_get_current();
void svc_process_destroy_current();

#endif // SVC_PROCESS_H
