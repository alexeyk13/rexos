/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2018, Alexey Kramarenko
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#include "power.h"
#include "sys_config.h"
#include "object.h"
#include "core/core.h"
#include "ipc.h"

void power_set_mode(POWER_MODE mode)
{
    ipc_post_exo(HAL_CMD(HAL_POWER, POWER_SET_MODE), mode, 0, 0);
}

unsigned int power_get_clock(POWER_CLOCK_TYPE clock_type)
{
    return get_exo(HAL_REQ(HAL_POWER, POWER_GET_CLOCK), clock_type, 0, 0);
}

unsigned int power_set_clock(POWER_CLOCK_TYPE clock_type, uint32_t clock)
{
    return get_exo(HAL_REQ(HAL_POWER, POWER_SET_CLOCK), clock_type, 0, clock);
}

unsigned int power_get_core_clock()
{
    return power_get_clock(POWER_CORE_CLOCK);
}

unsigned int power_set_core_clock(uint32_t clock)
{
    return power_set_clock(POWER_CORE_CLOCK, clock);
}

