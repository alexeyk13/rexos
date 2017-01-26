/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ETH_H
#define ETH_H

#include <stdint.h>
#include "ipc.h"
#include "mac.h"

typedef enum {
    ETH_10_HALF = 0,
    ETH_10_FULL,
    ETH_100_HALF,
    ETH_100_FULL,
    ETH_AUTO,
    ETH_LOOPBACK,
    //for status only
    ETH_NO_LINK,
    ETH_REMOTE_FAULT
} ETH_CONN_TYPE;

typedef enum {
    ETH_SET_MAC = IPC_USER,
    ETH_GET_MAC,
    ETH_NOTIFY_LINK_CHANGED,
    ETH_GET_HEADER_SIZE
}ETH_IPCS;

void eth_set_mac(HANDLE eth, unsigned int eth_handle, const MAC* mac);
void eth_get_mac(HANDLE eth, unsigned int eth_handle, MAC* mac);
unsigned int eth_get_header_size(HANDLE eth, unsigned int eth_handle);

#endif // ETH_H
