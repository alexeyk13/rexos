/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef WDT_H
#define WDT_H

#include "sys.h"

typedef enum {
    WDT_KICK = HAL_IPC(HAL_WDT),

    WDT_HAL_MAX
} WDT_IPCS;

__STATIC_INLINE void wdt_kick()
{
    ack(object_get(SYS_OBJ_CORE), WDT_KICK, 0, 0, 0);
}


#endif // WDT_H
