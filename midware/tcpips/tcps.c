/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "tcps.h"
#include "tcpips_private.h"
#include "../../userspace/tcp.h"
#include "../../userspace/stdio.h"
#include "../../userspace/endian.h"
#include "../../userspace/systime.h"
#include "../../userspace/error.h"
#include "icmps.h"
#include <string.h>

#define TCP_MSS_MAX                                      (IP_FRAME_MAX_DATA_SIZE - sizeof(TCP_HEADER))
#define TCP_MSS_MIN                                      536

#define MSL_MS                                           60000

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
    TCP_STATE_CLOSING,
    TCP_STATE_LAST_ACK,
    TCP_STATE_MAX
} TCP_STATE;

typedef struct {
    HANDLE process;
    IP remote_addr;
    IO* rx;
    IO* rx_tmp;
    IO* tx;
    HANDLE timer;
    unsigned int tx_cur, rx_cur;
    uint32_t snd_una, snd_nxt, rcv_nxt;

    TCP_STATE state;
    uint16_t remote_port, local_port, mss, rx_wnd, tx_wnd, retry;
    bool active, transmit, fin;
} TCP_TCB;

#if (TCP_DEBUG_PACKETS)
static const char* __TCP_FLAGS[TCP_FLAGS_COUNT] =                   {"FIN", "SYN", "RST", "PSH", "ACK", "URG"};
#endif //TCP_DEBUG_PACKETS
#if (TCP_DEBUG_FLOW)
static const char* __TCP_STATES[TCP_STATE_MAX] =                    {"CLOSED", "LISTEN", "SYN SENT", "SYN RECEIVED", "ESTABLISHED", "FIN WAIT1",
                                                                     "FIN WAIT2", "CLOSING", "LAST ACK"};
#endif //TCP_DEBUG_FLOW

static inline unsigned int tcps_data_offset(IO* io)
{
    TCP_HEADER* tcp = io_data(io);
    return (tcp->data_off >> 4) << 2;
}

static inline unsigned int tcps_data_len(IO* io)
{
    unsigned int res = tcps_data_offset(io);
    return io->data_size > res ? io->data_size - res : 0;
}

static inline unsigned int tcps_seg_len(IO* io)
{
    unsigned int res;
    TCP_HEADER* tcp = io_data(io);
    res = tcps_data_len(io);
    if (tcp->flags & TCP_FLAG_SYN)
        ++res;
    if (tcp->flags & TCP_FLAG_FIN)
        ++res;
    return res;
}

static uint32_t tcps_delta(uint32_t from, uint32_t to)
{
    if (to >= from)
        return to - from;
    else
        return (0xffffffff - from) + to + 1;
}

static int tcps_diff(uint32_t from, uint32_t to)
{
    uint32_t res = tcps_delta(from, to);
    if (res <= 0xffff)
        return (int)res;
    res = tcps_delta(to, from);
    if (res <= 0xffff)
        return -(int)res;
    return 0x10000;
}

static unsigned int tcps_get_first_opt(IO* io)
{
    TCP_OPT* opt;
    if (tcps_data_offset(io) <= sizeof(TCP_HEADER))
        return 0;
    opt = (TCP_OPT*)((uint8_t*)io_data(io) + sizeof(TCP_HEADER));
    return (opt->kind == TCP_OPTS_END) ? 0 : sizeof(TCP_HEADER);
}

static unsigned int tcps_get_next_opt(IO* io, unsigned int prev)
{
    TCP_OPT* opt;
    unsigned int res;
    unsigned int offset = tcps_data_offset(io);
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

static void tcps_append_opt(IO* io, uint8_t kind, uint8_t* data, uint8_t size)
{
    unsigned int opts_offset, data_offset;
    TCP_OPT* opt;
    TCP_HEADER* tcp = io_data(io);
    data_offset = tcps_data_offset(io);
    for (opts_offset = sizeof(TCP_HEADER); opts_offset < data_offset; )
    {
        opt = (TCP_OPT*)((uint8_t*)io_data(io) + opts_offset);
        if (opt->kind == TCP_OPTS_END)
            break;
        else if (opt->kind == TCP_OPTS_NOOP)
            ++opts_offset;
        else
            opts_offset += opt->len;
    }
    //append offset by size
    data_offset = (opts_offset + size + 3) & ~3;
    tcp->data_off = (data_offset >> 2) << 4;
    memset((uint8_t*)io_data(io) + opts_offset + size, TCP_OPTS_END, data_offset - opts_offset - size);
    io->data_size = data_offset;
    opt = (TCP_OPT*)((uint8_t*)io_data(io) + opts_offset);
    opt->kind = kind;
    switch(opt->kind)
    {
    case TCP_OPTS_END:
    case TCP_OPTS_NOOP:
        break;
    default:
        opt->len = size;
        memcpy(opt->data, data, size - 2);
    }
}

static void tcps_append_mss(IO* io)
{
    uint8_t mss_be[2];
    short2be(mss_be, TCP_MSS_MAX);
    tcps_append_opt(io, TCP_OPTS_MSS, mss_be, 2 + 2);
}

#if (TCP_DEBUG_PACKETS)
static void tcps_debug(IO* io, const IP* src, const IP* dst)
{
    bool has_flag;
    int i, j;
    TCP_HEADER* tcp = io_data(io);
    TCP_OPT* opt;
    printf("TCP: ");
    ip_print(src);
    printf(":%d -> ", be2short(tcp->src_port_be));
    ip_print(dst);
    printf(":%d <SEQ=%u>", be2short(tcp->dst_port_be), be2int(tcp->seq_be));
    if (tcp->flags & TCP_FLAG_ACK)
        printf("<ACK=%u>", be2int(tcp->ack_be));
    printf("<WND=%d>", be2short(tcp->window_be));
    if (tcp->flags & TCP_FLAG_MSK)
    {
        printf("<CTL=");
        has_flag = false;
        for (i = 0; i < TCP_FLAGS_COUNT; ++i)
            if (tcp->flags & (1 << i))
            {
                if (has_flag)
                    printf(",");
                printf(__TCP_FLAGS[i]);
                has_flag = true;
            }
        printf(">");
    }
    if (tcps_data_len(io))
        printf("<DATA=%d byte(s)>", tcps_data_len(io));
    if ((i = tcps_get_first_opt(io)) != 0)
    {
        tcps_append_opt(io, 0, NULL, 0);
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
#endif //TCP_DEBUG_PACKETS

#if (TCP_DEBUG_FLOW)
static void tcps_debug_state(TCP_STATE from, TCP_STATE to)
{
    printf("%s -> %s\n", __TCP_STATES[from], __TCP_STATES[to]);
}
#endif //TCP_DEBUG_FLOW

static inline void tcps_set_state(TCP_TCB* tcb, TCP_STATE state)
{
#if (TCP_DEBUG_FLOW)
    tcps_debug_state(tcb->state, state);
#endif //TCP_DEBUG_FLOW
    tcb->state = state;
    tcb->retry = 0;
}

static uint32_t tcps_gen_isn()
{
    SYSTIME uptime;
    get_uptime(&uptime);
    //increment every 4us
    return (uptime.sec % 17179) + (uptime.usec >> 2);
}

static bool tcps_update_rx_wnd(TCP_TCB* tcb)
{
    bool need_update = tcb->rx_wnd < (TCP_MSS_MAX / 2);
    tcb->rx_wnd = TCP_MSS_MAX;
    if (tcb->rx != NULL)
        tcb->rx_wnd += io_get_free(tcb->rx);
    if (tcb->rx_tmp != NULL)
        tcb->rx_wnd = io_get_free(tcb->rx_tmp);
    return need_update;
}

static void tcps_timer_start(TCP_TCB* tcb)
{
    switch (tcb->state)
    {
    case TCP_STATE_ESTABLISHED:
#if !(TCP_KEEP_ALIVE)
        if (!tcb->transmit)
            break;
#endif //!TCP_KEEP_ALIVE
    default:
        timer_start_ms(tcb->timer, TCP_TIMEOUT);
    }
}

static void tcps_rx_flush(TCPIPS* tcpips, HANDLE tcb_handle)
{
    TCP_STACK* tcp_stack;
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    if (tcb->rx)
    {
        if (tcb->rx->data_size)
        {
            tcp_stack = io_stack(tcb->rx);
            tcp_stack->flags |= TCP_PSH;
            io_complete(tcb->process, HAL_IO_CMD(HAL_TCP, IPC_READ), tcb_handle, tcb->rx);
        }
        else
            io_complete_ex(tcb->process, HAL_IO_CMD(HAL_TCP, IPC_READ), tcb_handle, tcb->rx, ERROR_CONNECTION_CLOSED);
        tcb->rx = NULL;
    }
    if (tcb->rx_tmp)
    {
        ips_release_io(tcpips, tcb->rx_tmp);
        tcb->rx_tmp = NULL;
    }
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

static HANDLE tcps_find_tcb_local_port(TCPIPS* tcpips, uint16_t local_port)
{
    HANDLE handle;
    TCP_TCB* tcb;
    for (handle = so_first(&tcpips->tcps.tcbs); handle != INVALID_HANDLE; handle = so_next(&tcpips->tcps.tcbs, handle))
    {
        tcb = so_get(&tcpips->tcps.tcbs, handle);
        if (tcb->local_port == local_port)
            return handle;
    }
    return INVALID_HANDLE;
}

static HANDLE tcps_create_tcb_internal(TCPIPS* tcpips, const IP* remote_addr, uint16_t remote_port, uint16_t local_port)
{
    TCP_TCB* tcb;
    HANDLE handle;
    if (so_count(&tcpips->tcps.tcbs) > TCP_HANDLES_LIMIT)
    {
        error(ERROR_TOO_MANY_HANDLES);
#if (TCP_DEBUG)
        printf("TCP: Too many handles\n");
#endif //TCP_DEBUG
        return INVALID_HANDLE;
    }
    handle = so_allocate(&tcpips->tcps.tcbs);
    if (handle == INVALID_HANDLE)
        return handle;
    tcb = so_get(&tcpips->tcps.tcbs, handle);
    tcb->timer = timer_create(handle, HAL_TCP);
    if (tcb->timer == INVALID_HANDLE)
    {
        so_free(&tcpips->tcps.tcbs, handle);
        return INVALID_HANDLE;
    }
    tcb->retry = 0;
    tcb->process = INVALID_HANDLE;
    tcb->remote_addr.u32.ip = remote_addr->u32.ip;
    tcb->snd_una = tcb->snd_nxt = 0;
    tcb->rcv_nxt = 0;
    tcb->state = TCP_STATE_CLOSED;
    tcb->remote_port = remote_port;
    tcb->local_port = local_port;
    tcb->mss = TCP_MSS_MAX;
    tcb->active = false;
    tcb->transmit = false;
    tcb->fin = false;
    tcb->rx = tcb->tx = tcb->rx_tmp = NULL;
    tcb->tx_cur = 0;
    tcps_update_rx_wnd(tcb);
    tcb->tx_wnd = 0;
    return handle;
}

static void tcps_destroy_tcb(TCPIPS* tcpips, HANDLE tcb_handle)
{
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
#if (TCP_DEBUG_FLOW)
    printf("%s -> 0\n", __TCP_STATES[tcb->state]);
#endif //TCP_DEBUG_FLOW
    timer_destroy(tcb->timer);
    tcps_rx_flush(tcpips, tcb_handle);
    if (tcb->tx)
        io_complete_ex(tcb->process, HAL_IO_CMD(HAL_TCP, IPC_WRITE), tcb_handle, tcb->tx, ERROR_CONNECTION_CLOSED);
    so_free(&tcpips->tcps.tcbs, tcb_handle);
}

static void tcps_close_connection(TCPIPS* tcpips, HANDLE tcb_handle, unsigned int error)
{
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);

    switch (tcb->state)
    {
    case TCP_STATE_SYN_RECEIVED:
        if (tcb->active)
            ipc_post_inline(tcb->process, HAL_CMD(HAL_TCP, IPC_OPEN), tcb_handle, INVALID_HANDLE, error);
        break;
    case TCP_STATE_ESTABLISHED:
    case TCP_STATE_FIN_WAIT_1:
    case TCP_STATE_FIN_WAIT_2:
        //inform user on connection closed\n"
        ipc_post_inline(tcb->process, HAL_CMD(HAL_TCP, IPC_CLOSE), tcb_handle, 0, error);
        break;
    default:
        break;
    }
    //enter closed state, destroy TCB and return
    tcps_destroy_tcb(tcpips, tcb_handle);
}

static inline uint16_t tcps_allocate_port(TCPIPS* tcpips)
{
    unsigned int res;
    //from current to HI
    for (res = tcpips->tcps.dynamic; res <= TCPIP_DYNAMIC_RANGE_HI; ++res)
    {
        if (tcps_find_tcb_local_port(tcpips, res) == INVALID_HANDLE)
        {
            tcpips->tcps.dynamic = res == TCPIP_DYNAMIC_RANGE_HI ? TCPIP_DYNAMIC_RANGE_LO : res + 1;
            return (uint16_t)res;
        }

    }
    //from LO to current
    for (res = TCPIP_DYNAMIC_RANGE_LO; res < tcpips->tcps.dynamic; ++res)
    {
        if (tcps_find_tcb_local_port(tcpips, res) == INVALID_HANDLE)
        {
            tcpips->tcps.dynamic = res + 1;
            return res;
        }

    }
    error(ERROR_TOO_MANY_HANDLES);
    return 0;
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

static IO* tcps_allocate_io(TCPIPS* tcpips, TCP_TCB* tcb)
{
    TCP_HEADER* tcp;
    IO* io = ips_allocate_io(tcpips, IP_FRAME_MAX_DATA_SIZE, PROTO_TCP);
    if (io == NULL)
        return NULL;
    tcp = io_data(io);
    short2be(tcp->src_port_be, tcb->local_port);
    short2be(tcp->dst_port_be, tcb->remote_port);
    int2be(tcp->seq_be, 0);
    int2be(tcp->ack_be, 0);
    tcp->data_off = (sizeof(TCP_HEADER) >> 2) << 4;
    tcp->flags = 0;
    short2be(tcp->urgent_pointer_be, 0);
    short2be(tcp->checksum_be, 0);
    io->data_size = sizeof(TCP_HEADER);
    return io;
}

static void tcps_tx(TCPIPS* tcpips, IO* io, TCP_TCB* tcb)
{
    TCP_HEADER* tcp = io_data(io);
    short2be(tcp->window_be, tcb->rx_wnd);
    short2be(tcp->checksum_be, tcp_checksum(io_data(io), io->data_size, &tcpips->ips.ip, &tcb->remote_addr));
#if (TCP_DEBUG_PACKETS)
    tcps_debug(io, &tcpips->ips.ip, &tcb->remote_addr);
#endif //TCP_DEBUG_PACKETS
    ips_tx(tcpips, io, &tcb->remote_addr);
}

static void tcps_tx_rst(TCPIPS* tcpips, HANDLE tcb_handle, uint32_t seq)
{
    IO* tx;
    TCP_HEADER* tcp_tx;
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);

    if ((tx = tcps_allocate_io(tcpips, tcb)) == NULL)
        return;

    tcp_tx = io_data(tx);
    tcp_tx->flags |= TCP_FLAG_RST;
    int2be(tcp_tx->seq_be, seq);
    tcps_tx(tcpips, tx, tcb);
}

static void tcps_tx_rst_ack(TCPIPS* tcpips, HANDLE tcb_handle, uint32_t ack)
{
    IO* tx;
    TCP_HEADER* tcp_tx;
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);

    if ((tx = tcps_allocate_io(tcpips, tcb)) == NULL)
        return;

    tcp_tx = io_data(tx);
    tcp_tx->flags |= TCP_FLAG_RST | TCP_FLAG_ACK;
    int2be(tcp_tx->seq_be, 0);
    int2be(tcp_tx->ack_be, ack);
    tcps_tx(tcpips, tx, tcb);
}

static void tcps_tx_ack(TCPIPS* tcpips, HANDLE tcb_handle)
{
    IO* tx;
    TCP_HEADER* tcp_tx;
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);

    if ((tx = tcps_allocate_io(tcpips, tcb)) == NULL)
        return;

    tcp_tx = io_data(tx);

    tcp_tx->flags |= TCP_FLAG_ACK;
    int2be(tcp_tx->seq_be, tcb->snd_nxt);
    int2be(tcp_tx->ack_be, tcb->rcv_nxt);
    tcps_tx(tcpips, tx, tcb);
    tcps_timer_start(tcb);
}

static void tcps_tx_text_ack_fin(TCPIPS* tcpips, HANDLE tcb_handle)
{
    IO* io;
    TCP_HEADER* tcp;
    TCP_STACK* tcp_stack;
    unsigned int size;
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);

    if ((io = tcps_allocate_io(tcpips, tcb)) == NULL)
        return;

    tcp = io_data(io);
    tcp->flags |= TCP_FLAG_ACK;
    int2be(tcp->seq_be, tcb->snd_una);
    int2be(tcp->ack_be, tcb->rcv_nxt);
    //if no transmit window, request window update, wait timeout, than try again
    if (tcb->tx_wnd)
    {
        if (tcb->tx != NULL)
        {
            size = tcb->tx->data_size - tcb->tx_cur;
            if (size > tcb->tx_wnd)
                size = tcb->tx_wnd;
            memcpy((uint8_t*)io_data(io) + io->data_size, (uint8_t*)io_data(tcb->tx) + tcb->tx_cur, size);
            io->data_size += size;
            //apply flags
            tcp_stack = io_stack(tcb->tx);
            if ((tcp_stack->flags & TCP_PSH) && (tcb->tx_cur + size >= tcb->tx->data_size))
                tcp->flags |= TCP_FLAG_PSH;
            if ((tcp_stack->flags & TCP_URG) && (tcp_stack->urg_len > tcb->tx_cur))
            {
                tcp->flags |= TCP_FLAG_URG;
                short2be(tcp->urgent_pointer_be, tcp_stack->urg_len - tcb->tx_cur);
            }
        }
        else
            size = 0;
        if (tcb->fin && (tcb->snd_una != tcb->snd_nxt) && (size < tcb->tx_wnd))
            tcp->flags |= TCP_FLAG_FIN;
        tcps_tx(tcpips, io, tcb);
    }
    tcps_timer_start(tcb);
}

static void tcps_tx_syn(TCPIPS* tcpips, HANDLE tcb_handle)
{
    IO* io;
    TCP_HEADER* tcp;
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);

    if ((io = tcps_allocate_io(tcpips, tcb)) == NULL)
        return;

    tcp = io_data(io);

    //SYN flag
    tcp->flags |= TCP_FLAG_SYN;
    tcps_append_mss(io);

    int2be(tcp->seq_be, tcb->snd_una);
    tcps_tx(tcpips, io, tcb);
    tcps_timer_start(tcb);
}

static void tcps_tx_syn_ack(TCPIPS* tcpips, HANDLE tcb_handle)
{
    IO* io;
    TCP_HEADER* tcp;
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);

    if ((io = tcps_allocate_io(tcpips, tcb)) == NULL)
        return;

    tcp = io_data(io);

    //add ACK, SYN flags
    tcp->flags |= TCP_FLAG_ACK | TCP_FLAG_SYN;
    tcps_append_mss(io);

    int2be(tcp->seq_be, tcb->snd_una);
    int2be(tcp->ack_be, tcb->rcv_nxt);
    tcps_tx(tcpips, io, tcb);
    tcps_timer_start(tcb);
}

static inline bool tcps_rx_otw_check_seq(TCPIPS* tcpips, IO* io, HANDLE tcb_handle)
{
    int seq_delta, seg_len;
    unsigned int data_off;
    uint32_t seq;
    TCP_HEADER* tcp;
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    tcp = io_data(io);

    seq = be2int(tcp->seq_be);
    seq_delta = tcps_diff(tcb->rcv_nxt, seq);
    seg_len = tcps_seg_len(io);
    //already paritally received segment
    if (seq_delta < 0)
    {
        if (seg_len + seq_delta <= 0)
        {
#if (TCP_DEBUG_FLOW)
            printf("TCP: Dup\n");
#endif //TCP_DEBUG_FLOW
            if (tcb->state == TCP_STATE_SYN_RECEIVED)
                tcps_tx_syn_ack(tcpips, tcb_handle);
            else
                tcps_tx_ack(tcpips, tcb_handle);
            return false;
        }
#if (TCP_DEBUG_FLOW)
        printf("TCP: partial receive %d seq\n", seg_len + seq_delta);
#endif //TCP_DEBUG_FLOW
        //SYN flag space goes first, remove from sequence
        if (tcp->flags & TCP_FLAG_SYN)
        {
            tcp->flags &= ~TCP_FLAG_SYN;
            --seg_len;
            ++seq_delta;
            ++seq;
        }
        data_off = tcps_data_offset(io);
        seg_len -= -seq_delta;
        //FIN is not in data, but occupying virtual byte
        if (tcp->flags & TCP_FLAG_FIN)
        {
            memmove((uint8_t*)io_data(io) + data_off, (uint8_t*)io_data(io) + data_off + (-seq_delta), seg_len - 1);
            io->data_size -= (-seq_delta) - 1;
        }
        else
        {
            memmove((uint8_t*)io_data(io) + data_off, (uint8_t*)io_data(io) + data_off + (-seq_delta), seg_len);
            io->data_size -= -seq_delta;
        }
        seq += -seq_delta;
        seq_delta = 0;
    }
    //don't fit in rx window
    if (seg_len > tcb->rx_wnd && tcb->rx_wnd > 0)
    {
#if (TCP_DEBUG_FLOW)
        printf("TCP: chop rx wnd %d seq\n", seg_len - tcb->rx_wnd);
#endif //TCP_DEBUG_FLOW
        //FIN is last virtual byte, remove it first
        if (tcp->flags & TCP_FLAG_FIN)
        {
            tcp->flags &= ~TCP_FLAG_FIN;
            --seg_len;
        }
        //still don't fit? remove some data
        if (seg_len > tcb->rx_wnd)
        {
            io->data_size = seg_len - tcb->rx_wnd;
            seg_len = tcb->rx_wnd;
        }
        //remove PSH flag, cause it's goes after all bytes
        tcp->flags &= ~TCP_FLAG_PSH;
    }
    if (seq != tcb->rcv_nxt || seg_len > tcb->rx_wnd)
    {
#if (TCP_DEBUG_FLOW)
        printf("TCP: Future sequence/don't fit\n");
#endif //TCP_DEBUG_FLOW
        //RST bit is set, drop the segment and return:
        if (tcp->flags & TCP_FLAG_RST)
        {
            tcps_timer_start(tcb);
            return false;
        }

        tcps_tx_ack(tcpips, tcb_handle);
        return false;
    }
    return true;
}

static inline bool tcps_rx_otw_ack(TCPIPS* tcpips, IO* io, HANDLE tcb_handle)
{
    int snd_diff, ack_diff;
    TCP_HEADER* tcp;
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    tcp = io_data(io);
    snd_diff = tcps_diff(tcb->snd_una, tcb->snd_nxt);
    ack_diff = tcps_diff(tcb->snd_una, be2int(tcp->ack_be));

    if (tcb->state == TCP_STATE_SYN_RECEIVED)
    {
        //SND.UNA =< SEG.ACK =< SND.NXT
        if (ack_diff >= 0 && ack_diff <= snd_diff)
        {
            tcps_set_state(tcb, TCP_STATE_ESTABLISHED);
            ipc_post_inline(tcb->process, HAL_CMD(HAL_TCP, IPC_OPEN), tcb_handle, tcb_handle, 0);
            //and continue processing in that state if no data
            if (tcps_seg_len(io) == 0)
                return false;
        }
        else
        {
            //form a reset segment
            tcps_tx_rst(tcpips, tcb_handle, be2int(tcp->ack_be));
            tcps_timer_start(tcb);
            return false;
        }
    }
    //SEG.ACK > SND.NXT
    if (ack_diff > snd_diff)
    {
#if (TCP_DEBUG_FLOW)
        printf("TCP: SEG.ACK > SND.NEXT. Keep-alive?\n");
#endif //TCP_DEBUG_FLOW
        tcps_tx_ack(tcpips, tcb_handle);
        return false;
    }

    if ((ack_diff > 0) || ((ack_diff == 0) && (tcb->snd_nxt == tcb->snd_una)))
        tcb->retry = 0;
    //adjust ack
    if (ack_diff > 0)
    {
        tcb->snd_una += ack_diff;
        if (tcb->tx != NULL)
        {
            if (ack_diff >= tcb->tx->data_size - tcb->tx_cur)
            {
                //return sent buffers to user
                io_pop(tcb->tx, sizeof(TCP_STACK));
                io_complete(tcb->process, HAL_IO_CMD(HAL_TCP, IPC_WRITE), tcb_handle, tcb->tx);
                tcb->tx = NULL;
                tcb->tx_cur = 0;
            }
            else
                tcb->tx_cur += ack_diff;
        }
    }

    switch (tcb->state)
    {
    case TCP_STATE_FIN_WAIT_1:
        if (tcb->snd_nxt == tcb->snd_una)
        {
            tcps_set_state(tcb, TCP_STATE_FIN_WAIT_2);
            //In addition to the processing for the ESTABLISHED state, if the retransmission queue is empty, the user’s CLOSE can be acknowledged
            ipc_post_inline(tcb->process, HAL_CMD(HAL_TCP, IPC_CLOSE), tcb_handle, 0, 0);
        }
        break;
    case TCP_STATE_CLOSING:
    case TCP_STATE_LAST_ACK:
        if (tcb->snd_nxt == tcb->snd_una)
        {
            tcps_destroy_tcb(tcpips, tcb_handle);
            return false;
        }
        break;
    default:
        break;
    }
    return true;
}

static void tcps_rx_text(TCPIPS* tcpips, IO* io, HANDLE tcb_handle)
{
    TCP_STACK* tcp_stack;
    TCP_HEADER* tcp;
    TCP_HEADER* tcp_tmp;
    unsigned int data_size, data_offset, size, data_offset_tmp;
    uint16_t urg, urg_tmp;
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    tcp = io_data(io);

    switch (tcb->state)
    {
    case TCP_STATE_ESTABLISHED:
    case TCP_STATE_FIN_WAIT_1:
    case TCP_STATE_FIN_WAIT_2:
        tcb->rx_cur = data_size = tcps_data_len(io);
        if (data_size)
        {
            data_offset = tcps_data_offset(io);
            urg = 0;
            if (tcp->flags & TCP_FLAG_URG)
            {
                urg = be2int(tcp->urgent_pointer_be);
                //make sure urg don't overlaps total data size
                if (urg > data_size)
                {
                    urg = data_size;
                    short2be(tcp->urgent_pointer_be, urg);
                }
            }
            tcb->rcv_nxt += data_size;
            tcb->retry = 0;
            //has user block
            if (tcb->rx != NULL)
            {
                size = data_size;
                if (size > io_get_free(tcb->rx))
                    size = io_get_free(tcb->rx);
                memcpy((uint8_t*)io_data(tcb->rx) + tcb->rx->data_size, (uint8_t*)io_data(io) + data_offset, size);
                tcb->rx->data_size += size;
                data_offset += size;
                data_size -= size;

                //apply flags, urgent data
                tcp_stack = (TCP_STACK*)io_stack(tcb->rx);
                if (tcp->flags & TCP_FLAG_PSH)
                    tcp_stack->flags |= TCP_PSH;
                if (urg)
                {
                    tcp_stack->flags |= TCP_URG;
                    tcp_stack->urg_len = urg;
                    if (urg > size)
                        urg -= size;
                    else
                        urg = 0;
                }

                //filled, send to user
                if ((io_get_free(tcb->rx) == 0) || (tcp->flags & TCP_FLAG_PSH))
                {
                    io_complete(tcb->process, HAL_IO_CMD(HAL_TCP, IPC_READ), tcb_handle, tcb->rx);
                    tcb->rx = NULL;
                }

            }
            //no user block/no fit
            if (data_size)
            {
                //move to tmp
                if (tcb->rx_tmp == NULL)
                    tcb->rx_tmp = io;
                //append to tmp
                else
                {
                    tcp_tmp = io_data(tcb->rx_tmp);
                    //apply flags, urgent data
                    if (tcp->flags & TCP_FLAG_PSH)
                        tcp_tmp->flags |= TCP_FLAG_PSH;
                    if (urg)
                    {
                        data_offset_tmp = tcps_data_offset(tcb->rx_tmp);
                        urg_tmp = 0;
                        //already have urgent pointer? add to end of urgent data
                        if (tcp_tmp->flags & TCP_FLAG_URG)
                            urg_tmp = be2short(tcp_tmp->urgent_pointer_be);
                        memmove((uint8_t*)io_data(tcb->rx_tmp) + data_offset_tmp + urg_tmp + urg,
                                (uint8_t*)io_data(tcb->rx_tmp) + data_offset_tmp + urg_tmp,
                                tcb->rx_tmp->data_size - data_offset - urg_tmp);
                        memcpy((uint8_t*)io_data(tcb->rx_tmp) + data_offset_tmp + urg_tmp, (uint8_t*)io_data(io) + data_offset, urg);
                        tcb->rx_tmp->data_size += urg;
                        data_offset += urg;
                        data_size -= urg;
                        tcp_tmp->flags |= TCP_FLAG_URG;
                        short2be(tcp_tmp->urgent_pointer_be, urg_tmp + urg);
                    }
                    memcpy((uint8_t*)io_data(tcb->rx_tmp) + tcb->rx_tmp->data_size, (uint8_t*)io_data(io) + data_offset, data_size);
                    tcb->rx_tmp->data_size += data_size;
                }
            }
            tcps_update_rx_wnd(tcb);
        }
        break;
    default:
        //Ignore the segment text
        break;
    }
}

static inline bool tcps_rx_otw_fin(TCPIPS* tcpips, HANDLE tcb_handle)
{
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);

    //ack FIN
    ++tcb->rcv_nxt;
    if (!tcb->fin)
    {
        tcb->fin = true;
        ++tcb->snd_nxt;
    }
    switch (tcb->state)
    {
    case TCP_STATE_ESTABLISHED:
        //return all rx buffers
        tcps_rx_flush(tcpips, tcb_handle);
        //inform user
        ipc_post_inline(tcb->process, HAL_CMD(HAL_TCP, IPC_CLOSE), tcb_handle, 0, 0);
        //follow down
    case TCP_STATE_SYN_RECEIVED:
        tcps_set_state(tcb, TCP_STATE_LAST_ACK);
        break;
    case TCP_STATE_FIN_WAIT_1:
        tcps_set_state(tcb, TCP_STATE_CLOSING);
        ipc_post_inline(tcb->process, HAL_CMD(HAL_TCP, IPC_CLOSE), tcb_handle, 0, 0);
        break;
    case TCP_STATE_FIN_WAIT_2:
        tcps_destroy_tcb(tcpips, tcb_handle);
        return false;
    default:
        break;
    }
    return true;
}

static inline void tcps_rx_send(TCPIPS* tcpips, HANDLE tcb_handle)
{
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);

    //ack from remote host - we transmitted all
    if (tcb->state == TCP_STATE_ESTABLISHED && tcb->transmit && (tcb->snd_una == tcb->snd_nxt) && !tcb->fin && tcb->rx_cur == 0)
    {
        tcb->transmit = false;
#if (TCP_KEEP_ALIVE)
        tcps_timer_start(tcb);
#endif //TCP_KEEP_ALIVE
        return;
    }
    tcps_tx_text_ack_fin(tcpips, tcb_handle);
}

static inline void tcps_rx_closed(TCPIPS* tcpips, IO* io, HANDLE tcb_handle)
{
    TCP_HEADER* tcp;
    tcp = io_data(io);

    do {
        //An incoming segment containing a RST is discarded
        if (tcp->flags & TCP_FLAG_RST)
            break;

        //An incoming segment not containing a RST causes a RST to be sent in response
        if (tcp->flags & TCP_FLAG_ACK)
            tcps_tx_rst(tcpips, tcb_handle, be2int(tcp->ack_be));
        else
            tcps_tx_rst_ack(tcpips, tcb_handle, be2int(tcp->seq_be) + tcps_seg_len(io));
    } while (false);
    tcps_destroy_tcb(tcpips, tcb_handle);
}

static inline void tcps_rx_listen(TCPIPS* tcpips, IO* io, HANDLE tcb_handle)
{
    TCP_HEADER* tcp;
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    tcp = io_data(io);

    do {
        //An incoming RST should be ignored
        if (tcp->flags & TCP_FLAG_RST)
            break;

        //An acceptable reset segment should be formed for any arriving ACK-bearing segment
        if (tcp->flags & TCP_FLAG_ACK)
        {
            tcps_tx_rst(tcpips, tcb_handle, be2int(tcp->ack_be));
            break;
        }

        if (tcp->flags & TCP_FLAG_SYN)
        {
            tcps_set_state(tcb, TCP_STATE_SYN_RECEIVED);
            tcb->rcv_nxt = be2int(tcp->seq_be) + 1;
            tcb->snd_una = tcb->snd_nxt = tcps_gen_isn();
            ++tcb->snd_nxt;

            tcps_tx_syn_ack(tcpips, tcb_handle);
            return;
        }

        //You are unlikely to get here, but if you do, drop the segment, and return
    } while (false);
    tcps_destroy_tcb(tcpips, tcb_handle);
}

static inline void tcps_rx_syn_sent(TCPIPS* tcpips, IO* io, HANDLE tcb_handle)
{
    int ack_diff;
    TCP_HEADER* tcp;
    uint32_t ack;
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    tcp = io_data(io);

    ack_diff = 0;
    //first check the ACK bit
    if (tcp->flags & TCP_FLAG_ACK)
    {
        ack = be2int(tcp->ack_be);
        ack_diff = tcps_diff(tcb->snd_una, ack);
        if (ack_diff < 0)
        {
            if ((tcp->flags & TCP_FLAG_RST) == 0)
                tcps_tx_rst(tcpips, tcb_handle, ack);
            tcps_timer_start(tcb);
            return;
        }
    }

    //second check the RST bit
    if (tcp->flags & TCP_FLAG_RST)
    {
        if (tcp->flags & TCP_FLAG_ACK)
        {
            //inform user connected refused
            ipc_post_inline(tcb->process, HAL_CMD(HAL_TCP, IPC_OPEN), tcb_handle, INVALID_HANDLE, ERROR_CONNECTION_REFUSED);
            tcps_destroy_tcb(tcpips, tcb_handle);
        }
        else
            tcps_timer_start(tcb);
        return;
    }

    //fourth check the SYN bit
    if (tcp->flags & TCP_FLAG_SYN)
    {
        if (ack_diff)
        {
            ++tcb->rcv_nxt;
            tcps_set_state(tcb, TCP_STATE_ESTABLISHED);
            //inform user connected successfully
            ipc_post_inline(tcb->process, HAL_CMD(HAL_TCP, IPC_OPEN), tcb_handle, tcb_handle, 0);
            tcps_rx_text(tcpips, io, tcb_handle);
        }
        tcps_rx_send(tcpips, tcb_handle);
        return;
    }
}

static inline void tcps_rx_otw(TCPIPS* tcpips, IO* io, HANDLE tcb_handle)
{
    TCP_HEADER* tcp = io_data(io);

    //first check sequence number
    if (!tcps_rx_otw_check_seq(tcpips, io, tcb_handle))
        return;

    //second check the RST bit
    //fourth, check the SYN bit
    if (tcp->flags & (TCP_FLAG_RST | TCP_FLAG_SYN))
    {
        tcps_close_connection(tcpips, tcb_handle, ERROR_CONNECTION_REFUSED);
        return;
    }
    //fifth check the ACK field
    if (tcp->flags & TCP_FLAG_ACK)
    {
        if (!tcps_rx_otw_ack(tcpips, io, tcb_handle))
            return;
    }
    else
    {
        tcps_timer_start(so_get(&tcpips->tcps.tcbs, tcb_handle));
        return;
    }

    //sixth, check the URG bit
    //seventh, process the segment text
    tcps_rx_text(tcpips, io, tcb_handle);

    //eighth, check the FIN bit
    if (tcp->flags & TCP_FLAG_FIN)
    {
        if (!tcps_rx_otw_fin(tcpips, tcb_handle))
            return;
    }

    //finally send ACK reply/data/fin/etc
    tcps_rx_send(tcpips, tcb_handle);
}

static inline void tcps_rx_process(TCPIPS* tcpips, IO* io, HANDLE tcb_handle)
{
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    switch (tcb->state)
    {
    case TCP_STATE_CLOSED:
        tcps_rx_closed(tcpips, io, tcb_handle);
        break;
    case TCP_STATE_LISTEN:
        tcps_rx_listen(tcpips, io, tcb_handle);
        break;
    case TCP_STATE_SYN_SENT:
        tcps_rx_syn_sent(tcpips, io, tcb_handle);
        break;
    default:
        tcps_rx_otw(tcpips, io, tcb_handle);
        break;
    }
}

void tcps_init(TCPIPS* tcpips)
{
    so_create(&tcpips->tcps.listen, sizeof(TCP_LISTEN_HANDLE), 1);
    so_create(&tcpips->tcps.tcbs, sizeof(TCP_TCB), 1);
}

void tcps_link_changed(TCPIPS* tcpips, bool link)
{
    HANDLE handle;
    //nothing to do if link, close all connections if not
    if (!link)
    {
        while((handle = so_first(&tcpips->tcps.tcbs)) != INVALID_HANDLE)
            tcps_close_connection(tcpips, handle, ERROR_CONNECTION_CLOSED);
        while((handle = so_first(&tcpips->tcps.listen)) != INVALID_HANDLE)
            so_free(&tcpips->tcps.listen, handle);
    }
}

void tcps_rx(TCPIPS* tcpips, IO* io, IP* src)
{
    TCP_HEADER* tcp;
    TCP_TCB* tcb;
    HANDLE tcb_handle, process;
    uint16_t src_port, dst_port;
    if (io->data_size < sizeof(TCP_HEADER) || tcp_checksum(io_data(io), io->data_size, src, &tcpips->ips.ip))
    {
        ips_release_io(tcpips, io);
        return;
    }
    tcp = io_data(io);
    src_port = be2short(tcp->src_port_be);
    dst_port = be2short(tcp->dst_port_be);
#if (TCP_DEBUG_PACKETS)
    tcps_debug(io, src, &tcpips->ips.ip);
#endif //TCP_DEBUG_PACKETS

    if ((tcb_handle = tcps_find_tcb(tcpips, src, src_port, dst_port)) == INVALID_HANDLE)
    {
        if ((tcb_handle = tcps_create_tcb_internal(tcpips, src, src_port, dst_port)) != INVALID_HANDLE)
        {
            //listening?
            if ((process = tcps_find_listener(tcpips, dst_port)) != INVALID_HANDLE)
            {
                tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
                tcb->state = TCP_STATE_LISTEN;
                tcb->active = false;
                tcb->process = process;
            }
        }
    }
    if (tcb_handle != INVALID_HANDLE)
    {
        tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
        timer_stop(tcb->timer, tcb_handle, HAL_TCP);
        tcps_apply_options(tcpips, io, tcb);
        tcb->tx_wnd = be2short(tcp->window_be);
        tcb->rx_cur = 0;
        tcps_rx_process(tcpips, io, tcb_handle);
        //make sure not queued in rx
        if (tcb->rx_tmp == io)
            return;
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

static inline void tcps_close_listen(TCPIPS* tcpips, HANDLE handle)
{
    so_free(&tcpips->tcps.listen, handle);
}

static HANDLE tcps_create_tcb(TCPIPS* tcpips, uint16_t remote_port, const IP* remote_addr, HANDLE process)
{
    HANDLE tcb_handle;
    TCP_TCB* tcb;
    uint16_t local_port = tcps_allocate_port(tcpips);
    if (local_port == 0)
        return INVALID_HANDLE;
    tcb_handle = tcps_create_tcb_internal(tcpips, remote_addr, remote_port, local_port);
    if (tcb_handle == INVALID_HANDLE)
        return INVALID_HANDLE;
    tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    tcb->process = process;
    return tcb_handle;
}

static inline void tcps_get_remote_addr(TCPIPS* tcpips, HANDLE tcb_handle, IP* ip)
{
    ip->u32.ip = 0;
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    if (tcb == NULL)
        return;
    ip->u32.ip = tcb->remote_addr.u32.ip;
}

static inline uint16_t tcps_get_remote_port(TCPIPS* tcpips, HANDLE tcb_handle)
{
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    if (tcb == NULL)
        return 0;
    return tcb->remote_port;
}

static inline uint16_t tcps_get_local_port(TCPIPS* tcpips, HANDLE tcb_handle)
{
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    if (tcb == NULL)
        return 0;
    return tcb->local_port;
}

static inline void tcps_open(TCPIPS* tcpips, HANDLE tcb_handle)
{
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    if (tcb == NULL)
        return;
    if (tcb->state != TCP_STATE_CLOSED)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
    tcps_set_state(tcb, TCP_STATE_SYN_SENT);
    tcb->snd_una = tcb->snd_nxt = tcps_gen_isn();
    ++tcb->snd_nxt;
    tcps_tx_syn(tcpips, tcb_handle);
    error(ERROR_SYNC);
}

static inline void tcps_close(TCPIPS* tcpips, HANDLE tcb_handle)
{
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    if (tcb == NULL)
        return;
    switch (tcb->state)
    {
    case TCP_STATE_ESTABLISHED:
        tcps_set_state(tcb, TCP_STATE_FIN_WAIT_1);
        tcb->fin = true;
        ++tcb->snd_nxt;
        tcps_rx_flush(tcpips, tcb_handle);
        tcps_tx_text_ack_fin(tcpips, tcb_handle);
        error(ERROR_SYNC);
        break;
    case TCP_STATE_LAST_ACK:
        //already sent with same handle
        error(ERROR_SYNC);
        break;
    default:
        error(ERROR_INVALID_STATE);
    }
}

static inline void tcps_read(TCPIPS* tcpips, HANDLE tcb_handle, IO* io)
{
    TCP_TCB* tcb;
    TCP_STACK* tcp_stack;
    TCP_HEADER* tcp;
    uint16_t urg;
    tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    unsigned int size, data_size, data_offset;
    if (tcb == NULL)
        return;
    if (tcb->rx != NULL)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    switch (tcb->state)
    {
    case TCP_STATE_ESTABLISHED:
    case TCP_STATE_FIN_WAIT_1:
    case TCP_STATE_FIN_WAIT_2:
        timer_stop(tcb->timer, tcb_handle, HAL_TCP);
        io->data_size = 0;
        tcp_stack = io_push(io, sizeof(TCP_STACK));
        tcp_stack->flags = 0;
        tcp_stack->urg_len = 0;
        size = io_get_free(io);
        //already on tmp buffers
        if (tcb->rx_tmp != NULL)
        {
            data_offset = tcps_data_offset(tcb->rx_tmp);
            data_size = tcps_data_len(tcb->rx_tmp);
            if (size > data_size)
                size = data_size;
            memcpy(io_data(io), (uint8_t*)io_data(tcb->rx_tmp) + data_offset, size);
            io->data_size = size;
            //apply flags
            tcp = io_data(tcb->rx_tmp);
            if (tcp->flags & TCP_FLAG_PSH)
                tcp_stack->flags |= TCP_PSH;
            if (tcp->flags & TCP_FLAG_URG)
            {
                urg = be2int(tcp->urgent_pointer_be);
                //make sure urg don't overlaps total data size
                if (urg > size)
                {
                    tcp_stack->urg_len = size;
                    short2be(tcp->urgent_pointer_be, urg - size);
                }
                else
                {
                    //all URG data processed
                    tcp_stack->urg_len = urg;
                    if (size < data_size)
                    {
                        short2be(tcp->urgent_pointer_be, 0);
                        tcp->flags &= ~TCP_FLAG_URG;
                    }
                }
                tcp_stack->flags |= TCP_URG;
            }
            //all copied?
            if (size == data_size)
            {
                ips_release_io(tcpips, tcb->rx_tmp);
                tcb->rx_tmp = NULL;
            }
            else
            {
                memmove((uint8_t*)io_data(tcb->rx_tmp) + data_offset, (uint8_t*)io_data(tcb->rx_tmp) + data_offset + size, data_size - size);
                tcb->rx_tmp->data_size -= size;
            }
            //can return to user?
            if ((io_get_free(io) == 0) || (tcp_stack->flags & TCP_PSH))
            {
                io_complete(tcb->process, HAL_IO_CMD(HAL_TCP, IPC_READ), tcb_handle, io);
                if (tcps_update_rx_wnd(tcb))
                    tcps_tx_ack(tcpips, tcb_handle);
                error(ERROR_SYNC);
                return;
            }
        }
        tcb->rx = io;
        if (tcps_update_rx_wnd(tcb))
            tcps_tx_ack(tcpips, tcb_handle);
        error(ERROR_SYNC);
        break;
    default:
        error(ERROR_INVALID_STATE);
    }
}

static inline void tcps_write(TCPIPS* tcpips, HANDLE tcb_handle, IO* io)
{
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    if (tcb == NULL)
        return;
    if (tcb->state != TCP_STATE_ESTABLISHED)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
    if (tcb->tx != NULL)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    timer_stop(tcb->timer, tcb_handle, HAL_TCP);

    tcb->tx = io;
    tcb->snd_nxt += io->data_size;
    tcb->transmit = true;
    tcps_tx_text_ack_fin(tcpips, tcb_handle);
    error(ERROR_SYNC);
}

static inline void tcps_flush(TCPIPS* tcpips, HANDLE tcb_handle)
{
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    if (tcb == NULL)
        return;
    tcps_rx_flush(tcpips, tcb_handle);
}

static inline void tcps_timeout(TCPIPS* tcpips, HANDLE tcb_handle)
{
    TCP_TCB* tcb = so_get(&tcpips->tcps.tcbs, tcb_handle);
    if (tcb == NULL)
        return;
#if (TCP_KEEP_ALIVE)
    //keep-alive, not error condition
    if ((tcb->state == TCP_STATE_ESTABLISHED) && (tcb->transmit == false))
    {
#if (TCP_DEBUG_FLOW)
        printf("TCP: Keep-alive to ");
        ip_print(&tcb->remote_addr);
        printf(":%u\n", tcb->remote_port);
#endif //TCP_DEBUG_FLOW
        tcb->transmit = true;
        tcps_tx_text_ack_fin(tcpips, tcb_handle);
        return;
    }
#endif //TCP_KEEP_ALIVE

#if (TCP_DEBUG_FLOW)
    printf("TCP: ");
    ip_print(&tcb->remote_addr);
    printf(":%u retry\n", tcb->remote_port);
#endif //TCP_DEBUG_FLOW
    if (++tcb->retry > TCP_RETRY_COUNT)
    {
#if (TCP_DEBUG_FLOW)
        printf("TCP: Retry exceed, closing connection\n");
#endif //TCP_DEBUG_FLOW
        tcps_close_connection(tcpips, tcb_handle, ERROR_TIMEOUT);
        return;
    }

    switch (tcb->state)
    {
    case TCP_STATE_SYN_SENT:
        tcps_tx_syn(tcpips, tcb_handle);
        break;
    default:
        tcps_tx_text_ack_fin(tcpips, tcb_handle);
        break;
    }
}

void tcps_request(TCPIPS* tcpips, IPC* ipc)
{
    IP ip;
    if (!tcpips->connected)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    case TCP_LISTEN:
        tcps_listen(tcpips, ipc);
        break;
    case TCP_CLOSE_LISTEN:
        tcps_close_listen(tcpips, (HANDLE)ipc->param1);
        break;
    case TCP_CREATE_TCB:
        ip.u32.ip = ipc->param2;
        ipc->param2 = tcps_create_tcb(tcpips, ipc->param1, &ip, ipc->process);
        break;
    case TCP_GET_REMOTE_ADDR:
        tcps_get_remote_addr(tcpips, (HANDLE)ipc->param1, &ip);
        ipc->param2 = ip.u32.ip;
        break;
    case TCP_GET_REMOTE_PORT:
        ipc->param2 = tcps_get_remote_port(tcpips, (HANDLE)ipc->param1);
        break;
    case TCP_GET_LOCAL_PORT:
        ipc->param2 = tcps_get_local_port(tcpips, (HANDLE)ipc->param1);
        break;
    case IPC_OPEN:
        tcps_open(tcpips, (HANDLE)ipc->param1);
        break;
    case IPC_CLOSE:
        tcps_close(tcpips, (HANDLE)ipc->param1);
        break;
    case IPC_READ:
        tcps_read(tcpips, (HANDLE)ipc->param1, (IO*)ipc->param2);
        break;
    case IPC_WRITE:
        tcps_write(tcpips, (HANDLE)ipc->param1, (IO*)ipc->param2);
        break;
    case IPC_FLUSH:
        tcps_flush(tcpips, (HANDLE)ipc->param1);
        break;
    case IPC_TIMEOUT:
        tcps_timeout(tcpips, (HANDLE)ipc->param1);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

#if (ICMP)
void tcps_icmps_error_process(TCPIPS* tcpips, IO* io, ICMP_ERROR code, const IP* src)
{
    HANDLE tcb_handle;
    TCP_HEADER* tcp;
    tcp = io_data(io);

    if ((tcb_handle = tcps_find_tcb(tcpips, src, be2short(tcp->dst_port_be), be2short(tcp->src_port_be))) == INVALID_HANDLE)
        return;
    tcps_close_connection(tcpips, tcb_handle, ERROR_NOT_RESPONDING);
}
#endif //ICMP
