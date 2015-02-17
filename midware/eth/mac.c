/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "mac.h"
#include "tcpip_private.h"
#include "../../userspace/stdio.h"
#include "arp.h"

void mac_init(TCPIP* tcpip)
{
    eth_get_mac(&tcpip->mac.mac);
}

#if (SYS_INFO) || (TCPIP_DEBUG)
void print_mac(MAC* mac)
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
    for (i = 0; i < MAC_SIZE; ++i)
    {
        printf("%02X", mac->u8[i]);
        if (i < MAC_SIZE - 1)
            printf(":");
    }
}
#endif

#if (SYS_INFO)
void mac_info(TCPIP* tcpip)
{
    printf("MAC: ");
    print_mac(&tcpip->mac.mac);
    printf("\n\r");
}
#endif

bool mac_compare(MAC* src, MAC* dst)
{
    return (src->u32.hi == dst->u32.hi) && (src->u32.lo == dst->u32.lo);
}

void mac_rx(TCPIP* tcpip, uint8_t* buf, unsigned int size, HANDLE block)
{
    uint16_t lentype;
    if (size < MAC_HEADER_SIZE)
    {
        tcpip_release_block(tcpip, block);
        return;
    }
    lentype = (buf[MAC_HEADER_LENTYPE_OFFSET] << 8) | buf[MAC_HEADER_LENTYPE_OFFSET + 1];

#if (MAC_FILTER)
    switch (((MAC*)buf)->u8[0] & MAC_CAST_MASK)
    {
    case MAC_BROADCAST_ADDRESS:
        //enable broadcast only for ARP
        if (lentype != ETHERTYPE_ARP)
        {
            tcpip_release_block(tcpip, block);
            return;
        }
        break;
    case MAC_MULTICAST_ADDRESS:
        //drop all multicast
        tcpip_release_block(tcpip, block);
        return;
    default:
        //enable unicast only on address compare
        if (!mac_compare(&tcpip->mac.mac, (MAC*)buf))
        {
            tcpip_release_block(tcpip, block);
            return;
        }
        break;
    }
#endif //MAC_FILTER

#if (MAC_DEBUG)
    printf("MAC RX: ");
    print_mac((MAC*)(buf + MAC_SIZE));
    printf(" -> ");
    print_mac((MAC*)buf);
    printf(", len/type: %04X, size: %d\n\r", lentype, size - MAC_HEADER_SIZE);
#endif
    //TODO: forward to IP
    switch (lentype)
    {
    case ETHERTYPE_IP:
///        printf("IP\n\r");
        tcpip_release_block(tcpip, block);
        break;
    case ETHERTYPE_ARP:
        arp_rx(tcpip, buf + MAC_HEADER_SIZE, size - MAC_HEADER_SIZE, block);
        break;
    default:
#if (MAC_DEBUG)
        printf("MAC: dropped lentype: %04X\n\r", lentype);
#endif
        tcpip_release_block(tcpip, block);
        break;
    }
}
