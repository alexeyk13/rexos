/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef CORE_H
#define CORE_H

/*
    core.h - core functions and model decoder. Alternatively you can specify in defines: SRAM_BASE, SRAM_SIZE, FLASH_BASE, core and vendor.
*/

#include "../stm32/stm32.h"
#include "../lpc/lpc.h"
#include "../ti/ti.h"
#include "../nrf/nrf.h"

#ifdef REXOSP
#include "rexosp.h"
#endif //REXOSP

#if defined(CORTEX_M3) || defined(CORTEX_M0) || defined(CORTEX_M4) || defined(CORTEX_M7)
#ifndef CORTEX_M
#define CORTEX_M
#endif //CORTEX_M
#endif //defined(CORTEX_M3) || defined(CORTEX_M0) || defined(CORTEX_M4)

#ifdef CORTEX_M
#ifndef SRAM_BASE
#define SRAM_BASE                0x20000000
#endif
#endif

#ifdef ARM7
#include "arm7/core_arm7.h"
#endif

#endif // CORE_H
