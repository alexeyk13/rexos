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
    error(ERROR_STUB_CALLED);
}

void kirq_init()
{
    int i;
    for (i = 0; i < IRQ_VECTORS_COUNT; ++i)
    {
        __KERNEL->irqs[i].handler = kirq_stub;
        __KERNEL->irqs[i].heap = __KERNEL;
    }
#ifndef NVIC_PRESENT
    irq_init();
#endif //NVIC_PRESENT
}

void kirq_enter(int vector)
{
    register void* saved_heap = __GLOBAL->heap;
    __GLOBAL->heap = __KERNEL->irqs[vector].heap;
    __KERNEL->irqs[vector].handler(vector, __KERNEL->irqs[vector].param);
    __GLOBAL->heap = saved_heap;
}

void kirq_register(int vector, IRQ handler, void* param)
{
    if (vector >= IRQ_VECTORS_COUNT)
        error(ERROR_OUT_OF_RANGE);
    else if (__KERNEL->irqs[vector].handler != kirq_stub)
        error(ERROR_ACCESS_DENIED);
    else
    {
        __KERNEL->irqs[vector].handler = handler;
        __KERNEL->irqs[vector].param = param;
        __KERNEL->irqs[vector].heap = __GLOBAL->heap;
    }
}

void kirq_unregister(int vector)
{
    if (__KERNEL->irqs[vector].heap == __GLOBAL->heap)
    {
        __KERNEL->irqs[vector].handler = kirq_stub;
        __KERNEL->irqs[vector].heap = __KERNEL;
        __KERNEL->irqs[vector].param = NULL;
    }
    else
        error(ERROR_ACCESS_DENIED);
}
