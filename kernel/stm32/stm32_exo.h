/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_EXO_H
#define STM32_EXO_H

#include "../../userspace/sys.h"
#include "../../userspace/process.h"
#include "stm32_config.h"
#include "sys_config.h"

typedef struct _EXO EXO;
/*
__STATIC_INLINE unsigned int stm32_core_request_outside(void* unused, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    return get_handle(object_get(SYS_OBJ_CORE), cmd, param1, param2, param3);
}
*/

#endif // STM32_EXO_H
