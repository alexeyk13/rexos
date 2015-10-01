/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "udps.h"
#include "tcpips_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/endian.h"
#include "../../userspace/udp.h"
#include "sys_config.h"

#pragma pack(push, 1)
typedef struct {
    uint8_t src_port_be[2];
    uint8_t dst_port_be[2];
    uint8_t len_be[2];
    uint8_t checksum_be[2];
} UDP_HEADER;

typedef struct {
    IP src;
    IP dst;
    uint8_t zero;
    uint8_t proto;
    uint8_t length_be[2];
} UDP_PSEUDO_HEADER;
#pragma pack(pop)

static uint16_t udps_checksum(IO* io, const IP* src, const IP* dst)
{
    UDP_PSEUDO_HEADER uph;
    uint16_t sum;
    uph.src.u32.ip = src->u32.ip;
    uph.dst.u32.ip = dst->u32.ip;
    uph.zero = 0;
    uph.proto = PROTO_UDP;
    short2be(uph.length_be, io->data_size);
    sum = ~ip_checksum(&uph, sizeof(UDP_PSEUDO_HEADER));
    sum += ~ip_checksum(io_data(io), io->data_size);
    sum = ~(((sum & 0xffff) + (sum >> 16)) & 0xffff);
    return sum;
}

static void udps_release_io(TCPIPS* tcpips, IO* io)
{
    io->data_offset -= sizeof(UDP_HEADER);
    io->data_size += sizeof(UDP_HEADER);
    io_pop(io, sizeof(UDP_STACK));
    ips_release_io(tcpips, io);
}

void udps_init(TCPIPS* tcpips)
{
    //TODO:
}

void udps_rx(TCPIPS* tcpips, IO* io, IP* src)
{
    UDP_STACK* udp_stack;
    UDP_HEADER* hdr;
    if (io->data_size < sizeof(UDP_HEADER) || udps_checksum(io, src, &tcpips->ip))
    {
        ips_release_io(tcpips, io);
        return;
    }
    hdr = io_data(io);
    udp_stack = io_push(io, sizeof(UDP_STACK));
    udp_stack->remote_addr.u32.ip = src->u32.ip;
    udp_stack->dst_port = be2short(hdr->dst_port_be);
    udp_stack->src_port = be2short(hdr->src_port_be);
    io->data_offset += sizeof(UDP_HEADER);
    io->data_size -= sizeof(UDP_HEADER);
#if (UDP_DEBUG_FLOW)
    printf("UDP: ");
    ip_print(src);
    printf(":%d -> ", udp_stack->src_port);
    ip_print(&tcpips->ip);
    printf(":%d, %d byte(s)\n", udp_stack->dst_port, io->data_size);
#endif //UDP_DEBUG_FLOW

    //TODO: process UDP
    printf("ch: %c\n", ((char*)io_data(io))[0]);
    udps_release_io(tcpips, io);
}

