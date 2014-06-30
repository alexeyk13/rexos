/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "kernel.h"
#include "kernel_config.h"
#include "dbg.h"

void default_irq_handler(int vector)
{
#if (KERNEL_DEBUG)
    printk("Warning: irq vector %d without handler\n\r", vector);
#endif
}

void panic()
{
#if (KERNEL_DEBUG)
    printk("Kernel panic\n\r");
    dump(SRAM_BASE, 0x200);
#endif
#if (KERNEL_HALT_ON_FATAL_ERROR)
    HALT();
#else
    reset();
#endif //KERNEL_HALT_ON_FATAL_ERROR
}
