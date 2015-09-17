/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "eth.h"

void eth_set_mac(HANDLE eth, unsigned int eth_handle, const MAC* mac)
{
    ack(eth, HAL_CMD(HAL_ETH, ETH_SET_MAC), eth_handle, mac->u32.hi, mac->u32.lo);
}

void eth_get_mac(HANDLE eth, unsigned int eth_handle, MAC* mac)
{
    IPC ipc;
    ipc.cmd = HAL_CMD(HAL_ETH, ETH_GET_MAC);
    ipc.process = eth;
    ipc.param1 = eth_handle;
    call(&ipc);
    mac->u32.hi = ipc.param2;
    mac->u32.lo = ipc.param3;
}
