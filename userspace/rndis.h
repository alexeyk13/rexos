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
    RNDIS_SET_HOST_MAC,
    RNDIS_GET_HOST_MAC,
    //TODO:
    RNDIS_HOST_MAC_CHANGED
} RNDIS_IPCS;

void rndis_set_link(HANDLE usbd, unsigned int iface, ETH_CONN_TYPE conn);
void rndis_set_host_mac(HANDLE usbd, unsigned int iface, const MAC* mac);
void rndis_get_host_mac(HANDLE usbd, unsigned int iface, MAC* mac);

#endif // RNDIS_H
