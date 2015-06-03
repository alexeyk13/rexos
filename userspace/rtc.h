/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef RTC_H
#define RTC_H

#include "sys.h"
#include "ipc.h"
#include "sys_config.h"
#include "cc_macro.h"
#include "ipc.h"
#include "time.h"

typedef enum {
    RTC_GET = IPC_USER,                                     //!< Get RTC value
    RTC_SET,                                                //!< Set RTC value

    RTC_HAL_MAX
} RTC_IPCS;

__STATIC_INLINE time_t rtc_get()
{
    return get(object_get(SYS_OBJ_CORE), HAL_CMD(HAL_RTC, RTC_GET), 0, 0, 0);
}

__STATIC_INLINE void rtc_set(time_t time)
{
    ack(object_get(SYS_OBJ_CORE), HAL_CMD(HAL_RTC, RTC_SET), (unsigned int)time, 0, 0);
}

#endif // RTC_H
