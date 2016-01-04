/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "rtc.h"
#include "sys_config.h"

TIME* rtc_get(TIME* time)
{
    IPC ipc;
    ipc.cmd = HAL_REQ(HAL_RTC, RTC_GET);
    ipc.process = object_get(SYS_OBJ_CORE);
    call(&ipc);
    time->day = ipc.param1;
    time->ms = ipc.param2;
    return time;
}

void rtc_set(TIME* time)
{
    ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_RTC, RTC_SET), (unsigned int)time->day, (unsigned int)time->ms, 0);
}
