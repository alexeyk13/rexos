/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef IP_H
#define IP_H

#include "tcpip.h"
#include "../../userspace/inet.h"
#include "../../userspace/ipc.h"

typedef struct {
    IP ip;
} TCPIP_IP;

void tcpip_ip_init(TCPIP* tcpip);

#if (SYS_INFO) || (TCPIP_DEBUG)
void print_ip(IP* ip);
#endif //(SYS_INFO) || (TCPIP_DEBUG)

bool tcpip_ip_request(TCPIP* tcpip, IPC* ipc);

const IP* tcpip_ip(TCPIP* tcpip);

#endif // IP_H
