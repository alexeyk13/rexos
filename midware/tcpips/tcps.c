/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "tcps.h"
#include "tcpips_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/endian.h"
#include "../../userspace/systime.h"
#include "icmps.h"

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

typedef enum {
    TCP_STATE_CLOSED,
    TCP_STATE_LISTEN,
    TCP_STATE_SYN_SENT,
    TCP_STATE_SYN_RECEIVED,
    TCP_STATE_ESTABLISHED,
    TCP_STATE_FIN_WAIT_1,
    TCP_STATE_FIN_WAIT_2,
    TCP_STATE_CLOSE_WAIT,
    TCP_STATE_CLOSING,
    TCP_STATE_LAST_ACK,
    TCP_STATE_TIME_WAIT
} TCP_STATE;

typedef struct {
    HANDLE process;
    IP remote_addr;
    IO* head;
    uint32_t snd_una, snd_nxt, rcv_nxt;
    TCP_STATE state;
    uint16_t remote_port, local_port, mss;
    //TODO: icmp?
} TCP_TCB;

#if (TCP_DEBUG_FLOW)
static const char* __TCP_FLAGS[TCP_FLAGS_COUNT] =                   {"FIN", "SYN", "RST", "PSH", "ACK", "URG"};

void tcps_debug(IO* io, const IP* src, const IP* dst)
{
    bool has_flag;
    int i;
    unsigned int data_size;
    TCP_HEADER* hdr = io_data(io);

    printf("TCP: ");
    ip_print(src);
    printf(":%d -> ", be2short(hdr->src_port_be));
    ip_print(dst);
    printf(":%d <SEQ=%u>", be2short(hdr->dst_port_be), be2int(hdr->seq_be));
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
    data_size = io->data_size - ((hdr->data_off >> 4) << 2);
    if (data_size)
        printf("<DATA=%d byte(s)>", data_size);
    printf("\n");
}
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

static uint32_t tcps_gen_isn()
{
    SYSTIME uptime;
    get_uptime(&uptime);
    //increment every 4us
    return (uptime.sec % 17179) + (uptime.usec >> 2);
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

static HANDLE tcps_find_tcb(TCPIPS* tcpips, const IP* src, uint16_t remote_port, uint16_t local_port)
{
    HANDLE handle;
    TCP_TCB* tcb;
    for (handle = so_first(&tcpips->tcps.tcbs); handle != INVALID_HANDLE; handle = so_next(&tcpips->tcps.tcbs, handle))
    {
        tcb = so_get(&tcpips->tcps.tcbs, handle);
        if (tcb->remote_port == remote_port && tcb->local_port == local_port && tcb->remote_addr.u32.ip == src->u32.ip)
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

static HANDLE tcps_create_tcb(TCPIPS* tcpips, const IP* remote_addr, uint16_t remote_port, uint16_t local_port, HANDLE process)
{
    TCP_TCB* tcb;
    HANDLE handle = so_allocate(&tcpips->tcps.tcbs);
    if (handle == INVALID_HANDLE)
        return handle;
    tcb = so_get(&tcpips->tcps.tcbs, handle);
    tcb->process = process;
    tcb->remote_addr.u32.ip = remote_addr->u32.ip;
    tcb->head = NULL;
    tcb->snd_una = tcb->snd_nxt = tcps_gen_isn();
    tcb->rcv_nxt = 0;
    tcb->state = TCP_STATE_CLOSED;
    tcb->remote_port = remote_port;
    tcb->local_port = local_port;
    tcb->mss = IP_FRAME_MAX_DATA_SIZE - sizeof(TCP_HEADER);
    return handle;
}

void tcps_init(TCPIPS* tcpips)
{
    so_create(&tcpips->tcps.listen, sizeof(TCP_LISTEN_HANDLE), 1);
    so_create(&tcpips->tcps.tcbs, sizeof(TCP_TCB), 1);
}

static inline void tcps_rx_syn(TCPIPS* tcpips, IO* io, IP* src, HANDLE handle)
{
    TCP_HEADER* hdr;
    TCP_LISTEN_HANDLE* tlh;
    TCP_TCB* tcb;
    HANDLE tcb_handle;
#if (TCP_DEBUG_FLOW)
    printf("TCP: SYN received, creating connection\n");
#endif //TCP_DEBUG_FLOW
    hdr = io_data(io);
    tlh = so_get(&tcpips->tcps.listen, handle);
    tcb_handle = tcps_create_tcb(tcpips, src, be2short(hdr->src_port_be), tlh->port, tlh->process);
    if (tcb_handle == INVALID_HANDLE)
    {
        ips_release_io(tcpips, io);
        return;
    }
    tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    tcb->snd_nxt = tcb->snd_una = tcps_gen_isn();
    tcb->state = TCP_STATE_SYN_RECEIVED;
    //TODO: decode options
    ips_release_io(tcpips, io);
}

static inline void tcps_rx_fin(TCPIPS* tcpips, IO* io, HANDLE handle)
{
    //TODO:
}

static inline void tcps_rx_process(TCPIPS* tcpips, IO* io, HANDLE handle)
{
    //TODO:
}

void tcps_rx(TCPIPS* tcpips, IO* io, IP* src)
{
    TCP_HEADER* hdr;
    HANDLE handle;
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
    tcps_debug(io, src, &tcpips->ip);
#endif //TCP_DEBUG_FLOW

    handle = tcps_find_tcb(tcpips, src, src_port, dst_port);
    if (handle != INVALID_HANDLE)
    {
        //FIN flag - connection close request
        if (hdr->flags & TCP_FLAG_FIN)
            tcps_rx_fin(tcpips, io, handle);
        //normal processing
        else
            tcps_rx_process(tcpips, io, handle);
    }
    //SYN flag - new connection request
    else if (hdr->flags & TCP_FLAG_SYN)
    {
        handle = tcps_find_listen(tcpips, dst_port);
        if (handle != INVALID_HANDLE)
            tcps_rx_syn(tcpips, io, src, handle);
        //no listening on port, inform by ICMP
        else
#if (ICMP)
            icmps_tx_error(tcpips, io, ICMP_ERROR_PORT, 0);
#else
            ips_release_io(tcpips, io);
#endif //ICMP
    }
    else
    {
        //TODO: half opened connection, send RST
    }
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
