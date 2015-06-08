/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ETH_H
#define ETH_H

#include <stdint.h>
#include "sys.h"
#include "ipc.h"
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

__STATIC_INLINE void eth_set_mac(const MAC* mac)
{
    ack(object_get(SYS_OBJ_ETH), HAL_CMD(HAL_ETH, ETH_SET_MAC), mac->u32.hi, mac->u32.lo, 0);
}

__STATIC_INLINE void eth_get_mac(MAC* mac)
{
    IPC ipc;
    ipc.process = object_get(SYS_OBJ_ETH);
    ipc.cmd = HAL_CMD(HAL_ETH, ETH_GET_MAC);
    call(&ipc);
    mac->u32.hi = ipc.param1;
    mac->u32.lo = ipc.param2;
}

#endif // ETH_H
