/*
 RExOS - embedded RTOS
 Copyright (c) 2011-2017, Alexey Kramarenko
 All rights reserved.
 */

#include "dhcp.h"

void dhcp_set_pool(HANDLE tcpip, const IP* ip_first, const IP* mask)
{
    ack(tcpip, HAL_REQ(HAL_DHCPS, IPC_WRITE), tcpip, ip_first->u32.ip, mask->u32.ip);
}
