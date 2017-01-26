/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_CORE_H
#define LPC_CORE_H

#include "../../userspace/ipc.h"
#include "../../userspace/sys.h"
#include "../../userspace/process.h"
#include "lpc_config.h"
#include "sys_config.h"

#define PLL_LOCK_TIMEOUT                        10000

typedef struct _CORE CORE;

__STATIC_INLINE unsigned int lpc_core_request_outside(void* unused, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    return get_handle(object_get(SYS_OBJ_CORE), cmd, param1, param2, param3);
}


#endif // LPC_CORE_H
