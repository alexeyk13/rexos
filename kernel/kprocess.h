/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KPROCESS_H
#define KPROCESS_H

#include "../userspace/process.h"
#include "kernel_config.h"
#include "dbg.h"

typedef struct _KPROCESS KPROCESS;

HANDLE kprocess_create(const REX* rex);
void kprocess_set_flags(HANDLE p, unsigned int flags);
void kprocess_set_priority(HANDLE p, unsigned int priority);
void kprocess_destroy(HANDLE p);
unsigned int kprocess_get_flags(HANDLE p);
unsigned int kprocess_get_priority(HANDLE p);
void kprocess_sleep(HANDLE p, SYSTIME* time, PROCESS_SYNC_TYPE sync_type, HANDLE sync_object);
bool kprocess_check_address(HANDLE p, void* addr, unsigned int size);
void kprocess_error(HANDLE p, int error);
HANDLE kprocess_get_current();
const char* kprocess_name(HANDLE p);

//called while IRQ disabled
void kprocess_wakeup(HANDLE p);

//called from startup
void kprocess_init(const REX *rex);

#if (KERNEL_PROFILING)
//called from svc, IRQ disabled
void kprocess_switch_test();
void kprocess_info();
#endif //(KERNEL_PROFILING)

#endif // KPROCESS_H
