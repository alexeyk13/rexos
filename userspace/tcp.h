/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TCP_H
#define TCP_H

#include "ip.h"
#include "io.h"
#include "ipc.h"
#include <stdint.h>

#define TCP_PSH                     (1 << 0)
#define TCP_URG                     (1 << 1)

typedef struct {
    uint16_t flags;
    uint16_t urg_len;
} TCP_STACK;

typedef enum {
    TCP_LISTEN = IPC_USER,
    TCP_CLOSE_LISTEN,
    TCP_CREATE_TCB,
    TCP_GET_REMOTE_ADDR,
    TCP_GET_REMOTE_PORT,
    TCP_GET_LOCAL_PORT
}TCP_IPCS;

uint16_t tcp_checksum(void* buf, unsigned int size, const IP* src, const IP* dst);

void tcp_get_remote_addr(HANDLE tcpip, HANDLE handle, IP* ip);
uint16_t tcp_get_remote_port(HANDLE tcpip, HANDLE handle);
uint16_t tcp_get_local_port(HANDLE tcpip, HANDLE handle);

HANDLE tcp_listen(HANDLE tcpip, unsigned short port);
void tcp_close_listen(HANDLE tcpip, HANDLE handle);

HANDLE tcp_create_tcb(HANDLE tcpip, const IP* remote_addr, uint16_t remote_port);
bool tcp_open(HANDLE tcpip, HANDLE handle);
void tcp_close(HANDLE tcpip, HANDLE handle);

#define tcp_read(tcpip, handle, io, size)                           io_read((tcpip), HAL_IO_REQ(HAL_TCP, IPC_READ), (handle), (io), (size))
#define tcp_read_sync(tcpip, handle, io, size)                      io_read_sync((tcpip), HAL_IO_REQ(HAL_TCP, IPC_READ), (handle), (io), (size))
#define tcp_write(tcpip, handle, io)                                io_write((tcpip), HAL_IO_REQ(HAL_TCP, IPC_WRITE), (handle), (io))
#define tcp_write_sync(tcpip, handle, io)                           io_write_sync((tcpip), HAL_IO_REQ(HAL_TCP, IPC_WRITE), (handle), (io))

void tcp_flush(HANDLE tcpip, HANDLE handle);

#endif // TCP_H
