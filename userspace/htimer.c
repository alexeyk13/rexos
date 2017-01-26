/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "htimer.h"
#include "object.h"
#include "core/core.h"
#include "sys_config.h"

#ifdef EXODRIVERS
bool htimer_open(int num, unsigned int flags)
{
    return get_size_exo(HAL_REQ(HAL_TIMER, IPC_OPEN), num, flags, 0) >= 0 ? true : false;
}

void htimer_close(int num)
{
    ipc_post_exo(HAL_CMD(HAL_TIMER, IPC_CLOSE), num, 0, 0);
}

void htimer_start(int num, TIMER_VALUE_TYPE value_type, unsigned int value)
{
    ipc_post_exo(HAL_CMD(HAL_TIMER, TIMER_START), num, value_type, value);
}

void htimer_stop(int num)
{
    ipc_post_exo(HAL_CMD(HAL_TIMER, TIMER_STOP), num, 0, 0);
}

void htimer_setup_channel(int num, int channel, TIMER_CHANNEL_TYPE type, unsigned int value)
{
    ipc_post_exo(HAL_CMD(HAL_TIMER, TIMER_SETUP_CHANNEL), num, (channel << TIMER_CHANNEL_POS) | (type << TIMER_CHANNEL_TYPE_POS), value);
}

#else
bool htimer_open(int num, unsigned int flags)
{
    return get_size(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_TIMER, IPC_OPEN), num, flags, 0) >= 0 ? true : false;
}

void htimer_close(int num)
{
    ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_TIMER, IPC_CLOSE), num, 0, 0);
}

void htimer_start(int num, TIMER_VALUE_TYPE value_type, unsigned int value)
{
    ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_TIMER, TIMER_START), num, value_type, value);
}

void htimer_stop(int num)
{
    ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_TIMER, TIMER_STOP), num, 0, 0);
}

void htimer_setup_channel(int num, int channel, TIMER_CHANNEL_TYPE type, unsigned int value)
{
    ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_TIMER, TIMER_SETUP_CHANNEL), num, (channel << TIMER_CHANNEL_POS) | (type << TIMER_CHANNEL_TYPE_POS), value);
}

#endif //EXODRIVERS
