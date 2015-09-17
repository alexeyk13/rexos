/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "ip.h"

void ip_set(HANDLE tcpip, const IP* ip)
{
    ack(tcpip, HAL_CMD(HAL_IP, IP_SET), 0, ip->u32.ip, 0);
}

void ip_get(HANDLE tcpip, IP* ip)
{
    ip->u32.ip = get(tcpip, HAL_CMD(HAL_IP, IP_GET), 0, 0, 0);
}
