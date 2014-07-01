/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KSEM_H
#define KSEM_H

#include "kprocess.h"
#include "dbg.h"

typedef struct {
    MAGIC;
    int value;
    PROCESS* waiters;
}SEM;

//called from process_private.c on destroy or timeout
void ksem_lock_release(SEM* sem, PROCESS* process);

//called from svc
void ksem_create(SEM** sem);
void ksem_signal(SEM* sem);
void ksem_wait(SEM* sem, TIME* time);
void ksem_destroy(SEM* sem);

#endif // KSEM_H
