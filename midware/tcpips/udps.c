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

typedef struct {
    IP src;
    IP dst;
    uint8_t zero;
    uint8_t proto;
    uint8_t length_be[2];
} UDP_PSEUDO_HEADER;
#pragma pack(pop)

typedef struct {
    HANDLE process;
    uint16_t remote_port, local_port;
    IP remote_addr;
    IO* head;
#if (ICMP)
    ICMP_ERROR err;
#endif //ICMP
} UDP_HANDLE;

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
        //peek from head
        user_io = uh->head;
        uh->head = *((IO**)io_data(uh->head));
        udp_stack = io_push(user_io, sizeof(UDP_STACK));
        udp_stack->remote_addr.u32.ip = src->u32.ip;
        udp_stack->dst_port = be2short(hdr->dst_port_be);
        udp_stack->src_port = be2short(hdr->src_port_be);

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
    tcpips->udps.dynamic = TCPIP_DYNAMIC_RANGE_LO;
}

void udps_rx(TCPIPS* tcpips, IO* io, IP* src)
{
    HANDLE handle;
    UDP_HEADER* hdr;
    UDP_HANDLE* uh;
    uint16_t src_port, dst_port;
    if (io->data_size < sizeof(UDP_HEADER) || udps_checksum(io, src, &tcpips->ip))
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
    ip_print(&tcpips->ip);
    printf(":%d, %d byte(s)\n", dst_port, io->data_size - sizeof(UDP_HEADER));
#endif //UDP_DEBUG_FLOW

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
#if (UDP_DEBUG)
        printf("UDP: no connection, datagramm dropped\n");
#endif //UDP_DEBUG
#if (ICMP)
        icmps_tx_error(tcpips, io, ICMP_ERROR_PORT, 0);
#endif //ICMP
    }
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
    uh->err = ICMP_ERROR_OK;
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
    handle = so_allocate(&tcpips->udps.handles);
    if (handle == INVALID_HANDLE)
        return;
//TODO: real processing
//TODO: #if (ICMP)
}

static inline void udps_read(TCPIPS* tcpips, HANDLE handle, IO* io)
{
    IO* cur;
    UDP_HANDLE* uh;
    uh = so_get(&tcpips->udps.handles, handle);
    if (uh == NULL)
        return;
    //TODO: #if (ICMP)
    io->data_size = 0;
    *((IO**)io_data(io)) = NULL;
    error(ERROR_SYNC);
    //add to head
    if (uh->head == NULL)
        uh->head = io;
    //add to end
    else
    {
        for (cur = uh->head; *((IO**)io_data(cur)) != NULL; cur = *((IO**)io_data(cur))) {}
        *((IO**)io_data(cur)) = io;
    }
}

void udps_request(TCPIPS* tcpips, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        if (ipc->param2 == LOCALHOST)
            udps_listen(tcpips, ipc);
        else
            udps_connect(tcpips, ipc);
        break;
    case IPC_CLOSE:
        //TODO:
        error(ERROR_NOT_SUPPORTED);
        break;
    case IPC_READ:
        udps_read(tcpips, ipc->param1, (IO*)ipc->param2);
        break;
    case IPC_WRITE:
        //TODO:
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

#if (ICMP)
void udps_icmps_error_process(TCPIPS* tcpips, IO* io, ICMP_ERROR code, const IP* src)
{
    //TODO:
}
#endif //ICMP
