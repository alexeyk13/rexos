/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TCP_H
#define TCP_H

#include "ip.h"
#include "io.h"

HANDLE tcp_listen(HANDLE tcpip, unsigned short port);
HANDLE tcp_connect(HANDLE tcpip, unsigned short port, const IP* remote_addr);

#endif // TCP_H
