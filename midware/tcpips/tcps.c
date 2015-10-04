/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "tcps.h"
#include "tcpips_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/endian.h"

#pragma pack(push, 1)
typedef struct {
    uint8_t src_port_be[2];
    uint8_t dst_port_be[2];
    uint8_t seq_be[4];
    uint8_t ack_be[4];
    //not byte aligned
    uint8_t data_off;
    uint8_t flags;
    uint8_t window_be[2];
    uint8_t checksum_be[2];
    uint8_t urgent_pointer_be[2];
    uint8_t options[3];
    uint8_t padding;
} TCP_HEADER;

typedef struct {
    IP src;
    IP dst;
    uint8_t zero;
    uint8_t ptcl;
    uint8_t length_be[2];
} TCP_PSEUDO_HEADER;
#pragma pack(pop)

static uint16_t tcps_checksum(IO* io, const IP* src, const IP* dst)
{
    TCP_PSEUDO_HEADER tph;
    uint16_t sum;
    tph.src.u32.ip = src->u32.ip;
    tph.dst.u32.ip = dst->u32.ip;
    tph.zero = 0;
    tph.ptcl = PROTO_TCP;
    short2be(tph.length_be, io->data_size);
    sum = ~ip_checksum(&tph, sizeof(TCP_PSEUDO_HEADER));
    sum += ~ip_checksum(io_data(io), io->data_size);
    sum = ~(((sum & 0xffff) + (sum >> 16)) & 0xffff);
    return sum;
}

void tcps_rx(TCPIPS* tcpips, IO* io, IP* src)
{
//TODO:    HANDLE handle;
    TCP_HEADER* hdr;
//TODO:    TCP_HANDLE* th;
    uint16_t src_port, dst_port;
    if (io->data_size < sizeof(TCP_HEADER) || tcps_checksum(io, src, &tcpips->ip))
    {
        ips_release_io(tcpips, io);
        return;
    }
    hdr = io_data(io);
    src_port = be2short(hdr->src_port_be);
    dst_port = be2short(hdr->dst_port_be);

#if (TCP_DEBUG_FLOW)
    printf("TCP: ");
    ip_print(src);
    printf(":%d -> ", src_port);
    ip_print(&tcpips->ip);
    //TODO: size is wrong
    printf(":%d, %d byte(s)\n", dst_port, io->data_size - sizeof(TCP_HEADER));
#endif //TCP_DEBUG_FLOW

#if 0
    //search in listeners
    handle = udps_find(tcpips, dst_port);
    if (handle != INVALID_HANDLE)
    {
        uh = so_get(&tcpips->udps.handles, handle);
        //listener or connected
        if (uh->remote_port == 0 || (uh->remote_port == src_port && uh->remote_addr.u32.ip == src->u32.ip))
            udps_send_user(tcpips, src, io, handle);
        else
            handle = INVALID_HANDLE;
    }

    if (handle != INVALID_HANDLE)
        ips_release_io(tcpips, io);
    else
    {
#if (TCP_DEBUG)
        printf("TCP: no connection, datagramm dropped\n");
#endif //TCP_DEBUG
#if (ICMP)
        icmps_tx_error(tcpips, io, ICMP_ERROR_PORT, 0);
#endif //ICMP
    }
#endif //0

//    dump(io_data(io), io->data_size);
    ips_release_io(tcpips, io);
}
