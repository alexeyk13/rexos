/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ARM7_H
#define ARM7_H

#include "../cc_macro.h"

#if !defined(LDS) && !defined(__ASSEMBLER__)

__STATIC_INLINE int disable_interrupts(void)
{
    int result;
    __ASM volatile ("mrs r1, cpsr\n\t"
                    "orr r0, r1, #0xc0\n\t"
                    "msr cpsr_c, r0\n\t"
                    "and %0, r1, #0xc0\n\t"
                    : "=r" (result) : : "r0", "r1", "cc");
    return result;
}

__STATIC_INLINE void restore_interrupts(int state)
{
    __ASM volatile ("mrs r1, cpsr\n\t"
                    "bic r1, r1, #0xC0\n\t"
                    "orr r1, %0\n\t"
                    "msr cpsr_c, r1" : : "r" (state) : "r1", "cc");
}

#endif //!defined(LDS) && !defined(__ASSEMBLER__)

#endif // ARM7_H
