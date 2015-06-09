/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "ip.h"
#include "tcpip_private.h"
#include "../../userspace/stdio.h"
#include "icmp.h"
#include <string.h>
#include "udp.h"

#define IP_DF                                   (1 << 6)
#define IP_MF                                   (1 << 5)

#define IP_HEADER_VERSION(buf)                  ((buf)[0] >> 4)
#define IP_HEADER_IHL(buf)                      ((buf)[0] & 0xf)
#define IP_HEADER_TOTAL_LENGTH(buf)             (((buf)[2] << 8) | (buf)[3])
#define IP_HEADER_ID(buf)                       (((buf)[4] << 8) | (buf)[5])
#define IP_HEADER_FLAGS(buf)                    ((buf)[6] & 0xe0)
#define IP_HEADER_FRAME_OFFSET(buf)             ((((buf)[6] & 0x1f) << 8) | (buf[7]))
#define IP_HEADER_TTL(buf)                      ((buf)[8])
#define IP_HEADER_PROTOCOL(buf)                 ((buf)[9])
#define IP_HEADER_CHECKSUM(buf)                 (((buf)[10] << 8) | (buf)[11])
#define IP_HEADER_SRC_IP(buf)                   ((IP*)((buf) + 12))
#define IP_HEADER_DST_IP(buf)                   ((IP*)((buf) + 16))


#if (TCPIP_DEBUG)
void ip_print(const IP* ip)
{
    int i;
    for (i = 0; i < IP_SIZE; ++i)
    {
        printf("%d", ip->u8[i]);
        if (i < IP_SIZE - 1)
            printf(".");
    }
}
#endif // (TCPIP_DEBUG)

const IP* tcpip_ip(TCPIP* tcpip)
{
    return &tcpip->ip.ip;
}

void ip_init(TCPIP* tcpip)
{
    tcpip->ip.ip.u32.ip = IP_MAKE(0, 0, 0, 0);
    tcpip->ip.id = 0;
}

static inline void ip_set_request(TCPIP* tcpip, uint32_t ip)
{
    tcpip->ip.ip.u32.ip = ip;
}

static inline void ip_get_request(TCPIP* tcpip, HANDLE process)
{
    ipc_post_inline(process, HAL_CMD(HAL_IP, IP_GET), 0, tcpip->ip.ip.u32.ip, 0);
}

bool ip_request(TCPIP* tcpip, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IP_SET:
        ip_set_request(tcpip, ipc->param1);
        need_post = true;
        break;
    case IP_GET:
        ip_get_request(tcpip, ipc->process);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

IO *ip_allocate_io(TCPIP* tcpip, unsigned int size, uint8_t proto)
{
    IP_STACK* ip_stack;
    IO* io = mac_allocate_io(tcpip);
    //TODO: fragmented frames
    if (io == NULL)
        return NULL;

    io_push(io, sizeof(IP_STACK));
    ip_stack = io_stack(io);

    //reserve space for IP header
    io->data_offset += IP_HEADER_SIZE;
    ip_stack->proto = proto;
    ip_stack->hdr_size = IP_HEADER_SIZE;
    return io;
}

void ip_release_io(TCPIP* tcpip, IO* io)
{
    //TODO: fragmented frames
    tcpip_release_io(tcpip, io);
}

void ip_tx(TCPIP* tcpip, IO* io, const IP* dst)
{
    IP_STACK* ip_stack;
    ip_stack = io_stack(io);
    //TODO: fragmented frames
    uint16_t cs;
    unsigned int size = io->data_size;
    io->data_offset -= ip_stack->hdr_size;
    io->data_size += ip_stack->hdr_size;

    //VERSION, IHL
    ((uint8_t*)io_data(io))[0] = 0x45;
    //DSCP, ECN
    ((uint8_t*)io_data(io))[1] = 0;
    //total len
    ((uint8_t*)io_data(io))[2] = (size >> 8) & 0xff;
    ((uint8_t*)io_data(io))[3] = size & 0xff;
    //id
    ((uint8_t*)io_data(io))[4] = (tcpip->ip.id >> 8) & 0xff;
    ((uint8_t*)io_data(io))[5] = tcpip->ip.id & 0xff;
    ++tcpip->ip.id;
    //flags, offset
    ((uint8_t*)io_data(io))[6] = ((uint8_t*)io_data(io))[7] = 0;
    //ttl
    ((uint8_t*)io_data(io))[8] = 0xff;
    //proto
    ((uint8_t*)io_data(io))[9] = ip_stack->proto;
    //src
    *(uint32_t*)(((uint8_t*)io_data(io)) + 12) = tcpip_ip(tcpip)->u32.ip;
    //dst
    *(uint32_t*)(((uint8_t*)io_data(io)) + 16) = dst->u32.ip;
    //update checksum
    ((uint8_t*)io_data(io))[10] = ((uint8_t*)io_data(io))[11] = 0;
    cs = ip_checksum(((uint8_t*)io_data(io)), ip_stack->hdr_size);
    ((uint8_t*)io_data(io))[10] = (cs >> 8) & 0xff;
    ((uint8_t*)io_data(io))[11] = cs & 0xff;

    io_pop(io, sizeof(IP_STACK));
    route_tx(tcpip, io, dst);
}

static void ip_process(TCPIP* tcpip, IO* io, IP* src)
{
    IP_STACK* ip_stack;
    ip_stack = io_stack(io);
#if (IP_DEBUG_FLOW)
    printf("IP: from ");
    ip_print(src);
    printf(", proto: %d, len: %d\n\r", ip_stack->proto, io->data_size);
#endif
    switch (ip_stack->proto)
    {
#if (ICMP)
    case PROTO_ICMP:
        icmp_rx(tcpip, io, src);
        break;
#endif //ICMP
#if (UDP)
    case PROTO_UDP:
        udp_rx(tcpip, io, src);
        break;
    default:
#endif //UDP
#if (IP_DEBUG)
        printf("IP: unhandled proto %d from", ip_stack->proto);
        ip_print(src);
        printf("\n\r");
#endif
#if (ICMP_FLOW_CONTROL)
        icmp_destination_unreachable(tcpip, ICMP_PROTOCOL_UNREACHABLE, io, src);
#else
        ip_release_io(tcpip, io);
#endif
    }
}

#if (IP_CHECKSUM)
uint16_t ip_checksum(uint8_t* buf, unsigned int size)
{
    unsigned int i;
    uint32_t sum = 0;
    for (i = 0; i < (size >> 1); ++i)
        sum += (buf[i << 1] << 8) | (buf[(i << 1) + 1]);
    sum = ((sum & 0xffff) + (sum >> 16)) & 0xffff;
    return ~((uint16_t)sum);
}
#endif

void ip_rx(TCPIP* tcpip, IO* io)
{
    IP src;
    IP_STACK* ip_stack;
    uint8_t* ip_header = io_data(io);
    if (io->data_size < IP_HEADER_SIZE)
    {
        tcpip_release_io(tcpip, io);
        return;
    }
    io_push(io, sizeof(IP_STACK));
    ip_stack = io_stack(io);

    ip_stack->hdr_size = IP_HEADER_IHL(ip_header) << 2;
#if (IP_CHECKSUM)
    //drop if checksum is invalid
    if (ip_checksum(ip_header, ip_stack->hdr_size))
    {
        tcpip_release_io(tcpip, io);
        return;
    }
#endif
    src.u32.ip = IP_HEADER_SRC_IP(ip_header)->u32.ip;
    ip_stack->proto = IP_HEADER_PROTOCOL(ip_header);

    //len more than MTU, inform host by ICMP and only than drop packet
    if (IP_HEADER_TOTAL_LENGTH(ip_header) > io->data_size)
    {
#if (ICMP_FLOW_CONTROL)
        icmp_parameter_problem(tcpip, 2, io, &src);
#else
        tcpip_release_io(tcpip, io);
#endif
        return;
    }

    io->data_size = IP_HEADER_TOTAL_LENGTH(ip_header);
    io->data_offset += ip_stack->hdr_size;
    io->data_size -= ip_stack->hdr_size;
    if ((IP_HEADER_VERSION(ip_header) != 4) || (ip_stack->hdr_size < IP_HEADER_SIZE))
    {
#if (ICMP_FLOW_CONTROL)
        icmp_parameter_problem(tcpip, 0, io, &src);
#else
        tcpip_release_io(tcpip, io);
#endif
        return;
    }
    //unicast-only filter
    if (tcpip_ip(tcpip)->u32.ip != IP_HEADER_DST_IP(ip_header)->u32.ip)
    {
        tcpip_release_io(tcpip, io);
        return;
    }

    //ttl exceeded
    if (IP_HEADER_TTL(ip_header) == 0)
    {
#if (ICMP_FLOW_CONTROL)
        icmp_time_exceeded(tcpip, ICMP_TTL_EXCEED_IN_TRANSIT, io, &src);
#else
        tcpip_release_io(tcpip, io);
#endif
        return;
    }

#if (IP_FRAGMENTATION)
    //TODO: packet assembly from fragments
#error IP FRAGMENTATION is Not Implemented!
#else
    //drop all fragmented frames, inform by ICMP
    if ((IP_HEADER_FLAGS(ip_header) & IP_MF) || (IP_HEADER_FRAME_OFFSET(ip_header)))
    {
#if (ICMP_FLOW_CONTROL)
        icmp_parameter_problem(tcpip, 6, io, &src);
#else
        tcpip_release_io(tcpip, io);
#endif
        return;
    }
#endif

    ip_process(tcpip, io, &src);
}
