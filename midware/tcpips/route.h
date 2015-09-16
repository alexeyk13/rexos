/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ROUTE_H
#define ROUTE_H

/*
    routing. Lookup router address by target IP. For now is just stub, always returning target IP.
 */

#include "tcpips.h"
#include "../../userspace/eth.h"
#include "../../userspace/inet.h"
#include "../../userspace/array.h"

typedef struct {
    ARRAY* tx_queue;
} TCPIP_ROUTE;

//called from tcpip
void route_init(TCPIPS* tcpips);
bool route_drop(TCPIPS* tcpips);

//called from arp
void arp_resolved(TCPIPS* tcpips, const IP* ip, const MAC* mac);
void arp_not_resolved(TCPIPS* tcpips, const IP* ip);

//called from ip
void route_tx(TCPIPS* tcpips, IO* io, const IP* target);

#endif // ROUTE_H
