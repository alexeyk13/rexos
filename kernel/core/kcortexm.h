/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KCORTEXM_H
#define KCORTEXM_H

#include "../../userspace/cc_macro.h"

//Application Interrupt and Reset Control Register
#define SCB_AIRCR                        (*(unsigned int*)0xE000ED0C)

#define AIRCR_SYSRESETREQ                (1 << 2)
#define AIRCR_VECTKEYSTAT                (0x5fa << 16)

//Configurable Fault Status Register
#define SCB_CFSR                         (*(unsigned int*)0xE000ED28)

#define CFSR_IACCVIOL                    (1 << 0)
#define CFSR_DACCVIOL                    (1 << 1)
#define CFSR_MUNSTKERR                   (1 << 3)
#define CFSR_MSTKERR                     (1 << 4)

#define CFSR_IBUSERR                     (1 << 8)
#define CFSR_PRECISERR                   (1 << 9)
#define CFSR_IMPRECISERR                 (1 << 10)
#define CFSR_BUNSTKERR                   (1 << 11)
#define CFSR_BSTKERR                     (1 << 12)

#define CFSR_UNDEFINSTR                  (1 << 16)
#define CFSR_INVSTATE                    (1 << 17)
#define CFSR_INVPC                       (1 << 18)
#define CFSR_NOCP                        (1 << 19)
#define CFSR_UNALIGNED                   (1 << 24)
#define CFSR_DIVBYZERO                   (1 << 25)

//HardFault Status Register
#define SCB_HFSR                         (*(unsigned int*)0xE000ED2C)

#define HFSR_VECTTBL                     (1 << 1)
#define HFSR_FORCED                      (1 << 30)
#define HFSR_DEBUGVT                     (1 << 31)

//MemManage Address Register
#define SCB_MMAR                         (*(unsigned int*)0xE000ED34)
//BusFault Address Register
#define SCB_BFAR                         (*(unsigned int*)0xE000ED38)

__STATIC_INLINE void fatal()
{
    __ASM volatile ("dsb");
    SCB_AIRCR = AIRCR_VECTKEYSTAT | AIRCR_SYSRESETREQ;
    __ASM volatile ("dsb");
}

__STATIC_INLINE void disable_interrupts(void)
{
    __ASM volatile ("cpsid i");
}

__STATIC_INLINE void enable_interrupts(void)
{
    __ASM volatile ("cpsie i");
}

#endif // KCORTEXM_H
