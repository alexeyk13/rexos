/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TI_H
#define TI_H

#if defined(CC2620) || defined(CC2630) || defined(CC2640) || defined(CC2650)
#define CC26x0

#define FLASH_BASE                      0x00000000
//128k
#define FLASH_SIZE                      0x20000
//20k
#define SRAM_SIZE                       0x5000

#define EXODRIVERS
#define IRQ_VECTORS_COUNT               34
#define CORTEX_M3

#endif

#if !defined(LDS) && !defined(__ASSEMBLER__)

#if defined(CC26x0)
#include "ti_config.h"
#include "ti_driver.h"

#include "../../CMSIS/Device/TI/CC26x0/Include/cc26x0.h"

#endif //defined(CC26x0)

#endif // !defined(LDS) && !defined(__ASSEMBLER__)

#endif //TI_H
