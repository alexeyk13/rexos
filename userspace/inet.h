/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef INET_H
#define INET_H

#include "sys.h"
#include "ipc.h"
#include "cc_macro.h"
#include "object.h"
#include "sys_config.h"

#define IP_SIZE                                     4

#pragma pack(push, 1)

typedef union {
    uint8_t u8[IP_SIZE];
    struct {
        uint32_t ip;
    }u32;
} IP;

#define IP_MAKE(a, b, c, d)                         (((a) << 0) | ((b) << 8) | ((c) << 16) | ((d) << 24))

typedef enum {
    IP_SET = IPC_USER,
    IP_GET,

    IP_HAL_MAX
}IP_IPCS;

typedef enum {
    ICMP_PING = IPC_USER,

    ICMP_HAL_MAX
}ICMP_IPCS;

#pragma pack(pop)

__STATIC_INLINE void ip_set(const IP* ip)
{
    ack(object_get(SYS_OBJ_TCPIP), HAL_CMD(HAL_IP, IP_SET), ip->u32.ip, 0, 0);
}

__STATIC_INLINE void ip_get(IP* ip)
{
    ip->u32.ip = get(object_get(SYS_OBJ_TCPIP), HAL_CMD(HAL_IP, IP_GET), 0, 0, 0);
}

__STATIC_INLINE int icmp_ping(IP* dst, unsigned int count)
{
    return get(object_get(SYS_OBJ_TCPIP), HAL_CMD(HAL_ICMP, ICMP_PING), dst->u32.ip, count, 0);
}

#endif // INET_H
