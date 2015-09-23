/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "ips.h"
#include "tcpips_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/endian.h"
#include "icmp.h"
#include <string.h>
#include "udp.h"

#define IP_DF                                   (1 << 6)
#define IP_MF                                   (1 << 5)
#define IP_FLAGS_MASK                           (7 << 5)

void ips_init(TCPIPS* tcpips)
{
    tcpips->ip.u32.ip = IP_MAKE(0, 0, 0, 0);
}

void ips_open(TCPIPS* tcpips)
{
    tcpips->ips.id = 0;
}

static inline void ips_set(TCPIPS* tcpips, uint32_t ip)
{
    tcpips->ip.u32.ip = ip;
}

static inline void ips_get(TCPIPS* tcpips, HANDLE process)
{
    ipc_post_inline(process, HAL_CMD(HAL_IP, IP_GET), 0, tcpips->ip.u32.ip, 0);
}

bool ips_request(TCPIPS* tcpips, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IP_SET:
        ips_set(tcpips, ipc->param2);
        need_post = true;
        break;
    case IP_GET:
        ips_get(tcpips, ipc->process);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

IO *ips_allocate_io(TCPIPS* tcpips, unsigned int size, uint8_t proto)
{
    IP_STACK* ip_stack;
    IO* io = macs_allocate_io(tcpips);
    //TODO: fragmented frames
    if (io == NULL)
        return NULL;

    ip_stack = io_push(io, sizeof(IP_STACK));

    //reserve space for IP header
    io->data_offset += sizeof(IP_HEADER);
    ip_stack->proto = proto;
    ip_stack->hdr_size = sizeof(IP_HEADER);
    return io;
}

void ips_release_io(TCPIPS* tcpips, IO* io)
{
    //TODO: fragmented frames
    tcpips_release_io(tcpips, io);
}

void ips_tx(TCPIPS* tcpips, IO* io, const IP* dst)
{
    IP_HEADER* hdr;
    IP_STACK* ip_stack = io_stack(io);
    //TODO: fragmented frames
    io->data_offset -= ip_stack->hdr_size;
    io->data_size += ip_stack->hdr_size;
    hdr = io_data(io);

    //VERSION, IHL
    hdr->ver_ihl = 0x45;
    //DSCP, ECN
    hdr->tos = 0;
    //total len
    short2be(hdr->total_len_be, io->data_size);
    //id
    short2be(hdr->id, tcpips->ips.id++);
    //flags, offset
    hdr->flags_offset_be[0] = hdr->flags_offset_be[1] = 0;
    //ttl
    hdr->ttl = 0xff;
    //proto
    hdr->proto = ip_stack->proto;
    //src
    hdr->src.u32.ip = tcpips->ip.u32.ip;
    //dst
    hdr->dst.u32.ip = dst->u32.ip;
    //update checksum
    short2be(hdr->header_crc_be, 0);
    short2be(hdr->header_crc_be, ip_checksum(io_data(io), ip_stack->hdr_size));

    io_pop(io, sizeof(IP_STACK));
    routes_tx(tcpips, io, dst);
}

static void ips_process(TCPIPS* tcpips, IO* io, IP* src)
{
    IP_STACK* ip_stack = io_stack(io);
#if (IP_DEBUG_FLOW)
    printf("IP: from ");
    ip_print(src);
    printf(", proto: %d, len: %d\n", ip_stack->proto, io->data_size);
#endif
    switch (ip_stack->proto)
    {
#if (ICMP)
    case PROTO_ICMP:
        icmp_rx(tcpips, io, src);
        break;
#endif //ICMP
#if (UDP)
    case PROTO_UDP:
        udp_rx(tcpips, io, src);
        break;
    default:
#endif //UDP
#if (IP_DEBUG)
        printf("IP: unhandled proto %d from", ip_stack->proto);
        ip_print(src);
        printf("\n");
#endif
#if (ICMP_FLOW_CONTROL)
        icmp_destination_unreachable(tcpips, ICMP_PROTOCOL_UNREACHABLE, io, src);
#else
        ips_release_io(tcpips, io);
#endif
    }
}

void ips_rx(TCPIPS* tcpips, IO* io)
{
    IP src;
    uint16_t total_len, offset;
    uint8_t flags;
    IP_STACK* ip_stack;
    IP_HEADER* hdr = io_data(io);
    if (io->data_size < sizeof(IP_HEADER))
    {
        tcpips_release_io(tcpips, io);
        return;
    }
    ip_stack = io_push(io, sizeof(IP_STACK));

    ip_stack->hdr_size = (hdr->ver_ihl & 0xf) << 2;
#if (IP_CHECKSUM)
    //drop if checksum is invalid
    if (ip_checksum(io_data(io), ip_stack->hdr_size))
    {
        tcpips_release_io(tcpips, io);
        return;
    }
#endif
    src.u32.ip = hdr->src.u32.ip;
    total_len = be2short(hdr->total_len_be);
    ip_stack->proto = hdr->proto;

    //len more than MTU, inform host by ICMP and only than drop packet
    if (total_len > io->data_size)
    {
#if (ICMP_FLOW_CONTROL)
        icmp_parameter_problem(tcpips, 2, io, &src);
#else
        tcpips_release_io(tcpips, io);
#endif
        return;
    }

    io->data_size = total_len;
    io->data_offset += ip_stack->hdr_size;
    io->data_size -= ip_stack->hdr_size;
    if (((hdr->ver_ihl >> 4) != 4) || (ip_stack->hdr_size < sizeof(IP_HEADER)))
    {
#if (ICMP_FLOW_CONTROL)
        icmp_parameter_problem(tcpips, 0, io, &src);
#else
        tcpips_release_io(tcpips, io);
#endif
        return;
    }
    //unicast-only filter
    if (tcpips->ip.u32.ip != hdr->dst.u32.ip)
    {
        tcpips_release_io(tcpips, io);
        return;
    }

    //ttl exceeded
    if (hdr->ttl == 0)
    {
#if (ICMP_FLOW_CONTROL)
        icmp_time_exceeded(tcpips, ICMP_TTL_EXCEED_IN_TRANSIT, io, &src);
#else
        tcpips_release_io(tcpips, io);
#endif
        return;
    }

    flags = hdr->flags_offset_be[0] & IP_FLAGS_MASK;
    offset = ((hdr->flags_offset_be[0] & ~IP_FLAGS_MASK) << 8) | hdr->flags_offset_be[1];
#if (IP_FRAGMENTATION)
    //TODO: packet assembly from fragments
#error IP FRAGMENTATION is Not Implemented!
#else
    //drop all fragmented frames, inform by ICMP
    if ((flags & IP_MF) || offset)
    {
#if (ICMP_FLOW_CONTROL)
        icmp_parameter_problem(tcpips, 6, io, &src);
#else
        tcpips_release_io(tcpips, io);
#endif
        return;
    }
#endif

    ips_process(tcpips, io, &src);
}
