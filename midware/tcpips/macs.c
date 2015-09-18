/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "macs.h"
#include "tcpips_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/endian.h"
#include "arps.h"
#include <string.h>

void macs_init(TCPIPS* tcpips)
{
    memset(&tcpips->mac, 0, sizeof(MAC));
}

void macs_open(TCPIPS* tcpips)
{
    eth_get_mac(tcpips->eth, tcpips->eth_handle, &tcpips->mac);
}

bool macs_request(TCPIPS* tcpips, IPC* ipc)
{
    bool need_post;
    switch (HAL_ITEM(ipc->cmd))
    {
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

void macs_rx(TCPIPS* tcpips, IO* io)
{
    uint16_t lentype;
    MAC_HEADER* hdr = io_data(io);
    if (io->data_size < sizeof(MAC_HEADER))
    {
        tcpips_release_io(tcpips, io);
        return;
    }
    lentype = be2short(hdr->lentype_be);

#if (MAC_FILTER)
    switch (hdr->dst.u8[0] & MAC_CAST_MASK)
    {
    case MAC_BROADCAST_ADDRESS:
        //enable broadcast only for ARP
        if (lentype != ETHERTYPE_ARP)
        {
            tcpips_release_io(tcpips, io);
            return;
        }
        break;
    case MAC_MULTICAST_ADDRESS:
        //drop all multicast
        tcpips_release_io(tcpips, io);
        return;
    default:
        //enable unicast only on address compare
        if (!mac_compare(&tcpips->mac, &hdr->dst))
        {
            tcpips_release_io(tcpips, io);
            return;
        }
        break;
    }
#endif //MAC_FILTER

#if (TCPIP_MAC_DEBUG)
    printf("MAC RX: ");
    mac_print(&hdr->src);
    printf(" -> ");
    mac_print(&hdr->dst);
    printf(", len/type: %04X, size: %d\n", lentype, io->data_size - sizeof(MAC_HEADER));
#endif
    io->data_offset += sizeof(MAC_HEADER);
    io->data_size -= sizeof(MAC_HEADER);
    switch (lentype)
    {
    case ETHERTYPE_IP:
        ips_rx(tcpips, io);
        break;
    case ETHERTYPE_ARP:
        arps_rx(tcpips, io);
        break;
    default:
#if (TCPIP_MAC_DEBUG)
        printf("MAC: dropped lentype: %04X\n", lentype);
#endif
        tcpips_release_io(tcpips, io);
        break;
    }
}

IO* macs_allocate_io(TCPIPS* tcpips)
{
    IO* io = tcpips_allocate_io(tcpips);
    if (io == NULL)
        return NULL;
    io->data_offset += sizeof(MAC_HEADER);
    return io;
}

void macs_tx(TCPIPS* tcpips, IO* io, const MAC* dst, uint16_t lentype)
{
    io->data_offset -= sizeof(MAC_HEADER);
    io->data_size += sizeof(MAC_HEADER);
    MAC_HEADER* hdr = io_data(io);

    hdr->dst.u32.hi = dst->u32.hi;
    hdr->dst.u32.lo = dst->u32.lo;
    hdr->src.u32.hi = tcpips->mac.u32.hi;
    hdr->src.u32.lo = tcpips->mac.u32.lo;
    short2be(hdr->lentype_be, lentype);

    tcpips_tx(tcpips, io);
}
