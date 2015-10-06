/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "tcp.h"

HANDLE tcp_listen(HANDLE tcpip, unsigned short port)
{
    return get_handle(tcpip, HAL_REQ(HAL_TCP, IPC_OPEN), port, LOCALHOST, 0);
}

HANDLE tcp_connect(HANDLE tcpip, unsigned short port, const IP* remote_addr)
{
    return get_handle(tcpip, HAL_REQ(HAL_TCP, IPC_OPEN), port, remote_addr->u32.ip, 0);
}

