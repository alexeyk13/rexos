/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef RNDIS_H
#define RNDIS_H

#include "eth.h"
#include "ipc.h"
#include <stdint.h>

typedef enum {
    RNDIS_SET_LINK = IPC_USER,
    RNDIS_SET_VENDOR_ID,
    RNDIS_SET_VENDOR_DESCRIPTION,
    RNDIS_SET_HOST_MAC,
    RNDIS_GET_HOST_MAC
} RNDIS_IPCS;

void rndis_set_link(HANDLE usbd, unsigned int iface, ETH_CONN_TYPE conn);
void rndis_set_vendor_id(HANDLE usbd, unsigned int iface, uint8_t a, uint8_t b, uint8_t c);
void rndis_set_vendor_description(HANDLE usbd, unsigned int iface, const char* vendor);
void rndis_set_host_mac(HANDLE usbd, unsigned int iface, const MAC* mac);
void rndis_get_host_mac(HANDLE usbd, unsigned int iface, MAC* mac);

#endif // RNDIS_H
