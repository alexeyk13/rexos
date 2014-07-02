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

void kirq_init();
void kirq_stub(int vector, void* param);
void kirq_enter(int vector);

void kirq_register(int vector, IRQ handler, void* param);
void kirq_unregister(int vector);

#endif // KIRQ_H
