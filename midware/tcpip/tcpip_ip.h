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
#include "sys_config.h"

typedef struct {
    IP ip;
    uint16_t id;
} TCPIP_IP;

//tools
#if (SYS_INFO) || (TCPIP_DEBUG)
void print_ip(IP* ip);
#endif //(SYS_INFO) || (TCPIP_DEBUG)
const IP* tcpip_ip(TCPIP* tcpip);

//from tcpip
void tcpip_ip_init(TCPIP* tcpip);
bool tcpip_ip_request(TCPIP* tcpip, IPC* ipc);

//allocate IP io. If more than (MTU - MAC header - IP header) and fragmentation enabled, will be allocated long frame
uint8_t* tcpip_ip_allocate_io(TCPIP* tcpip, TCPIP_IO* io, unsigned int size);
//release previously allocated io. IO is not actually freed, just put in queue of free blocks
void tcpip_ip_release_io(TCPIP* tcpip, TCPIP_IO* io);

//from mac
void tcpip_ip_rx(TCPIP* tcpip, TCPIP_IO* io);

#endif // TCPIP_IP_H
