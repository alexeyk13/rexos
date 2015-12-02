/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef RNDIS_H
#define RNDIS_H

#include "eth.h"
#include "ipc.h"

typedef enum {
    RNDIS_SET_LINK = IPC_USER,
} RNDIS_IPCS;


void rndis_set_link(HANDLE usbd, unsigned int iface, ETH_CONN_TYPE conn);

#endif // RNDIS_H
