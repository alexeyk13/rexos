/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef KIRQ_H
#define KIRQ_H

#include "../userspace/irq.h"
#include "../userspace/core/core.h"
#include "kernel_config.h"

//called from kernel startup
void kirq_init();

//called from low-level startup
void kirq_stub(int vector, void* param);
void kirq_enter(int vector);

//called from svc / exodriver
void kirq_register(HANDLE owner, int vector, IRQ handler, void* param);
void kirq_unregister(HANDLE owner, int vector);

#endif // KIRQ_H
