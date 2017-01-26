/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "../../userspace/core/core.h"
#include "karm7.h"

void prefetch_abort_entry_arm7(unsigned int address)
{
#if (KERNEL_DEBUG)
    printk("PREFETCH ABORT AT: %#X\n", address);
#endif
    panic();
}

void data_abort_entry_arm7(unsigned int address)
{
#if (KERNEL_DEBUG)
    printk("DATA ABORT AT: %#X\n", address);
#endif
    panic();
}

