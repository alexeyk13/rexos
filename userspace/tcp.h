/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TCP_H
#define TCP_H

#include "ip.h"
#include <stdint.h>

#define TCP_PSH                     (1 << 0)
#define TCP_URG                     (1 << 1)

typedef struct {
    uint16_t flags;
    uint16_t urgent;
} TCP_STACK;

uint16_t tcp_checksum(void* buf, unsigned int size, const IP* src, const IP* dst);
HANDLE tcp_listen(HANDLE tcpip, unsigned short port);
HANDLE tcp_connect(HANDLE tcpip, unsigned short port, const IP* remote_addr);

#endif // TCP_H
