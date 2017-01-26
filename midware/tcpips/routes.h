/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ROUTES_H
#define ROUTES_H

/*
    routing. Lookup router address by target IP. For now is just stub, always returning target IP.
 */

#include "tcpips.h"
#include "../../userspace/eth.h"
#include "../../userspace/ip.h"
#include "../../userspace/array.h"

typedef struct {
    ARRAY* tx_queue;
} ROUTES;

//called from tcpip
void routes_init(TCPIPS* tcpips);
bool routes_drop(TCPIPS* tcpips);
void routes_link_changed(TCPIPS* tcpips, bool link);

//called from arp
void routes_resolved(TCPIPS* tcpips, const IP* ip, const MAC* mac);
void routes_not_resolved(TCPIPS* tcpips, const IP* ip);

//called from ip
void routes_tx(TCPIPS* tcpips, IO* io, const IP* target);

#endif // ROUTES_H
