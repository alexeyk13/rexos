/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TCPIP_ROUTE_H
#define TCPIP_ROUTE_H

/*
    routing. Lookup router address by target IP. For now is just stub, always returning target IP.
 */

#include "tcpip.h"
#include "../../userspace/eth.h"
#include "../../userspace/inet.h"
#include "../../userspace/array.h"

typedef struct {
    ARRAY* tx_array;
} TCPIP_ROUTE;

//called from tcpip
void tcpip_route_init(TCPIP* tcpip);

//called from arp
void tcpip_route_arp_resolved(TCPIP* tcpip, const IP* ip, const MAC* mac);
void tcpip_route_arp_not_resolved(TCPIP* tcpip, const IP* ip);

//called from ip
void tcpip_route_tx(TCPIP* tcpip, TCPIP_IO* io, const IP* target);

#endif // TCPIP_ROUTE_H
