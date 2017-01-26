/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "eth.h"

void eth_set_mac(HANDLE eth, unsigned int eth_handle, const MAC* mac)
{
    ack(eth, HAL_REQ(HAL_ETH, ETH_SET_MAC), eth_handle, mac->u32.hi, mac->u32.lo);
}

void eth_get_mac(HANDLE eth, unsigned int eth_handle, MAC* mac)
{
    IPC ipc;
    ipc.cmd = HAL_REQ(HAL_ETH, ETH_GET_MAC);
    ipc.process = eth;
    ipc.param1 = eth_handle;
    call(&ipc);
    mac->u32.hi = ipc.param2;
    mac->u32.lo = ipc.param3;
}

unsigned int eth_get_header_size(HANDLE eth, unsigned int eth_handle)
{
    int res = get(eth, HAL_REQ(HAL_ETH, ETH_GET_HEADER_SIZE), eth_handle, 0, 0);
    return (res < 0) ? 0 : res;
}
