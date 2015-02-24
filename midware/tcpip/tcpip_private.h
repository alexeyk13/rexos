/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TCPIP_PRIVATE_H
#define TCPIP_PRIVATE_H

#include "../../userspace/array.h"
#include "../../userspace/eth.h"
#include "mac.h"
#include "arp.h"
#include "route.h"
#include "ip.h"
#include "icmp.h"
#include "sys_config.h"

typedef struct _TCPIP {
    //stack itself - public use
    HANDLE eth, timer;
    unsigned seconds;
    ETH_CONN_TYPE conn;
    //stack itself - private use
    unsigned int blocks_allocated;
    ARRAY* free_blocks;
    bool active, connected;
    TCPIP_MAC mac;
    TCPIP_ARP arp;
    TCPIP_ROUTE route;
    TCPIP_IP ip;
#if (ICMP)
    TCPIP_ICMP icmp;
#endif
} TCPIP;

#endif // TCPIP_PRIVATE_H
