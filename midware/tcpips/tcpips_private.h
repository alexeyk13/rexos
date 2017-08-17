/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TCPIPS_PRIVATE_H
#define TCPIPS_PRIVATE_H

#include "../../userspace/array.h"
#include "../../userspace/eth.h"
#include "../../userspace/mac.h"
#include "../../userspace/ip.h"
#include "macs.h"
#include "arps.h"
#include "routes.h"
#include "ips.h"
#include "icmps.h"
#include "udps.h"
#include "tcps.h"
#include "dnss.h"
#include "dhcps.h"
#include "sys_config.h"

typedef struct _TCPIPS {
    //stack itself - public use
    HANDLE eth, timer, app;
    unsigned seconds;
    ETH_CONN_TYPE conn;
    //stack itself - private use
    unsigned int io_allocated, tx_count, eth_handle, eth_header_size;
    ARRAY* free_io;
    ARRAY* tx_queue;
    bool connected;
    MACS macs;
    IPS ips;
    ARPS arps;
    ROUTES routes;
#if (ICMP)
    ICMPS icmps;
#endif
#if (UDP)
    UDPS udps;
#endif //UDP
#if (DNSS)
    _DNSS dnss;
#endif //DNSS
#if (DHCPS)
    _DHCPS dhcps;
#endif //DHCP
    TCPS tcps;
} TCPIPS;

#endif // TCPIPS_PRIVATE_H
