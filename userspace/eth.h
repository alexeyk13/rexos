/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ETH_H
#define ETH_H

#include <stdint.h>
#include "ipc.h"

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

#define MAC_SIZE                                    6

typedef enum {
    ETH_SET_MAC = IPC_USER,
    ETH_GET_MAC,
    ETH_NOTIFY_LINK_CHANGED,

    ETH_HAL_MAX
}ETH_IPCS;

#pragma pack(push, 1)

typedef union {
    uint8_t u8[MAC_SIZE];
    struct {
        uint32_t hi;
        uint16_t lo;
    }u32;
} MAC;

#pragma pack(pop)

void eth_set_mac(const MAC* mac);
void eth_get_mac(MAC* mac);

#endif // ETH_H
