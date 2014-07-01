/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SVC_MUTEX_H
#define SVC_MUTEX_H

#include "../userspace/dlist.h"
#include "svc_process.h"
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
unsigned int svc_mutex_calculate_owner_priority(PROCESS* process);
//release lock, acquired by mutex. Called from process_private to release
//locked object - by timeout or process termination. also can be called on normal release
void svc_mutex_lock_release(MUTEX* mutex, PROCESS* process);

//called from svc
void svc_mutex_create(MUTEX** mutex);
void svc_mutex_lock(MUTEX* mutex, TIME* time);
void svc_mutex_unlock(MUTEX* mutex);
void svc_mutex_destroy(MUTEX* mutex);

#endif // SVC_MUTEX_H
