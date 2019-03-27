/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef ICMP_H
#define ICMP_H

#include "ip.h"
#include "ipc.h"

typedef enum {
    ICMP_PING = IPC_USER
}ICMP_IPCS;

bool icmp_ping(HANDLE tcpip, const IP* dst);

#endif // ICMP_H
