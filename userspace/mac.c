/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "mac.h"
#include "stdio.h"

void mac_print(const MAC* mac)
{
    int i;
    switch (mac->u8[0] & MAC_CAST_MASK)
    {
    case MAC_BROADCAST_ADDRESS:
        printf("(broadcast)");
        break;
    case MAC_MULTICAST_ADDRESS:
        printf("(multicast)");
        break;
    default:
        break;
    }
    for (i = 0; i < sizeof(MAC); ++i)
    {
        printf("%02X", mac->u8[i]);
        if (i < sizeof(MAC) - 1)
            printf(":");
    }
}

bool mac_compare(const MAC* src, const MAC* dst)
{
    return (src->u32.hi == dst->u32.hi) && (src->u32.lo == dst->u32.lo);
}

void mac_enable_firewall(HANDLE tcpip, const MAC* src)
{
    ack(tcpip, HAL_REQ(HAL_MAC, MAC_ENABLE_FIREWALL), 0, src->u32.hi, src->u32.lo);
}

void mac_disable_firewall(HANDLE tcpip)
{
    ack(tcpip, HAL_REQ(HAL_MAC, MAC_DISABLE_FIREWALL), 0, 0, 0);
}
