/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KMUTEX_H
#define KMUTEX_H

#include "../userspace/lib/dlist.h"
#include "kprocess.h"
#include "dbg.h"

typedef struct {
    DLIST list;
    MAGIC;
    //only one item
    PROCESS* owner;
    //list
    PROCESS* waiters;
}MUTEX;

//update owner priority according lowest priority of owner's all waiters of all mutexes
//can be called from process_private.c on base priority update
//also called internally on mutex unlock
unsigned int kmutex_calculate_owner_priority(PROCESS* process);
//release lock, acquired by mutex. Called from process_private to release
//locked object - by timeout or process termination. also can be called on normal release
void kmutex_lock_release(MUTEX* mutex, PROCESS* process);

//called from svc
void kmutex_create(MUTEX** mutex);
void kmutex_lock(MUTEX* mutex, TIME* time);
void kmutex_unlock(MUTEX* mutex);
void kmutex_destroy(MUTEX* mutex);

#endif // KMUTEX_H
