/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ARCH_H
#define ARCH_H

/*
    arch.h - decode MCU family
*/

#include "arch_stm32.h"
#ifdef CUSTOM_ARCH
#include "arch_custom.h"
#endif

#ifdef ARM7
#include "arm7.h"
#elif defined(CORTEX_M3)
#define NVIC_PRESENT
#else
#error MCU architecture is not decoded! Define MCU in Makefile!
#endif

#endif // ARCH_H
