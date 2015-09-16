/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "mac.h"
#include "tcpips_private.h"
#include "../../userspace/stdio.h"
#include "arp.h"

#define MAC_DST(buf)                                        ((MAC*)(buf))
#define MAC_SRC(buf)                                        ((MAC*)((buf) + MAC_SIZE))
#define MAC_LENTYPE(buf)                                    (((buf)[2 * MAC_SIZE] << 8) | ((buf)[2 * MAC_SIZE + 1]))

#if (TCPIP_DEBUG)
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
    for (i = 0; i < MAC_SIZE; ++i)
    {
        printf("%02X", mac->u8[i]);
        if (i < MAC_SIZE - 1)
            printf(":");
    }
}
#endif

bool mac_compare(const MAC* src, const MAC* dst)
{
    return (src->u32.hi == dst->u32.hi) && (src->u32.lo == dst->u32.lo);
}

void mac_init(TCPIPS* tcpips)
{
    eth_get_mac(&tcpips->mac.mac);
}

const MAC* tcpip_mac(TCPIPS* tcpips)
{
    return &tcpips->mac.mac;
}

bool mac_request(TCPIPS* tcpips, IPC* ipc)
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

void mac_rx(TCPIPS* tcpips, IO* io)
{
    uint16_t lentype;
    if (io->data_size < MAC_HEADER_SIZE)
    {
        tcpips_release_io(tcpips, io);
        return;
    }
#if (MAC_FILTER)
    switch (MAC_DST(io_data(io))->u8[0] & MAC_CAST_MASK)
    {
    case MAC_BROADCAST_ADDRESS:
        //enable broadcast only for ARP
        if (MAC_LENTYPE((uint8_t*)io_data(io)) != ETHERTYPE_ARP)
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
        if (!mac_compare(&tcpips->mac.mac, MAC_DST(io_data(io))))
        {
            tcpips_release_io(tcpips, io);
            return;
        }
        break;
    }
#endif //MAC_FILTER

    lentype = MAC_LENTYPE((uint8_t*)io_data(io));
#if (TCPIP_MAC_DEBUG)
    printf("MAC RX: ");
    mac_print(MAC_SRC(io_data(io)));
    printf(" -> ");
    mac_print(MAC_DST(io_data(io)));
    printf(", len/type: %04X, size: %d\n", lentype, io->data_size - MAC_HEADER_SIZE);
#endif
    io->data_offset += MAC_HEADER_SIZE;
    io->data_size -= MAC_HEADER_SIZE;
    switch (lentype)
    {
    case ETHERTYPE_IP:
        ips_rx(tcpips, io);
        break;
    case ETHERTYPE_ARP:
        arp_rx(tcpips, io);
        break;
    default:
#if (TCPIP_MAC_DEBUG)
        printf("MAC: dropped lentype: %04X\n", lentype);
#endif
        tcpips_release_io(tcpips, io);
        break;
    }
}

IO* mac_allocate_io(TCPIPS* tcpips)
{
    IO* io = tcpips_allocate_io(tcpips);
    if (io == NULL)
        return NULL;
    io->data_offset += MAC_HEADER_SIZE;
    return io;
}

void mac_tx(TCPIPS* tcpips, IO* io, const MAC* dst, uint16_t lentype)
{
    io->data_offset -= MAC_HEADER_SIZE;
    io->data_size += MAC_HEADER_SIZE;

    MAC_DST(io_data(io))->u32.hi = dst->u32.hi;
    MAC_DST(io_data(io))->u32.lo = dst->u32.lo;
    MAC_SRC(io_data(io))->u32.hi = tcpips->mac.mac.u32.hi;
    MAC_SRC(io_data(io))->u32.lo = tcpips->mac.mac.u32.lo;

    ((uint8_t*)io_data(io))[2 * MAC_SIZE] = (lentype >> 8) & 0xff;
    ((uint8_t*)io_data(io))[2 * MAC_SIZE +  1] = lentype & 0xff;

    tcpips_tx(tcpips, io);
}
