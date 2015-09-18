/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "tcpip.h"
#include "stdio.h"
#include "process.h"

extern void tcpips_main();

HANDLE tcpip_create(unsigned int process_size, unsigned int priority, unsigned int eth_handle)
{
    char name[24];
    REX rex;
    sprintf(name, "TCP/IP %#x stack", eth_handle);
    rex.name = name;
    rex.size = process_size;
    rex.priority = priority;
    rex.flags = PROCESS_FLAGS_ACTIVE;
    rex.fn = tcpips_main;
    return process_create(&rex);
}

bool tcpip_open(HANDLE tcpip, HANDLE eth, unsigned int eth_handle, ETH_CONN_TYPE conn_type)
{
    return get(tcpip, HAL_CMD(HAL_TCPIP, IPC_OPEN), eth_handle, eth, conn_type) != INVALID_HANDLE;
}
