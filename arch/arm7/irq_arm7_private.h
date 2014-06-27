/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef IRQ_ARM7_PRIVATE_H
#define IRQ_ARM7_PRIVATE_H

#include "arch.h"

extern void irq_hw_init();
extern IRQn irq_hw_get_vector();
extern void irq_hw_exit(IRQn irq);
extern void fiq_hw_exit();
extern void fiq_hw_set_source(IRQn irq);

#endif // IRQ_ARM7_PRIVATE_H
