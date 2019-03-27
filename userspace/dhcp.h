/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2018, Alexey Kramarenko
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef DHCP_H
#define DHCP_H

#include "ip.h"
#include <stdint.h>

void dhcp_set_pool(HANDLE tcpip, const IP* ip_first, const IP* mask);

#endif // DHCP_H
