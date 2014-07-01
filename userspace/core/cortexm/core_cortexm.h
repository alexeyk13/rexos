/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef CORE_CORTEXM_H
#define CORE_CORTEXM_H

#ifndef SRAM_BASE
#define SRAM_BASE                                           0x20000000
#endif

#if !defined(FLASH_BASE)
#if defined(STM)
#define FLASH_BASE                                          0x08000000
#elif defined(NXP)
#define FLASH_BASE                                          0x00000000
#else
#error Vendor unknown, flash base address is unknown
#endif
#endif //FLASH_BASE

#if !defined(LDS) && !defined(__ASSEMBLER__)

#include "../../cc_macro.h"

__STATIC_INLINE int disable_interrupts(void)
{
    int result;
    __ASM volatile ("MRS %0, primask\n\t"
                        "cpsid i" : "=r" (result) );
    return(result);
}

__STATIC_INLINE void restore_interrupts(int state)
{
    __ASM volatile ("MSR primask, %0" : : "r" (state) );
}

#endif //!defined(LDS) && !defined(__ASSEMBLER__)

#endif // CORE_CORTEXM_H
