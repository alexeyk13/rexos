/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef UDPS_H
#define UDPS_H

#include "tcpips.h"
#include "ips.h"
#include "icmps.h"
#include "../../userspace/ip.h"
#include "../../userspace/io.h"
#include "../../userspace/so.h"

typedef struct {
    SO handles;
    uint16_t dynamic;
} UDPS;

//from tcpip
void udps_init(TCPIPS* tcpips);
//TODO: link change - drop all connections
void udps_rx(TCPIPS* tcpips, IO* io, IP* src);
void udps_request(TCPIPS* tcpips, IPC* ipc);

//from icmp
void udps_icmps_error_process(TCPIPS* tcpips, IO* io, ICMP_ERROR code, const IP* src);

#endif // UDPS_H
