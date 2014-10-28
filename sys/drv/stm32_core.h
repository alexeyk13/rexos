/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_CORE_H
#define STM32_CORE_H

#include "../sys.h"
#include "../../userspace/process.h"
#include "stm32_config.h"
#include "sys_config.h"

typedef struct _CORE CORE;

extern const REX __STM32_CORE;

__STATIC_INLINE unsigned int stm32_core_request_outside(void* unused, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    return get(object_get(SYS_OBJ_CORE), cmd, param1, param2, param3);
}


#endif // STM32_CORE_H
