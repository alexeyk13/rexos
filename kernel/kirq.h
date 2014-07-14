/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KIRQ_H
#define KIRQ_H

#include "../userspace/lib/types.h"
#include "kprocess.h"

typedef struct {
    IRQ handler;
    void* param;
    PROCESS* process;
}KIRQ;

typedef enum {
    USER_CONTEXT,
    SVC_CONTEXT,
    IRQ_CONTEXT
}CONTEXT;

//called from kernel startup
void kirq_init();

//called from low-level startup
void kirq_stub(int vector, void* param);
void kirq_enter(int vector);

//called from svc
void kirq_register(int vector, IRQ handler, void* param);
void kirq_unregister(int vector);

//called from kernel
CONTEXT get_context();

#endif // KIRQ_H
