/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2018, Alexey Kramarenko
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef DNS_H
#define DNS_H

#include "ip.h"
#include <stdint.h>


void dns_set_name(HANDLE tcpip, const char* domain_name);

#endif // DNS_H
