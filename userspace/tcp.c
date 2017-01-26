/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "tcp.h"
#include "endian.h"

#pragma pack(push, 1)
typedef struct {
    IP src;
    IP dst;
    uint8_t zero;
    uint8_t ptcl;
    uint8_t length_be[2];
} TCP_PSEUDO_HEADER;
#pragma pack(pop)

uint16_t tcp_checksum(void* buf, unsigned int size, const IP* src, const IP* dst)
{
    TCP_PSEUDO_HEADER tph;
    unsigned int i;
    uint32_t sum = 0;
    tph.src.u32.ip = src->u32.ip;
    tph.dst.u32.ip = dst->u32.ip;
    tph.zero = 0;
    tph.ptcl = PROTO_TCP;
    short2be(tph.length_be, size);

    for (i = 0; i < (sizeof(TCP_PSEUDO_HEADER) >> 1); ++i)
        sum += (((uint8_t*)&tph)[i << 1] << 8) | (((uint8_t*)&tph)[(i << 1) + 1]);
    for (i = 0; i < (size >> 1); ++i)
        sum += (((uint8_t*)buf)[i << 1] << 8) | (((uint8_t*)buf)[(i << 1) + 1]);
    //padding zero
    if (size & 1)
        sum += ((uint8_t*)buf)[size - 1] << 8;
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);
    return ~((uint16_t)sum);
}

void tcp_get_remote_addr(HANDLE tcpip, HANDLE handle, IP* ip)
{
    ip->u32.ip = get(tcpip, HAL_REQ(HAL_TCP, TCP_GET_REMOTE_ADDR), handle, 0, 0);
}

uint16_t tcp_get_remote_port(HANDLE tcpip, HANDLE handle)
{
    return get(tcpip, HAL_REQ(HAL_TCP, TCP_GET_REMOTE_PORT), handle, 0, 0);
}

uint16_t tcp_get_local_port(HANDLE tcpip, HANDLE handle)
{
    return get(tcpip, HAL_REQ(HAL_TCP, TCP_GET_LOCAL_PORT), handle, 0, 0);
}

HANDLE tcp_listen(HANDLE tcpip, unsigned short port)
{
    return get_handle(tcpip, HAL_REQ(HAL_TCP, TCP_LISTEN), port, 0, 0);
}

void tcp_close_listen(HANDLE tcpip, HANDLE handle)
{
    ack(tcpip, HAL_REQ(HAL_TCP, TCP_CLOSE_LISTEN), handle, 0, 0);
}

HANDLE tcp_create_tcb(HANDLE tcpip, const IP* remote_addr, uint16_t remote_port)
{
    return get_handle(tcpip, HAL_REQ(HAL_TCP, TCP_CREATE_TCB), remote_port, remote_addr->u32.ip, 0);
}

bool tcp_open(HANDLE tcpip, HANDLE handle)
{
    return get_handle(tcpip, HAL_REQ(HAL_TCP, IPC_OPEN), handle, 0, 0) != INVALID_HANDLE;
}

void tcp_close(HANDLE tcpip, HANDLE handle)
{
    ack(tcpip, HAL_REQ(HAL_TCP, IPC_CLOSE), handle, 0, 0);
}

void tcp_flush(HANDLE tcpip, HANDLE handle)
{
    ack(tcpip, HAL_REQ(HAL_TCP, IPC_FLUSH), handle, 0, 0);
}
