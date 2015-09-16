/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "eth.h"
#include "object.h"
#include "sys_config.h"

void eth_set_mac(const MAC* mac)
{
    ack(object_get(SYS_OBJ_ETH), HAL_CMD(HAL_ETH, ETH_SET_MAC), mac->u32.hi, mac->u32.lo, 0);
}

void eth_get_mac(MAC* mac)
{
    IPC ipc;
    ipc.process = object_get(SYS_OBJ_ETH);
    ipc.cmd = HAL_CMD(HAL_ETH, ETH_GET_MAC);
    call(&ipc);
    mac->u32.hi = ipc.param1;
    mac->u32.lo = ipc.param2;
}
