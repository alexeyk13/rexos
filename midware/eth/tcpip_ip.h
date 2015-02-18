/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TCPIP_IP_H
#define TCPIP_IP_H

#include "tcpip.h"
#include "../../userspace/inet.h"
#include "../../userspace/ipc.h"

typedef struct {
    IP ip;
} TCPIP_IP;

//tools
#if (SYS_INFO) || (TCPIP_DEBUG)
void print_ip(IP* ip);
#endif //(SYS_INFO) || (TCPIP_DEBUG)
const IP* tcpip_ip(TCPIP* tcpip);

//from tcpip
void tcpip_ip_init(TCPIP* tcpip);
bool tcpip_ip_request(TCPIP* tcpip, IPC* ipc);

//from mac
void tcpip_ip_rx(TCPIP* tcpip, TCPIP_IO* io);

#endif // TCPIP_IP_H
