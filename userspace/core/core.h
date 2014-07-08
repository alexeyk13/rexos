/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef CORE_H
#define CORE_H

/*
    core.h - core functions and model decoder. Alternatively you can specify in defines: SRAM_BASE, SRAM_SIZE, FLASH_BASE, core and vendor.
*/

#include "stm32.h"

#ifdef ARM7
#include "arm7/core_arm7.h"
#elif defined(CORTEX_M3) || defined(CORTEX_M0) || defined(CORTEX_M1) || defined(CORTEX_M4)
#include "cortexm.h"
#else
#error MCU core is not defined or not supported
#endif

#endif // CORE_H