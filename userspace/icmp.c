/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#include "icmp.h"
#include "systime.h"

bool icmp_ping(HANDLE tcpip, const IP* dst)
{
    return get_handle(tcpip, HAL_REQ(HAL_ICMP, ICMP_PING), dst->u32.ip, 0, 0) != INVALID_HANDLE;
}
