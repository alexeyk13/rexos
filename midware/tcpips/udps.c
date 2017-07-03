/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "udps.h"
#include "tcpips_private.h"
#include "../../userspace/udp.h"
#include "../../userspace/error.h"
#include "../../userspace/stdio.h"
#include "../../userspace/endian.h"
#include <string.h>
#include "sys_config.h"
#include "icmps.h"

#pragma pack(push, 1)
typedef struct {
    uint8_t src_port_be[2];
    uint8_t dst_port_be[2];
    uint8_t len_be[2];
    uint8_t checksum_be[2];
} UDP_HEADER;
#pragma pack(pop)

typedef struct {
    HANDLE process;
    uint16_t remote_port, local_port;
    IP remote_addr;
    IO* head;
#if (ICMP)
    int err;
#endif //ICMP
} UDP_HANDLE;

#define UDP_FRAME_MAX_DATA_SIZE                                 (IP_FRAME_MAX_DATA_SIZE - sizeof(UDP_HEADER))

static HANDLE udps_find(TCPIPS* tcpips, uint16_t local_port)
{
    HANDLE handle;
    UDP_HANDLE* uh;
    for (handle = so_first(&tcpips->udps.handles); handle != INVALID_HANDLE; handle = so_next(&tcpips->udps.handles, handle))
    {
        uh = so_get(&tcpips->udps.handles, handle);
        if (uh->local_port == local_port)
            return handle;
    }
    return INVALID_HANDLE;
}

static inline uint16_t udps_allocate_port(TCPIPS* tcpips)
{
    unsigned int res;
    //from current to HI
    for (res = tcpips->udps.dynamic; res <= TCPIP_DYNAMIC_RANGE_HI; ++res)
    {
        if (udps_find(tcpips, res) == INVALID_HANDLE)
        {
            tcpips->udps.dynamic = res == TCPIP_DYNAMIC_RANGE_HI ? TCPIP_DYNAMIC_RANGE_LO : res + 1;
            return (uint16_t)res;
        }

    }
    //from LO to current
    for (res = TCPIP_DYNAMIC_RANGE_LO; res < tcpips->udps.dynamic; ++res)
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

static IO* udps_peek_head(TCPIPS* tcpips, UDP_HANDLE* uh)
{
    IO* io = uh->head;
    if (io)
        uh->head = *((IO**)io_data(uh->head));
    return io;
}

static void udps_flush(TCPIPS* tcpips, HANDLE handle)
{
    IO* io;
    UDP_HANDLE* uh;
    int err;
    uh = so_get(&tcpips->udps.handles, handle);
    if (uh == NULL)
        return;
    err = ERROR_IO_CANCELLED;
#if (ICMP)
    if (uh->err != ERROR_OK)
        err = uh->err;
#endif //ICMP
    while ((io = udps_peek_head(tcpips, uh)) != NULL)
        io_complete_ex(uh->process, HAL_CMD(HAL_UDP, IPC_READ), handle, io, err);
}

static void udps_send_user(TCPIPS* tcpips, IP* src, IO* io, HANDLE handle)
{
    IO* user_io;
    unsigned int offset, size;
    UDP_STACK* udp_stack;
    UDP_HANDLE* uh;
    UDP_HEADER* hdr = io_data(io);

    uh = so_get(&tcpips->udps.handles, handle);
    for (offset = sizeof(UDP_HEADER); uh->head && offset < io->data_size; offset += size)
    {
        user_io = udps_peek_head(tcpips, uh);
        udp_stack = io_push(user_io, sizeof(UDP_STACK));
        udp_stack->remote_addr.u32.ip = src->u32.ip;
        udp_stack->remote_port = be2short(hdr->src_port_be);

        size = io_get_free(user_io);
        if (size > io->data_size - offset)
            size = io->data_size - offset;
        memcpy(io_data(user_io), (uint8_t*)io_data(io) + offset, size);
        user_io->data_size = size;
        io_complete(uh->process, HAL_IO_CMD(HAL_UDP, IPC_READ), handle, user_io);
    }
#if (UDP_DEBUG)
    if (offset < io->data_size)
        printf("UDP: %d byte(s) dropped\n", io->data_size - offset);
#endif //UDP_DEBUG
}

void udps_init(TCPIPS* tcpips)
{
    so_create(&tcpips->udps.handles, sizeof(UDP_HANDLE), 1);
}

void udps_link_changed(TCPIPS* tcpips, bool link)
{
    HANDLE handle;
    if (link)
        tcpips->udps.dynamic = TCPIP_DYNAMIC_RANGE_LO;
    else
    {
        while ((handle = so_first(&tcpips->udps.handles)) != INVALID_HANDLE)
        {
            udps_flush(tcpips, handle);
            so_free(&tcpips->udps.handles, handle);
        }
    }
}

static void udps_replay(TCPIPS* tcpips, IO* io, const IP* src)
{
    UDP_HEADER* udp;
    IP dst;
    uint16_t src_port;
    dst.u32.ip = src->u32.ip;
    io_unhide(io, sizeof(UDP_HEADER));
    udp = io_data(io);
    //format header
    src_port = be2short(udp->dst_port_be);
    udp->dst_port_be[0] = udp->src_port_be[0];
    udp->dst_port_be[1] = udp->src_port_be[1];
    short2be(udp->src_port_be, src_port);

    short2be(udp->len_be, io->data_size);
    short2be(udp->checksum_be, 0);
    short2be(udp->checksum_be, udp_checksum(io_data(io), io->data_size, &tcpips->ips.ip, &dst));
    ips_tx(tcpips, io, &dst);
}

void udps_rx(TCPIPS* tcpips, IO* io, IP* src)
{
    HANDLE handle;
    UDP_HEADER* hdr;
    UDP_HANDLE* uh;
    uint16_t src_port, dst_port;
#if(UDP_BROADCAST)
    const IP* dst;
    dst = (const IP*)io_data(io) - 1;
    if (io->data_size < sizeof(UDP_HEADER) || udp_checksum(io_data(io), io->data_size, src, dst))
#else
    if (io->data_size < sizeof(UDP_HEADER) || udp_checksum(io_data(io), io->data_size, src, &tcpips->ips.ip))
#endif
    {
        ips_release_io(tcpips, io);
        return;
    }
    hdr = io_data(io);
    src_port = be2short(hdr->src_port_be);
    dst_port = be2short(hdr->dst_port_be);
#if (UDP_DEBUG_FLOW)
    printf("UDP: ");
    ip_print(src);
    printf(":%d -> ", src_port);
    ip_print(&tcpips->ips.ip);
    printf(":%d, %d byte(s)\n", dst_port, io->data_size - sizeof(UDP_HEADER));
#endif //UDP_DEBUG_FLOW

#if(DHCPS)
    if((dst_port == DHCP_SERVER_PORT)||(src_port == DHCP_CLIENT_PORT))
    {
        io_hide(io, sizeof(UDP_HEADER));
        if(dhcps_rx(tcpips,io,src))
            udps_replay(tcpips,io,&__BROADCAST);
        else
            ips_release_io(tcpips, io);
        return;
    }
#endif

#if(DNSS)
    if((dst_port == DNS_PORT)&&(dst->u32.ip == tcpips->ips.ip.u32.ip))
    {
        io_hide(io, sizeof(UDP_HEADER));
        if(dnss_rx(tcpips,io,src))
            udps_replay(tcpips,io,src);
        else
            ips_release_io(tcpips, io);
        return;
    }
#endif
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
#if(UDP_BROADCAST)
    if ((handle == INVALID_HANDLE) && (dst->u32.ip != BROADCAST))
#else
    if (handle == INVALID_HANDLE)
#endif
    {
#if (UDP_DEBUG)
        printf("UDP: no connection, datagramm dropped\n");
#endif //UDP_DEBUG
#if (ICMP)
        icmps_tx_error(tcpips, io, ICMP_ERROR_PORT, 0);
#endif //ICMP
    }
    ips_release_io(tcpips, io);
}

static inline void udps_listen(TCPIPS* tcpips, IPC* ipc)
{
    HANDLE handle;
    UDP_HANDLE* uh;
    if (udps_find(tcpips, (uint16_t)ipc->param1) != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    handle = so_allocate(&tcpips->udps.handles);
    if (handle == INVALID_HANDLE)
        return;
    uh = so_get(&tcpips->udps.handles, handle);
    uh->remote_port = 0;
    uh->local_port = (uint16_t)ipc->param1;
    uh->remote_addr.u32.ip = __LOCALHOST.u32.ip;
    uh->process = ipc->process;
    uh->head = NULL;
#if (ICMP)
    uh->err = ERROR_OK;
#endif //ICMP

    ipc->param2 = handle;
}

static inline void udps_connect(TCPIPS* tcpips, IPC* ipc)
{
    HANDLE handle;
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
    ipc->param2 = handle;
}

static inline void udps_close(TCPIPS* tcpips, HANDLE handle)
{
    UDP_HANDLE* uh;
    if ((uh = so_get(&tcpips->udps.handles, handle)) == NULL)
        return;
    udps_flush(tcpips, handle);
    so_free(&tcpips->udps.handles, handle);
}

static inline void udps_read(TCPIPS* tcpips, HANDLE handle, IO* io)
{
    IO* cur;
    UDP_HANDLE* uh;
    uh = so_get(&tcpips->udps.handles, handle);
    if (uh == NULL)
        return;
#if (ICMP)
    if (uh->err != ERROR_OK)
    {
        error(uh->err);
        return;
    }
#endif //ICMP
    io->data_size = 0;
    *((IO**)io_data(io)) = NULL;
    //add to head
    if (uh->head == NULL)
        uh->head = io;
    //add to end
    else
    {
        for (cur = uh->head; *((IO**)io_data(cur)) != NULL; cur = *((IO**)io_data(cur))) {}
        *((IO**)io_data(cur)) = io;
    }
    error(ERROR_SYNC);
}

static inline void udps_write(TCPIPS* tcpips, HANDLE handle, IO* io)
{
    IO* cur;
    unsigned int offset, size;
    unsigned short remote_port;
    IP dst;
    UDP_STACK* udp_stack;
    UDP_HEADER* udp;
    UDP_HANDLE* uh = so_get(&tcpips->udps.handles, handle);
    if (uh == NULL)
        return;
#if (ICMP)
    if (uh->err != ERROR_OK)
    {
        error(uh->err);
        return;
    }
#endif //ICMP
    //listening socket
    if (uh->remote_port == 0)
    {
        if (io->stack_size < sizeof(UDP_STACK))
        {
            error(ERROR_INVALID_PARAMS);
            return;
        }
        udp_stack = io_stack(io);
        remote_port = udp_stack->remote_port;
        dst.u32.ip = udp_stack->remote_addr.u32.ip;
        io_pop(io, sizeof(UDP_STACK));
    }
    else
    {
        remote_port = uh->remote_port;
        dst.u32.ip = uh->remote_addr.u32.ip;
    }

    for (offset = 0; offset < io->data_size; offset += size)
    {
        size = UDP_FRAME_MAX_DATA_SIZE;
        if (size > io->data_size - offset)
            size = io->data_size - offset;
        cur = ips_allocate_io(tcpips, size + sizeof(UDP_HEADER), PROTO_UDP);
        if (cur == NULL)
            return;
        //copy data
        memcpy((uint8_t*)io_data(cur) + sizeof(UDP_HEADER), (uint8_t*)io_data(io) + offset, size);
        udp = io_data(cur);
// correct size
        cur->data_size = size + sizeof(UDP_HEADER);
        //format header
        short2be(udp->src_port_be, uh->local_port);
        short2be(udp->dst_port_be, remote_port);
        short2be(udp->len_be, size + sizeof(UDP_HEADER));
        short2be(udp->checksum_be, 0);
        short2be(udp->checksum_be, udp_checksum(io_data(cur), cur->data_size, &tcpips->ips.ip, &dst));
        ips_tx(tcpips, cur, &dst);
    }
}

void udps_request(TCPIPS* tcpips, IPC* ipc)
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
            udps_listen(tcpips, ipc);
        else
            udps_connect(tcpips, ipc);
        break;
    case IPC_CLOSE:
        udps_close(tcpips, ipc->param1);
        break;
    case IPC_READ:
        udps_read(tcpips, ipc->param1, (IO*)ipc->param2);
        break;
    case IPC_WRITE:
        udps_write(tcpips, ipc->param1, (IO*)ipc->param2);
        break;
    case IPC_FLUSH:
        udps_flush(tcpips, ipc->param1);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

#if (ICMP)
void udps_icmps_error_process(TCPIPS* tcpips, IO* io, ICMP_ERROR code, const IP* src)
{
    HANDLE handle;
    UDP_HANDLE* uh;
    uint16_t src_port, dst_port;
    UDP_HEADER* hdr = io_data(io);
    src_port = be2short(hdr->src_port_be);
    dst_port = be2short(hdr->dst_port_be);

    if ((handle = udps_find(tcpips, src_port)) == INVALID_HANDLE)
        return;
    uh = so_get(&tcpips->udps.handles, handle);
    //ignore errors on listening sockets
    if (uh->remote_port == dst_port && uh->remote_addr.u32.ip == src->u32.ip)
    {
        switch (code)
        {
        case ICMP_ERROR_NETWORK:
        case ICMP_ERROR_HOST:
        case ICMP_ERROR_PROTOCOL:
        case ICMP_ERROR_PORT:
        case ICMP_ERROR_FRAGMENTATION:
        case ICMP_ERROR_ROUTE:
            uh->err = ERROR_NOT_RESPONDING;
            break;
        case ICMP_ERROR_TTL_EXCEED:
        case ICMP_ERROR_FRAGMENT_REASSEMBLY_EXCEED:
            uh->err = ERROR_TIMEOUT;
            break;
        case ICMP_ERROR_PARAMETER:
            uh->err = ERROR_INVALID_PARAMS;
            break;
        default:
            break;
        }
        udps_flush(tcpips, handle);
    }
}
#endif //ICMP
