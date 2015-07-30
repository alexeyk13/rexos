/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "mac.h"
#include "tcpip_private.h"
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

void mac_init(TCPIP* tcpip)
{
    eth_get_mac(&tcpip->mac.mac);
}

const MAC* tcpip_mac(TCPIP* tcpip)
{
    return &tcpip->mac.mac;
}

bool mac_request(TCPIP* tcpip, IPC* ipc)
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

void mac_rx(TCPIP* tcpip, IO* io)
{
    uint16_t lentype;
    if (io->data_size < MAC_HEADER_SIZE)
    {
        tcpip_release_io(tcpip, io);
        return;
    }
#if (MAC_FILTER)
    switch (MAC_DST(io_data(io))->u8[0] & MAC_CAST_MASK)
    {
    case MAC_BROADCAST_ADDRESS:
        //enable broadcast only for ARP
        if (MAC_LENTYPE((uint8_t*)io_data(io)) != ETHERTYPE_ARP)
        {
            tcpip_release_io(tcpip, io);
            return;
        }
        break;
    case MAC_MULTICAST_ADDRESS:
        //drop all multicast
        tcpip_release_io(tcpip, io);
        return;
    default:
        //enable unicast only on address compare
        if (!mac_compare(&tcpip->mac.mac, MAC_DST(io_data(io))))
        {
            tcpip_release_io(tcpip, io);
            return;
        }
        break;
    }
#endif //MAC_FILTER

    lentype = MAC_LENTYPE((uint8_t*)io_data(io));
#if (MAC_DEBUG)
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
        ip_rx(tcpip, io);
        break;
    case ETHERTYPE_ARP:
        arp_rx(tcpip, io);
        break;
    default:
#if (MAC_DEBUG)
        printf("MAC: dropped lentype: %04X\n", lentype);
#endif
        tcpip_release_io(tcpip, io);
        break;
    }
}

IO* mac_allocate_io(TCPIP* tcpip)
{
    IO* io = tcpip_allocate_io(tcpip);
    if (io == NULL)
        return NULL;
    io->data_offset += MAC_HEADER_SIZE;
    return io;
}

void mac_tx(TCPIP* tcpip, IO* io, const MAC* dst, uint16_t lentype)
{
    io->data_offset -= MAC_HEADER_SIZE;
    io->data_size += MAC_HEADER_SIZE;

    MAC_DST(io_data(io))->u32.hi = dst->u32.hi;
    MAC_DST(io_data(io))->u32.lo = dst->u32.lo;
    MAC_SRC(io_data(io))->u32.hi = tcpip->mac.mac.u32.hi;
    MAC_SRC(io_data(io))->u32.lo = tcpip->mac.mac.u32.lo;

    ((uint8_t*)io_data(io))[2 * MAC_SIZE] = (lentype >> 8) & 0xff;
    ((uint8_t*)io_data(io))[2 * MAC_SIZE +  1] = lentype & 0xff;

    tcpip_tx(tcpip, io);
}
