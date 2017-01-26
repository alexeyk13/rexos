/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "wdt.h"
#include "object.h"
#include "sys_config.h"
#include "core/core.h"

#ifdef EXODRIVERS
void wdt_kick()
{
    ipc_post_exo(HAL_CMD(HAL_WDT, WDT_KICK), 0, 0, 0);
}

#else
void wdt_kick()
{
    ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_WDT, WDT_KICK), 0, 0, 0);
}

#endif //EXODRIVERS

