/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef INET_H
#define INET_H

#include "ip.h"
#include "ipc.h"
#include "cc_macro.h"
#include "object.h"
#include "sys_config.h"

extern const REX __TCPIP;

typedef enum {
    ICMP_PING = IPC_USER,

    ICMP_HAL_MAX
}ICMP_IPCS;

__STATIC_INLINE int icmp_ping(HANDLE tcpip, IP* dst, unsigned int count)
{
    return get(tcpip, HAL_CMD(HAL_ICMP, ICMP_PING), dst->u32.ip, count, 0);
}

#endif // INET_H
