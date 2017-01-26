/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "power.h"
#include "sys_config.h"
#include "object.h"
#include "core/core.h"
#include "ipc.h"

#ifdef EXODRIVERS
void power_set_mode(POWER_MODE mode)
{
    ipc_post_exo(HAL_CMD(HAL_POWER, POWER_SET_MODE), mode, 0, 0);
}

unsigned int power_get_clock(POWER_CLOCK_TYPE clock_type)
{
    return get_exo(HAL_REQ(HAL_POWER, POWER_GET_CLOCK), clock_type, 0, 0);
}
#else

void power_set_mode(POWER_MODE mode)
{
    ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_POWER, POWER_SET_MODE), mode, 0, 0);
}

unsigned int power_get_clock(POWER_CLOCK_TYPE clock_type)
{
    return get(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_POWER, POWER_GET_CLOCK), clock_type, 0, 0);
}
#endif //EXODRIVERS

unsigned int power_get_core_clock()
{
    return power_get_clock(POWER_CORE_CLOCK);
}

