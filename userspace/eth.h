/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ETH_H
#define ETH_H

#include <stdint.h>
#include "sys.h"
#include "object.h"
#include "sys_config.h"
#include "cc_macro.h"

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
    ETH_SET_MAC = HAL_IPC(HAL_ETH),
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

typedef struct {
    MAC src;
    MAC dst;
    uint16_t len;
} MAC_FRAME_HEADER;

#pragma pack(pop)

__STATIC_INLINE void eth_set_mac(const uint8_t* mac)
{
    ack(object_get(SYS_OBJ_ETH), ETH_SET_MAC, (mac[0] << 24) | (mac[1] << 16) | (mac[2] << 8) | mac[3], (mac[4] << 8) | mac[5], 0);
}

__STATIC_INLINE void eth_get_mac(uint8_t* mac)
{
    IPC ipc;
    ipc.process = object_get(SYS_OBJ_ETH);
    ipc.cmd = ETH_GET_MAC;
    ipc_call_ms(&ipc, 0);
    mac[0] = (ipc.param1 >> 24) & 0xff;
    mac[1] = (ipc.param1 >> 16) & 0xff;
    mac[2] = (ipc.param1 >> 8) & 0xff;
    mac[3] = (ipc.param1 >> 0) & 0xff;
    mac[4] = (ipc.param2 >> 8) & 0xff;
    mac[5] = (ipc.param2 >> 0) & 0xff;
}

#endif // ETH_H
