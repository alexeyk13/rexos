/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "udp.h"
#include "endian.h"

#pragma pack(push, 1)
typedef struct {
    IP src;
    IP dst;
    uint8_t zero;
    uint8_t proto;
    uint8_t length_be[2];
} UDP_PSEUDO_HEADER;
#pragma pack(pop)

uint16_t udp_checksum(void* buf, unsigned int size, const IP* src, const IP* dst)
{
    unsigned int i;
    UDP_PSEUDO_HEADER uph;
    uint32_t sum = 0;
    uph.src.u32.ip = src->u32.ip;
    uph.dst.u32.ip = dst->u32.ip;
    uph.zero = 0;
    uph.proto = PROTO_UDP;
    short2be(uph.length_be, size);

    for (i = 0; i < (sizeof(UDP_PSEUDO_HEADER) >> 1); ++i)
        sum += (((uint8_t*)&uph)[i << 1] << 8) | (((uint8_t*)&uph)[(i << 1) + 1]);
    for (i = 0; i < (size >> 1); ++i)
        sum += (((uint8_t*)buf)[i << 1] << 8) | (((uint8_t*)buf)[(i << 1) + 1]);
    //padding zero
    if (size & 1)
        sum += ((uint8_t*)buf)[size - 1] << 8;
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);
    return ~((uint16_t)sum);
}

HANDLE udp_listen(HANDLE tcpip, unsigned short port)
{
    return get_handle(tcpip, HAL_REQ(HAL_UDP, IPC_OPEN), port, LOCALHOST, 0);
}

HANDLE udp_connect(HANDLE tcpip, unsigned short port, const IP* remote_addr)
{
    return get_handle(tcpip, HAL_REQ(HAL_UDP, IPC_OPEN), port, remote_addr->u32.ip, 0);
}

void udp_close_connect(HANDLE tcpip, HANDLE handle)
{
    ack(tcpip, HAL_REQ(HAL_UDP, IPC_CLOSE), handle, 0, 0);
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

void udp_flush(HANDLE tcpip, HANDLE handle)
{
    ack(tcpip, HAL_CMD(HAL_UDP, IPC_FLUSH), handle, 0, 0);
}
