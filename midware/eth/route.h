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

#include "tcpip.h"
#include "../../userspace/inet.h"

const IP* route_lookup(const IP* target);

#endif // ROUTE_H
