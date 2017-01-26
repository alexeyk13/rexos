/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "macs.h"
#include "tcpips_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/endian.h"
#include "../../userspace/error.h"
#include "arps.h"
#include <string.h>

void macs_init(TCPIPS* tcpips)
{
    memset(&tcpips->macs.mac, 0, sizeof(MAC));
#if (MAC_FIREWALL)
    tcpips->macs.firewall_enabled = false;
#endif //MAC_FIREWALL
}

#if (MAC_FIREWALL)
void macs_enable_firewall(TCPIPS* tcpips, MAC* src)
{
    if (tcpips->macs.firewall_enabled)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    tcpips->macs.src.u32.hi = src->u32.hi;
    tcpips->macs.src.u32.lo = src->u32.lo;
    tcpips->macs.firewall_enabled = true;
}

void macs_disable_firewall(TCPIPS* tcpips)
{
    if (!tcpips->macs.firewall_enabled)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    tcpips->macs.firewall_enabled = false;
}
#endif //MAC_FIREWALL

void macs_request(TCPIPS* tcpips, IPC* ipc)
{
#if (MAC_FIREWALL)
    MAC src;
#endif //MAC_FIREWALL
    switch (HAL_ITEM(ipc->cmd))
    {
#if (MAC_FIREWALL)
    case MAC_ENABLE_FIREWALL:
        src.u32.hi = ipc->param2;
        src.u32.lo = ipc->param3;
        macs_enable_firewall(tcpips, &src);
        break;
    case MAC_DISABLE_FIREWALL:
        macs_disable_firewall(tcpips);
        break;
#endif //MAC_FIREWALL
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

void macs_link_changed(TCPIPS* tcpips, bool link)
{
    if (link)
    {
        eth_get_mac(tcpips->eth, tcpips->eth_handle, &tcpips->macs.mac);
    }
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
        if (!mac_compare(&tcpips->macs.mac, &hdr->dst))
        {
            tcpips_release_io(tcpips, io);
            return;
        }
        break;
    }
#endif //MAC_FILTER

#if (MAC_FIREWALL)
    if (tcpips->macs.firewall_enabled && !mac_compare(&tcpips->macs.src, &hdr->src))
    {
#if (TCPIP_MAC_DEBUG)
        printf("MAC: ");
        mac_print(&hdr->src);
        printf(" Firewall condition\n");
#endif //TCPIP_MAC_DEBUG
        tcpips_release_io(tcpips, io);
        return;
    }
#endif //MAC_FIREWALL

#if (TCPIP_MAC_DEBUG)
    printf("MAC RX: ");
    mac_print(&hdr->src);
    printf(" -> ");
    mac_print(&hdr->dst);
    printf(", len/type: %04X, size: %d\n", lentype, io->data_size - sizeof(MAC_HEADER));
#endif //TCPIP_MAC_DEBUG
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
#if ((TCPIP_MAC_DEBUG))
        printf("MAC: dropped lentype: %04X\n", lentype);
#endif //TCPIP_MAC_DEBUG
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
    hdr->src.u32.hi = tcpips->macs.mac.u32.hi;
    hdr->src.u32.lo = tcpips->macs.mac.u32.lo;
    short2be(hdr->lentype_be, lentype);

    tcpips_tx(tcpips, io);
}
