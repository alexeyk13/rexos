/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "arp.h"
#include "tcpip_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/inet.h"
#include "../../userspace/eth.h"
#include "mac.h"

#define ARP_HEADER_SIZE             (2 + 2 + 1 + 1 + 2)

#define ARP_HRD(buf)                (((buf)[0] << 8) | (buf)[1])
#define ARP_PRO(buf)                (((buf)[2] << 8) | (buf)[3])
#define ARP_HLN(buf)                ((buf)[4])
#define ARP_PLN(buf)                ((buf)[5])
#define ARP_OP(buf)                 (((buf)[6] << 8) | (buf)[7])
#define ARP_SHA(buf)                ((MAC*)((buf) + 8))
#define ARP_SPA(buf)                ((IP*)((buf) + 8 + 6))
#define ARP_THA(buf)                ((MAC*)((buf) + 8 + 6 + 4))
#define ARP_TPA(buf)                ((IP*)((buf) + 8 + 6 + 4 + 6))

//TODO: move to ip.h
void print_ip(IP* ip)
{
    int i;
    for (i = 0; i < IP_SIZE; ++i)
    {
        printf("%d", ip->u8[i]);
        if (i < IP_SIZE - 1)
            printf(".");
    }
}

void arp_rx(TCPIP* tcpip, uint8_t* buf, unsigned int size, HANDLE block)
{
    if (size < ARP_HEADER_SIZE)
    {
        tcpip_release_block(tcpip, block);
        return;
    }
    if (ARP_PRO(buf) != ETHERTYPE_IP || ARP_HLN(buf) != MAC_SIZE || ARP_PLN(buf) != IP_SIZE)
    {
        tcpip_release_block(tcpip, block);
        return;
    }
    switch (ARP_OP(buf))
    {
    case ARP_REQUEST:
        printf("ARP request: ");
        print_mac(ARP_SHA(buf));
        printf("(");
        print_ip(ARP_SPA(buf));
        printf(") -> ");
        print_mac(ARP_THA(buf));
        printf("(");
        print_ip(ARP_TPA(buf));
        printf(")\n\r");
        break;
    case ARP_REPLY:
        printf("TODO: ARP REPLY\n\r");
        break;
    }
    tcpip_release_block(tcpip, block);
}
