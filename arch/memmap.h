/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MEMMAP_H
#define MEMMAP_H

#ifndef __ASSEMBLER__
//defined in .ld
extern unsigned int    _end;
#endif //__ASSEMBLER__

#include "arch.h"
#include "../userspace/core/core.h"
#ifdef CUSTOM_ARCH
#include "memmap_custom.h"
#endif
#ifdef CORTEX_M3
#include "memmap_cortex_m3.h"
#endif

#endif // MEMMAP_H
