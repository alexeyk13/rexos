/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KPROCESS_H
#define KPROCESS_H

#include "../userspace/process.h"
#include "ktimer.h"
#include "kernel_config.h"
#include "dbg.h"
#include "kipc.h"

typedef struct {
    void* data;
    unsigned int size;
}PROCESS_BLOCK;

typedef struct {
    DLIST list;                                                        //list of processes - active, frozen, or owned by sync object
    HEAP* heap;                                                        //process heap pointer
    unsigned int* sp;                                                  //current sp(if saved)
    MAGIC;
    unsigned int size;
    unsigned long flags;
    unsigned base_priority;                                            //base priority
    KTIMER timer;                                                      //timer for process sleep and sync objects timeouts
    void* sync_object;                                                 //sync object we are waiting for
#if (KERNEL_MES)
    unsigned current_priority;                                         //priority, adjusted by mutex
    DLIST* owned_mutexes;                                              //owned mutexes list for nested mutex priority inheritance
#endif //KERNEL_MES
#if (KERNEL_PROCESS_STAT)
    TIME uptime;
    TIME uptime_start;
#endif //KERNEL_PROCESS_STAT
    unsigned int blocks_count;
    PROCESS_BLOCK blocks[KERNEL_BLOCKS_COUNT];
    KIPC kipc;
    //IPC is following
}PROCESS;

//called from svc, IRQ disabled
void kprocess_create(const REX* rex, PROCESS** process);
void kprocess_set_flags(PROCESS* process, unsigned int flags);
void kprocess_unfreeze(PROCESS* process);
void kprocess_freeze(PROCESS* process);
void kprocess_set_priority(PROCESS* process, unsigned int priority);
void kprocess_destroy(PROCESS* process);
//cannot be called from init task, because init task is only task running, while other tasks are waiting or frozen
//this function can be call indirectly from any sync object.
//also called from sync objects
void kprocess_sleep_current(TIME* time, PROCESS_SYNC_TYPE sync_type, void *sync_object);

//called from svc, IRQ enabled
void kprocess_get_flags(PROCESS* process, unsigned int* flags);
void kprocess_get_priority(PROCESS* process, unsigned int* priority);

//called from other places in kernel, IRQ disabled
void kprocess_sleep(PROCESS* process, TIME* time, PROCESS_SYNC_TYPE sync_type, void *sync_object);
void kprocess_wakeup(PROCESS* process);
void kprocess_set_current_priority(PROCESS* process, unsigned int priority);
bool kprocess_block_open(PROCESS* process, void* data, unsigned int size);
void kprocess_block_close(PROCESS* process, void *data);
void kprocess_destroy_current();

//called from other places in kernel, IRQ enabled
bool kprocess_check_address(PROCESS* process, void* addr, unsigned int size);
bool kprocess_check_address_read(PROCESS* process, void* addr, unsigned int size);
void kprocess_error(PROCESS* process, int error);
void kprocess_error_current(int error);
PROCESS* kprocess_get_current();


//called from startup
void kprocess_init(const REX *rex);

#if (KERNEL_PROFILING)
//called from svc, IRQ disabled
void kprocess_switch_test();
void kprocess_info();
#endif //(KERNEL_PROFILING)

#endif // KPROCESS_H
