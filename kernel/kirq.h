/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KIRQ_H
#define KIRQ_H

#include "../userspace/irq.h"
#include "kprocess.h"
#include "../userspace/core/core.h"
#include "kernel_config.h"

typedef struct {
    IRQ handler;
    void* param;
    KPROCESS* process;
#ifdef SOFT_NVIC
    bool pending;
#endif
}KIRQ;

//called from kernel startup
void kirq_init();

//called from low-level startup
void kirq_stub(int vector, void* param);
void kirq_enter(int vector);

//called from svc
void kirq_register(int vector, IRQ handler, void* param);
void kirq_unregister(int vector);

#endif // KIRQ_H
