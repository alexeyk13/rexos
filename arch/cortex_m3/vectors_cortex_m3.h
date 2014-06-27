/*
    RExOS - embedded real-time RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef VECTORS_CORTEX_M3_H
#define VECTORS_CORTEX_M3_H

#include "arch.h"

#ifdef _STM
#include "vectors_stm32.h"
#elif defined CUSTOM_ARCH
#include "vectors_cortexm_custom.h"
#else
#error Vendor is not decoded
#endif


#endif // VECTORS_CORTEX_M3_H
