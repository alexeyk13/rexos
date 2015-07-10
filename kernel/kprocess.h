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
#if (KERNEL_BD)
#include "kblock.h"
#endif //KERNEL_BD

typedef struct _PROCESS {
    DLIST list;                                                        //list of processes - active, frozen, or owned by sync object
    HEAP* heap;                                                        //process heap pointer
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
#if (KERNEL_BD)
    BLOCK* blocks;
#endif //KERNEL_BD
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

//called from svc, IRQ enabled
void kprocess_get_flags(PROCESS* process, unsigned int* flags);
void kprocess_get_priority(PROCESS* process, unsigned int* priority);
void kprocess_get_current_svc(PROCESS** var);

//called from other places in kernel, IRQ disabled
void kprocess_sleep(PROCESS* process, SYSTIME* time, PROCESS_SYNC_TYPE sync_type, void *sync_object);
void kprocess_wakeup(PROCESS* process);
void kprocess_set_current_priority(PROCESS* process, unsigned int priority);

#if (KERNEL_BD)
__STATIC_INLINE void kprocess_block_open(PROCESS* process, BLOCK* block)
{
    dlist_add_tail((DLIST**)&process->blocks, (DLIST*)block);
}

__STATIC_INLINE void kprocess_block_close(PROCESS* process, BLOCK* block)
{
    dlist_remove((DLIST**)&process->blocks, (DLIST*)block);
}
#endif //KERNEL_BD

void kprocess_destroy_current();

//called from other places in kernel, IRQ enabled
bool kprocess_check_address(PROCESS* process, void* addr, unsigned int size);
bool kprocess_check_address_read(PROCESS* process, void* addr, unsigned int size);
void kprocess_error(PROCESS* process, int error);
void kprocess_error_current(int error);
PROCESS* kprocess_get_current();

__STATIC_INLINE void* kprocess_struct_ptr(PROCESS* process, HEAP_STRUCT_TYPE struct_type)
{
    return ((const LIB_HEAP*)__GLOBAL->lib[LIB_ID_HEAP])->__heap_struct_ptr(process->heap, struct_type);
}

__STATIC_INLINE char* kprocess_name(PROCESS* process)
{
    return ((const LIB_HEAP*)__GLOBAL->lib[LIB_ID_HEAP])->__process_name(process->heap);
}



//called from startup
void kprocess_init(const REX *rex);

#if (KERNEL_PROFILING)
//called from svc, IRQ disabled
void kprocess_switch_test();
void kprocess_info();
#endif //(KERNEL_PROFILING)

#endif // KPROCESS_H
