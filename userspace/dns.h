/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef DNS_H
#define DNS_H

#include "ip.h"
#include <stdint.h>


void dns_set_name(HANDLE tcpip, const char* domain_name);

#endif // DNS_H
