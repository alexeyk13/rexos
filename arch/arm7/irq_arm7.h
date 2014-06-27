/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef IRQ_ARM7_H
#define IRQ_ARM7_H

#include "arch.h"

#define IRQ_LOWEST_PRIORITY					0xff
#define IRQ_NO_IRQ								0x100

#ifndef __ASSEMBLER__
typedef void (*ISR_VECTOR)(void);

void irq_init();
IRQn irq_get_current_vector();
//must be called, when interrupts are disabled
void irq_register_vector(IRQn irq, ISR_VECTOR vector);
void irq_set_priority(IRQn irq, unsigned char priority);
void fiq_register_vector(IRQn irq, ISR_VECTOR vector);
void irq_clear_pending(IRQn irq);

//must be declared by hw
void irq_mask(IRQn irq);
void irq_unmask(IRQn irq);
bool irq_is_masked(IRQn irq);
void fiq_mask();
void fiq_unmask();
bool fiq_is_masked();
#endif //__ASSEMBLER__

#endif // IRQ_ARM7_H
