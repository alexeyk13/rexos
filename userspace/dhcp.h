/*
 RExOS - embedded RTOS
 Copyright (c) 2011-2017, Alexey Kramarenko
 All rights reserved.
 */

#ifndef DHCP_H
#define DHCP_H

#include "ip.h"
#include <stdint.h>

void dhcp_set_pool(HANDLE tcpip, const IP* ip_first, const IP* mask);

#endif // DHCP_H
