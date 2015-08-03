/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KPROCESS_H
#define KPROCESS_H

#include "../userspace/process.h"
#include "ksystime.h"
#include "kernel_config.h"
#include "dbg.h"
#include "kipc.h"

typedef struct _PROCESS {
    DLIST list;                                                        //list of processes - active, frozen, or owned by sync object
    PROCESS* heap;                                                     //process userspace data pointer
    unsigned int* sp;                                                  //current sp(if saved)
    MAGIC;
    unsigned int size;
    unsigned long flags;
    unsigned base_priority;                                            //base priority
    KTIMER timer;                                                      //timer for process sleep and sync objects timeouts
    void* sync_object;                                                 //sync object we are waiting for
#if (KERNEL_PROCESS_STAT)
    SYSTIME uptime;
    SYSTIME uptime_start;
#endif //KERNEL_PROCESS_STAT
    KIPC kipc;
    //IPC is following
}KPROCESS;

//called from svc, IRQ disabled
void kprocess_create(const REX* rex, KPROCESS** kprocess);
void kprocess_set_flags(KPROCESS* kprocess, unsigned int flags);
void kprocess_unfreeze(KPROCESS* kprocess);
void kprocess_freeze(KPROCESS* kprocess);
void kprocess_set_priority(KPROCESS* kprocess, unsigned int priority);
void kprocess_destroy(KPROCESS* kprocess);

//called from svc, IRQ enabled
void kprocess_get_flags(KPROCESS* kprocess, unsigned int* flags);
void kprocess_get_priority(KPROCESS* kprocess, unsigned int* priority);
void kprocess_get_current_svc(KPROCESS** var);

//called from other places in kernel, IRQ disabled
void kprocess_sleep(KPROCESS* kprocess, SYSTIME* time, PROCESS_SYNC_TYPE sync_type, void *sync_object);
void kprocess_wakeup(KPROCESS* kprocess);
void kprocess_set_current_priority(KPROCESS* kprocess, unsigned int priority);

void kprocess_destroy_current();

//called from other places in kernel, IRQ enabled
bool kprocess_check_address(KPROCESS* kprocess, void* addr, unsigned int size);
bool kprocess_check_address_read(KPROCESS* kprocess, void* addr, unsigned int size);
void kprocess_error(KPROCESS* kprocess, int error);
void kprocess_error_current(int error);
KPROCESS* kprocess_get_current();
const char* kprocess_name(KPROCESS* kprocess);

//called from startup
void kprocess_init(const REX *rex);

#if (KERNEL_PROFILING)
//called from svc, IRQ disabled
void kprocess_switch_test();
void kprocess_info();
#endif //(KERNEL_PROFILING)

#endif // KPROCESS_H
