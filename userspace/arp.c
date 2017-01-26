/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "arp.h"

bool arp_add_static(HANDLE tcpip, const IP* ip, const MAC* mac)
{
    return get_handle(tcpip, HAL_REQ(HAL_ARP, ARP_ADD_STATIC), ip->u32.ip, mac->u32.hi, mac->u32.lo) != INVALID_HANDLE;
}

bool arp_remove(HANDLE tcpip, const IP* ip)
{
    return get_handle(tcpip, HAL_REQ(HAL_ARP, ARP_REMOVE), ip->u32.ip, 0, 0) != INVALID_HANDLE;
}

void arp_flush(HANDLE tcpip)
{
    ack(tcpip, HAL_REQ(HAL_ARP, IPC_FLUSH), 0, 0, 0);
}

void arp_show_table(HANDLE tcpip)
{
    ack(tcpip, HAL_REQ(HAL_ARP, ARP_SHOW_TABLE), 0, 0, 0);
}
