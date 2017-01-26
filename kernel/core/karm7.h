/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KARM7_H
#define KARM7_H

#include "../../userspace/cc_macro.h"

#define SVC_MODE                    0x13
#define IRQ_MODE                    0x12
#define FIQ_MODE                    0x11
#define ABORT_MODE                  0x17
#define UNDEFINE_MODE               0x1b
#define SYS_MODE                    0x1f
#define USER_MODE                   0x10

#define I_BIT                       0x80
#define F_BIT                       0x40

//reset it's not common for ARM7 core, so, it must be defined in vendor-specific implementation.
__STATIC_INLINE void fatal()
{
    for (;;) {}
}

//including FIQ, which is IRQ0
__STATIC_INLINE void disable_interrupts(void)
{
    __ASM volatile ("mrs r1, cpsr\n\t"
                    "orr r1, r1, #0xc0\n\t"
                    "msr cpsr_c, r1" : : : "r1", "cc");
}

__STATIC_INLINE void enable_interrupts(void)
{
    __ASM volatile ("mrs r1, cpsr\n\t"
                    "bic r1, r1, #0xc0\n\t"
                    "msr cpsr_c, r1" : : : "r1", "cc");
}

#endif // KARM7_H
