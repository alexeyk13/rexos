/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef UDP_H
#define UDP_H

#include "ip.h"
#include "io.h"
#include <stdint.h>

#pragma pack(push, 1)

typedef struct {
    IP remote_addr;
    uint16_t remote_port;
} UDP_STACK;

#pragma pack(pop)

uint16_t udp_checksum(void* buf, unsigned int size, const IP* src, const IP* dst);
HANDLE udp_listen(HANDLE tcpip, unsigned short port);
HANDLE udp_connect(HANDLE tcpip, unsigned short port, const IP* remote_addr);
void udp_close_connect(HANDLE tcpip, HANDLE handle);
#define udp_read(tcpip, handle, io, size)                           io_read((tcpip), HAL_IO_REQ(HAL_UDP, IPC_READ), (handle), (io), (size))
#define udp_read_sync(tcpip, handle, io, size)                      io_read_sync((tcpip), HAL_IO_REQ(HAL_UDP, IPC_READ), (handle), (io), (size))

//write to outgoing connection
#define udp_write(tcpip, handle, io)                                io_write((tcpip), HAL_IO_REQ(HAL_UDP, IPC_WRITE), (handle), (io))
#define udp_write_sync(tcpip, handle, io)                           io_write_sync((tcpip), HAL_IO_REQ(HAL_UDP, IPC_WRITE), (handle), (io))
//write to listening connection. Need to provide remote address and port
void udp_write_listen(HANDLE tcpip, HANDLE handle, IO* io, const IP* remote_addr, unsigned short remote_port);
int udp_write_listen_sync(HANDLE tcpip, HANDLE handle, IO* io, const IP* remote_addr, unsigned short remote_port);

void udp_flush(HANDLE tcpip, HANDLE handle);

#endif // UDP_H
