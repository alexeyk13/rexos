/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "icmp.h"

bool icmp_ping(HANDLE tcpip, IP* dst)
{
    return get(tcpip, HAL_CMD(HAL_ICMP, ICMP_PING), dst->u32.ip, 0, 0) != INVALID_HANDLE;
}
