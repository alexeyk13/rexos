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

#define TCP_MSS_MAX                                      (IP_FRAME_MAX_DATA_SIZE - sizeof(TCP_HEADER))
#define TCP_MSS_MIN                                      536

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

typedef struct {
    uint8_t kind;
    uint8_t len;
    uint8_t data[65495];
} TCP_OPT;
#pragma pack(pop)

typedef struct {
    HANDLE process;
    uint16_t port;
} TCP_LISTEN_HANDLE;

typedef enum {
    TCP_STATE_CLOSED = 0,
    TCP_STATE_LISTEN,
    TCP_STATE_SYN_SENT,
    TCP_STATE_SYN_RECEIVED,
    TCP_STATE_ESTABLISHED,
    TCP_STATE_FIN_WAIT_1,
    TCP_STATE_FIN_WAIT_2,
    TCP_STATE_CLOSE_WAIT,
    TCP_STATE_CLOSING,
    TCP_STATE_LAST_ACK,
    TCP_STATE_TIME_WAIT,
    TCP_STATE_MAX
} TCP_STATE;

typedef struct {
    HANDLE process;
    IP remote_addr;
    IO* head;
    uint32_t snd_una, snd_nxt, rcv_nxt, ttl;
    TCP_STATE state;
    uint16_t remote_port, local_port, mss, rx_wnd, tx_wnd;
    //TODO: icmp?
} TCP_TCB;

#if (TCP_DEBUG_FLOW)
static const char* __TCP_FLAGS[TCP_FLAGS_COUNT] =                   {"FIN", "SYN", "RST", "PSH", "ACK", "URG"};
static const char* __TCP_STATES[TCP_STATE_MAX] =                    {"CLOSED", "LISTEN", "SYN SENT", "SYN RECEIVED", "ESTABLISHED", "FIN WAIT1",
                                                                     "FIN WAIT2", "CLOSE WAIT", "CLOSING", "LAST ACK", "TIME_WAIT"};
#endif //TCP_DEBUG_FLOW

static inline unsigned int tcps_get_data_offset(IO* io)
{
    TCP_HEADER* hdr = io_data(io);
    return (hdr->data_off >> 4) << 2;
}

static unsigned int tcps_get_first_opt(IO* io)
{
    TCP_OPT* opt;
    if (tcps_get_data_offset(io) <= sizeof(TCP_HEADER))
        return 0;
    opt = (TCP_OPT*)((uint8_t*)io_data(io) + sizeof(TCP_HEADER));
    return (opt->kind == TCP_OPTS_END) ? 0 : sizeof(TCP_HEADER);
}

static unsigned int tcps_get_next_opt(IO* io, unsigned int prev)
{
    TCP_OPT* opt;
    unsigned int res;
    unsigned int offset = tcps_get_data_offset(io);
    opt = (TCP_OPT*)((uint8_t*)io_data(io) + prev);
    switch(opt->kind)
    {
    case TCP_OPTS_END:
        //end of list
        return 0;
    case TCP_OPTS_NOOP:
        res = prev + 1;
        break;
    default:
        res = prev + opt->len;
    }
    return res < offset ? res : 0;
}

#if (TCP_DEBUG_FLOW)
static void tcps_debug(IO* io, const IP* src, const IP* dst)
{
    bool has_flag;
    int i, j;
    TCP_HEADER* hdr = io_data(io);
    TCP_OPT* opt;
    printf("TCP: ");
    ip_print(src);
    printf(":%d -> ", be2short(hdr->src_port_be));
    ip_print(dst);
    printf(":%d <SEQ=%u>", be2short(hdr->dst_port_be), be2int(hdr->seq_be));
    if (hdr->flags & TCP_FLAG_ACK)
        printf("<ACK=%u>", be2int(hdr->ack_be));
    printf("<WND=%d>", be2short(hdr->window_be));
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
    if (io->data_size - tcps_get_data_offset(io))
        printf("<DATA=%d byte(s)>", io->data_size - tcps_get_data_offset(io));
    if ((i = tcps_get_first_opt(io)) != 0)
    {
        has_flag = false;
        printf("<OPTS=");
        for (; i; i = tcps_get_next_opt(io, i))
        {
            if (has_flag)
                printf(",");
            opt = (TCP_OPT*)((uint8_t*)io_data(io) + i);
            switch(opt->kind)
            {
            case TCP_OPTS_NOOP:
                printf("NOOP");
                break;
            case TCP_OPTS_MSS:
                printf("MSS:%d", be2short(opt->data));
                break;
            default:
                printf("K%d", opt->kind);
                for (j = 0; j < opt->len - 2; ++j)
                {
                    printf(j ? " " : ":");
                    printf("%02X", opt->data[j]);
                }
            }
            has_flag = true;
        }
        printf(">");
    }
    printf("\n");
}

static void tcps_debug_state(TCP_STATE from, TCP_STATE to)
{
    printf("%s -> %s\n", __TCP_STATES[from], __TCP_STATES[to]);
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

static HANDLE tcps_find_listener(TCPIPS* tcpips, uint16_t port)
{
    HANDLE handle;
    TCP_LISTEN_HANDLE* tlh;
    for (handle = so_first(&tcpips->tcps.listen); handle != INVALID_HANDLE; handle = so_next(&tcpips->tcps.listen, handle))
    {
        tlh = so_get(&tcpips->tcps.listen, handle);
        if (tlh->port == port)
            return tlh->process;
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
    tcb->mss = TCP_MSS_MAX;
    tcb->rx_wnd = TCP_MSS_MAX;
    tcb->tx_wnd = 0;
    return handle;
}

static inline bool tcps_set_mss(TCP_TCB* tcb, uint16_t mss)
{
    if (mss < TCP_MSS_MIN || mss > TCP_MSS_MAX)
        return false;
    tcb->mss = mss;
    return true;
}

static void tcps_apply_options(TCPIPS* tcpips, IO* io, TCP_TCB* tcb)
{
    int i;
    TCP_OPT* opt;
    for (i = tcps_get_first_opt(io); i; i = tcps_get_next_opt(io, i))
    {
        opt = (TCP_OPT*)((uint8_t*)io_data(io) + i);
        switch(opt->kind)
        {
        case TCP_OPTS_MSS:
#if (ICMP)
            if (!tcps_set_mss(tcb, be2short(opt->data)))
                icmps_tx_error(tcpips, io, ICMP_ERROR_PARAMETER, ((IP_STACK*)io_stack(io))->hdr_size + sizeof(TCP_HEADER) + i);
#else
            tcps_set_mss(tcpips, tcb, be2short(opt->data));
#endif //ICMP
            break;
        default:
            break;
        }
    }
}

void tcps_init(TCPIPS* tcpips)
{
    so_create(&tcpips->tcps.listen, sizeof(TCP_LISTEN_HANDLE), 1);
    so_create(&tcpips->tcps.tcbs, sizeof(TCP_TCB), 1);
}

static void tcps_tx(TCPIPS* tcpips, IO* io, TCP_TCB* tcb)
{
    TCP_HEADER* tcp = io_data(io);
    short2be(tcp->src_port_be, tcb->local_port);
    short2be(tcp->dst_port_be, tcb->remote_port);
    int2be(tcp->seq_be, tcb->snd_una);
    int2be(tcp->ack_be, (tcp->flags & TCP_FLAG_ACK) ? tcb->rcv_nxt : 0);
    //not byte aligned
//    uint8_t data_off;
//    uint8_t window_be[2];
    //TODO:
//    uint8_t checksum_be[2];
    //TODO:
//    uint8_t urgent_pointer_be[2];

    ips_tx(tcpips, io, &tcb->remote_addr);
    //TODO:
}

static inline void tcps_rx_listen(TCPIPS* tcpips, IO* io, HANDLE tcb_handle)
{
    TCP_HEADER* hdr;
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    hdr = io_data(io);

    //SYN flag - new connection request
    if (hdr->flags & TCP_FLAG_SYN)
    {
        tcb->state = TCP_STATE_SYN_RECEIVED;
#if (TCP_DEBUG_FLOW)
        tcps_debug_state(TCP_STATE_LISTEN, TCP_STATE_SYN_RECEIVED);
#endif //TCP_DEBUG_FLOW
        tcb->snd_nxt = tcb->snd_una = tcps_gen_isn();
        //SYN needs 1 seq
        tcb->rcv_nxt = be2int(hdr->seq_be) + 1;

        //TODO: tx ack here
    }
    //half opened connection, RST
    else
    {
#if (TCP_DEBUG)
        printf("TCP: Half-opened connection, closing");
#endif //TCP_DEBUG_FLOW
        //TODO: half opened connection, send RST
    }
}

static inline void tcps_rx_process(TCPIPS* tcpips, IO* io, HANDLE tcb_handle)
{
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    switch (tcb->state)
    {
    case TCP_STATE_LISTEN:
        tcps_rx_listen(tcpips, io, tcb_handle);
        break;
    default:
        printf("TODO: unsupported state: %d\n", tcb->state);
        break;
    }
}

void tcps_rx(TCPIPS* tcpips, IO* io, IP* src)
{
    TCP_HEADER* hdr;
    TCP_TCB* tcb;
    HANDLE tcb_handle, process;
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

    tcb_handle = tcps_find_tcb(tcpips, src, src_port, dst_port);
    if (tcb_handle == INVALID_HANDLE)
    {
        process = tcps_find_listener(tcpips, dst_port);
        //found? Create TCB
        if (process != INVALID_HANDLE)
        {
            tcb_handle = tcps_create_tcb(tcpips, src, src_port, dst_port, process);
            tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
            tcb->state = TCP_STATE_LISTEN;
        }
        //no listening on port, inform by ICMP
#if (TCP_DEBUG_FLOW) || (ICMP)
        else
        {
#if (TCP_DEBUG_FLOW)
            printf("TCP: port closed, connection rejected\n");
#endif //TCP_DEBUG_FLOW
#if (ICMP)
            icmps_tx_error(tcpips, io, ICMP_ERROR_PORT, 0);
#endif //ICMP
        }
#endif //(TCP_DEBUG_FLOW) || (ICMP)
    }
    if (tcb_handle != INVALID_HANDLE)
    {
        tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
        tcps_apply_options(tcpips, io, tcb);
        tcb->tx_wnd = be2short(hdr->window_be);
        tcps_rx_process(tcpips, io, tcb_handle);
    }
    ips_release_io(tcpips, io);
}

static inline void tcps_listen(TCPIPS* tcpips, IPC* ipc)
{
    HANDLE handle;
    TCP_LISTEN_HANDLE* tlh;
    if (tcps_find_listener(tcpips, (uint16_t)ipc->param1) != INVALID_HANDLE)
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
