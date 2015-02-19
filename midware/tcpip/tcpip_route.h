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
#include "../../userspace/inet.h"

const IP* tcpip_route_lookup(const IP* target);

#endif // TCPIP_ROUTE_H
