/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef UDPS_H
#define UDPS_H

#include "tcpips.h"
#include "ips.h"
#include "../../userspace/ip.h"
#include "../../userspace/io.h"
#include "../../userspace/so.h"

typedef struct {
    SO handles;
} UDPS;

void udps_init(TCPIPS* tcpips);
//TODO: link change - drop all connections
void udps_rx(TCPIPS* tcpips, IO* ip, IP* src);
void udps_request(TCPIPS* tcpips, IPC* ipc);

#endif // UDPS_H
