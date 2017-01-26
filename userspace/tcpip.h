/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TCPIP_H
#define TCPIP_H

#include "types.h"
#include "eth.h"
#include "ipc.h"

typedef enum {
    TCPIP_GET_CONN_STATE = IPC_USER,
}TCPIP_IPCS;

HANDLE tcpip_create(unsigned int process_size, unsigned int priority, unsigned int eth_handle);
bool tcpip_open(HANDLE tcpip, HANDLE eth, unsigned int eth_handle, ETH_CONN_TYPE conn_type);
void tcpip_close(HANDLE tcpip);
ETH_CONN_TYPE tcpip_get_conn_state(HANDLE tcpip);

#endif // TCPIP_H
