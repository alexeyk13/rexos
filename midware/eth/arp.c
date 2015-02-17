/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "arp.h"
#include "tcpip_private.h"
#include "../../userspace/stdio.h"

void arp_rx(TCPIP* tcpip, MAC* src, void* buf, unsigned int size, HANDLE block)
{
    ARP_HEADER* h;
    if (size < sizeof(ARP_HEADER))
    {
        tcpip_release_block(tcpip, block);
        return;
    }

    h = (ARP_HEADER*)buf;
    printf("ARP RX\n\r");
    printf("hrd: %X\n\r", h->hrd);
    printf("pro: %X\n\r", h->pro);
    printf("hln: %X\n\r", h->hln);
    printf("pln: %X\n\r", h->pln);
    printf("op: %X\n\r", h->op);

    tcpip_release_block(tcpip, block);
}
