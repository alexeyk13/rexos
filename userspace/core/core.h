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
#include "../lpc/lpc.h"

#ifdef CORTEX_M
#ifndef SRAM_BASE
#define SRAM_BASE                0x20000000
#endif
#endif

#ifdef ARM7
#include "arm7/core_arm7.h"
#endif

#endif // CORE_H
