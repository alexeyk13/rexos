/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TCPIP_H
#define TCPIP_H

#include "types.h"
#include "eth.h"

HANDLE tcpip_create(unsigned int process_size, unsigned int priority, unsigned int eth_handle);
void tcpip_open(HANDLE tcpip, HANDLE eth, unsigned int eth_handle, ETH_CONN_TYPE conn_type);

#endif // TCPIP_H
