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

typedef struct {
    HANDLE process;
    uint16_t port;
} TCP_LISTEN_HANDLE;

typedef struct {
    HANDLE process;
    uint16_t remote_port, local_port, mss;
    IP remote_addr;
    IO* head;
    //TODO: icmp?
} TCP_ACTIVE_HANDLE;

#if (TCP_DEBUG_FLOW)
static const char* __TCP_FLAGS[TCP_FLAGS_COUNT] =                   {"FIN", "SYN", "RST", "PSH", "ACK", "URG"};

    bool has_flag;
    int i;
#endif //TCP_DEBUG_FLOW

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

static HANDLE tcps_find_listen(TCPIPS* tcpips, uint16_t port)
{
    HANDLE handle;
    TCP_LISTEN_HANDLE* tlh;
    for (handle = so_first(&tcpips->tcps.listen); handle != INVALID_HANDLE; handle = so_next(&tcpips->tcps.listen, handle))
    {
        tlh = so_get(&tcpips->tcps.listen, handle);
        if (tlh->port == port)
            return handle;
    }
    return INVALID_HANDLE;
}

//TODO:
/*
static inline uint16_t tcps_allocate_port(TCPIPS* tcpips)
{
    unsigned int res;
    //from current to HI
    for (res = tcpips->tcps.dynamic; res <= TCPIP_DYNAMIC_RANGE_HI; ++res)
    {
        if (udps_find(tcpips, res) == INVALID_HANDLE)
        {
            tcpips->tcps.dynamic = res == TCPIP_DYNAMIC_RANGE_HI ? TCPIP_DYNAMIC_RANGE_LO : res + 1;
            return (uint16_t)res;
        }

    }
    //from LO to current
    for (res = TCPIP_DYNAMIC_RANGE_LO; res < tcpips->tcps.dynamic; ++res)
    {
        if (udps_find(tcpips, res) == INVALID_HANDLE)
        {
            tcpips->udps.dynamic = res + 1;
            return res;
        }

    }
    error(ERROR_TOO_MANY_HANDLES);
    return 0;
}
*/

void tcps_init(TCPIPS* tcpips)
{
    so_create(&tcpips->tcps.listen, sizeof(TCP_LISTEN_HANDLE), 1);
    so_create(&tcpips->tcps.active, sizeof(TCP_ACTIVE_HANDLE), 1);
}

void tcps_rx(TCPIPS* tcpips, IO* io, IP* src)
{
#if (TCP_DEBUG_FLOW)
    bool has_flag;
    int i;
#endif //TCP_DEBUG_FLOW

    HANDLE handle;
    TCP_HEADER* hdr;
//TODO:    TCP_HANDLE* th;
    uint16_t src_port, dst_port;
    uint8_t data_offset;
    if (io->data_size < sizeof(TCP_HEADER) || tcps_checksum(io, src, &tcpips->ip))
    {
        ips_release_io(tcpips, io);
        return;
    }
    hdr = io_data(io);
    src_port = be2short(hdr->src_port_be);
    dst_port = be2short(hdr->dst_port_be);
    data_offset = (hdr->data_off >> 4) << 2;
#if (TCP_DEBUG_FLOW)
    printf("TCP: ");
    ip_print(src);
    printf(":%d -> ", src_port);
    ip_print(&tcpips->ip);
    printf(" ");
    if (hdr->flags & TCP_FLAG_MSK)
    {
        printf("<CTL=");
        has_flag = false;
        for (i = 0; i < TCP_FLAGS_COUNT; ++i)
            if (hdr->flags & (1 << i))
            {
                if (has_flag)
                    printf(",");
                printf(__TCP_FLAGS[i]);
            }
        printf(">");
    }
    if (io->data_size - data_offset)
        printf("<DATA=%d byte(s)>", io->data_size - data_offset);
    printf("\n");
#endif //TCP_DEBUG_FLOW

    //TODO: search in active handles
    if ((handle = tcps_find_listen(tcpips, dst_port)) != INVALID_HANDLE)
    {
        //TODO:

    }
    else
    {
#if (TCP_DEBUG)
        printf("TCP: no connection, RST\n");
#endif //TCP_DEBUG
        //TODO: send RST
    }
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

static inline void tcps_listen(TCPIPS* tcpips, IPC* ipc)
{
    HANDLE handle;
    TCP_LISTEN_HANDLE* tlh;
    if (tcps_find_listen(tcpips, (uint16_t)ipc->param1) != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    handle = so_allocate(&tcpips->tcps.listen);
    if (handle == INVALID_HANDLE)
        return;
    tlh = so_get(&tcpips->tcps.listen, handle);
    tlh->port = (uint16_t)ipc->param1;
    tlh->process = ipc->process;
    ipc->param2 = handle;
}

static inline void tcps_connect(TCPIPS* tcpips, IPC* ipc)
{
    //TODO:
/*    HANDLE handle;
    UDP_HANDLE* uh;
    IP dst;
    uint16_t local_port;
    dst.u32.ip = ipc->param2;
    local_port = udps_allocate_port(tcpips);
    if ((local_port = udps_allocate_port(tcpips)) == 0)
        return;
    if ((handle = so_allocate(&tcpips->udps.handles)) == INVALID_HANDLE)
        return;
    uh = so_get(&tcpips->udps.handles, handle);
    uh->remote_port = (uint16_t)ipc->param1;
    uh->local_port = local_port;
    uh->remote_addr.u32.ip = dst.u32.ip;
    uh->process = ipc->process;
    uh->head = NULL;
#if (ICMP)
    uh->err = ERROR_OK;
#endif //ICMP
    ipc->param2 = handle;*/
    error(ERROR_NOT_SUPPORTED);
}

void tcps_request(TCPIPS* tcpips, IPC* ipc)
{
    if (!tcpips->connected)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        if (ipc->param2 == LOCALHOST)
            tcps_listen(tcpips, ipc);
        else
            tcps_connect(tcpips, ipc);
        break;
    case IPC_CLOSE:
        //TODO:
//        tcps_close(tcpips, ipc->param1);
        break;
    case IPC_READ:
        //TODO:
//        tcps_read(tcpips, ipc->param1, (IO*)ipc->param2);
        break;
    case IPC_WRITE:
        //TODO:
//        tcps_write(tcpips, ipc->param1, (IO*)ipc->param2);
        break;
    case IPC_FLUSH:
        //TODO:
//        tcps_flush(tcpips, ipc->param1);
        break;
    //TODO: accept/reject
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}
