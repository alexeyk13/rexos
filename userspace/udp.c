/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "udp.h"

HANDLE udp_listen(HANDLE tcpip, unsigned short port)
{
    return get_handle(tcpip, HAL_REQ(HAL_UDP, IPC_OPEN), port, LOCALHOST, 0);
}

HANDLE udp_connect(HANDLE tcpip, unsigned short port, const IP* remote_addr)
{
    return get_handle(tcpip, HAL_REQ(HAL_UDP, IPC_OPEN), port, remote_addr->u32.ip, 0);
}

void udp_write_listen(HANDLE tcpip, HANDLE handle, IO* io, const IP* remote_addr, unsigned short remote_port)
{
    UDP_STACK* udp_stack = io_push(io, sizeof(UDP_STACK));
    udp_stack->remote_addr.u32.ip = remote_addr->u32.ip;
    udp_stack->remote_port = remote_port;
    io_write(tcpip, HAL_IO_REQ(HAL_UDP, IPC_WRITE), handle, io);
}

int udp_write_listen_sync(HANDLE tcpip, HANDLE handle, IO* io, const IP* remote_addr, unsigned short remote_port)
{
    UDP_STACK* udp_stack = io_push(io, sizeof(UDP_STACK));
    udp_stack->remote_addr.u32.ip = remote_addr->u32.ip;
    udp_stack->remote_port = remote_port;
    return io_write_sync(tcpip, HAL_IO_REQ(HAL_UDP, IPC_WRITE), handle, io);
}
