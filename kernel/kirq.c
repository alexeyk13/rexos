/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "kirq.h"
#include "kernel.h"
#include "../userspace/error.h"

void kirq_stub(int vector, void* param)
{
#if (KERNEL_DEBUG)
    printk("Warning: irq vector %d stub called\n\r", vector);
#endif
    kprocess_error_current(ERROR_STUB_CALLED);
}

void kirq_init()
{
    int i;
    for (i = 0; i < IRQ_VECTORS_COUNT; ++i)
        __KERNEL->irqs[i].handler = kirq_stub;
    __KERNEL->context = -1;
#ifndef NVIC_PRESENT
    irq_init();
#endif //NVIC_PRESENT
}

void kirq_enter(int vector)
{
    register int saved_context = __KERNEL->context;
    __KERNEL->context = vector;
    __KERNEL->irqs[vector].handler(vector, __KERNEL->irqs[vector].param);
    __KERNEL->context = saved_context;
}

void kirq_register(int vector, IRQ handler, void* param)
{
    if (vector >= IRQ_VECTORS_COUNT)
        kprocess_error_current(ERROR_OUT_OF_RANGE);
    else if (__KERNEL->irqs[vector].handler != kirq_stub)
        kprocess_error_current(ERROR_ACCESS_DENIED);
    else
    {
        __KERNEL->irqs[vector].handler = handler;
        __KERNEL->irqs[vector].param = param;
        __KERNEL->irqs[vector].process = kprocess_get_current();
    }
}

void kirq_unregister(int vector)
{
    if (__KERNEL->irqs[vector].process == kprocess_get_current())
    {
        __KERNEL->irqs[vector].handler = kirq_stub;
        __KERNEL->irqs[vector].process = NULL;
        __KERNEL->irqs[vector].param = NULL;
    }
    else
        kprocess_error_current(ERROR_ACCESS_DENIED);
}
