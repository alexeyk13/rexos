/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "icmps.h"
#include "tcpips_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/ip.h"
#include "../../userspace/endian.h"
#include "../../userspace/icmp.h"
#include "../../userspace/error.h"
#include <string.h>

#define ICMP_TYPE(buf)                                              ((buf)[0])
#define ICMP_CODE(buf)                                              ((buf)[1])
#define ICMP_ID(buf)                                                (((buf)[4] << 8) | ((buf)[5]))
#define ICMP_SEQ(buf)                                               (((buf)[6] << 8) | ((buf)[7]))

//64 - ip header - mac header - 2 inverted sequence number?!
#define ICMP_DATA_MAGIC_SIZE                                        22
static const uint8_t __ICMP_DATA_MAGIC[ICMP_DATA_MAGIC_SIZE] =      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22};

void icmps_init(TCPIPS* tcpips)
{
    tcpips->icmps.echo_ip.u32.ip = 0;
    tcpips->icmps.process = INVALID_HANDLE;
}

static void icmps_tx(TCPIPS* tcpips, IO* io, const IP* dst)
{
    ICMP_HEADER* icmp = io_data(io);
    short2be(icmp->checksum_be, 0);
    short2be(icmp->checksum_be, ip_checksum(io_data(io), io->data_size));
    ips_tx(tcpips, io, dst);
}

static void icmps_echo_complete(TCPIPS* tcpips, int err)
{
    ipc_post_inline(tcpips->icmps.process, HAL_CMD(HAL_ICMP, ICMP_PING), tcpips->icmps.echo_ip.u32.ip, err == ERROR_OK ? 0 : INVALID_HANDLE, err);
    tcpips->icmps.process = INVALID_HANDLE;
}

static inline void icmps_error_process(TCPIPS* tcpips, ICMP_ERROR code)
{
    //only icmp echo response
    if (tcpips->icmps.process == INVALID_HANDLE)
        return;
    switch (code)
    {
    case ICMP_ERROR_NETWORK:
    case ICMP_ERROR_HOST:
    case ICMP_ERROR_PROTOCOL:
    case ICMP_ERROR_PORT:
    case ICMP_ERROR_ROUTE:
        icmps_echo_complete(tcpips, ERROR_NOT_RESPONDING);
        break;
    case ICMP_ERROR_TTL_EXCEED:
    case ICMP_ERROR_FRAGMENT_REASSEMBLY_EXCEED:
        icmps_echo_complete(tcpips, ERROR_TIMEOUT);
        break;
    default:
        icmps_echo_complete(tcpips, ERROR_CONNECTION_REFUSED);
    }
}

static void icmps_rx_error_process(TCPIPS* tcpips, IO* io, ICMP_ERROR code)
{
    //hide ICMP header for protocol-depending processing
    IP_HEADER* hdr;
    IP dst;
    unsigned int hdr_size;
    hdr = io_data(io);
    hdr_size = (hdr->ver_ihl & 0xf) << 2;
    io->data_offset += hdr_size;
    io->data_size -= hdr_size;
    if (hdr->src.u32.ip == tcpips->ips.ip.u32.ip)
    {
        dst.u32.ip = hdr->dst.u32.ip;
        switch (hdr->proto)
        {
        case PROTO_ICMP:
            icmps_error_process(tcpips, code);
            break;
#if (UDP)
        case PROTO_UDP:
            udps_icmps_error_process(tcpips, io, code, &dst);
            break;
#endif //UDP
        case PROTO_TCP:
            tcps_icmps_error_process(tcpips, io, code, &dst);
        default:
            break;
        }
    }
    //restore IP header
    io->data_offset -= hdr_size;
    io->data_size += hdr_size;
}

static void icmps_rx_error(TCPIPS* tcpips, IO* io, ICMP_ERROR code)
{
    //hide ICMP header for protocol-depending processing
    io->data_offset += sizeof(ICMP_HEADER);
    io->data_size -= sizeof(ICMP_HEADER);
    icmps_rx_error_process(tcpips, io, code);
    //restore ICMP header and release
    io->data_offset -= sizeof(ICMP_HEADER);
    io->data_size += sizeof(ICMP_HEADER);
    ips_release_io(tcpips, io);
}

#if (ICMP_ECHO)
static inline void icmps_rx_echo(TCPIPS* tcpips, IO* io, IP* src)
{
    ICMP_HEADER_ID_SEQ* icmp = io_data(io);
#if (ICMP_DEBUG)
    printf("ICMP: ECHO from ");
    ip_print(src);
    printf("\n");
#endif
    icmp->type = ICMP_CMD_ECHO_REPLY;
    icmps_tx(tcpips, io, src);
}
#endif

static inline void icmps_rx_echo_reply(TCPIPS* tcpips, IO* io, IP* src)
{
    ICMP_HEADER_ID_SEQ* icmp = io_data(io);
#if (ICMP_DEBUG)
    printf("ICMP: ECHO REPLY from ");
    ip_print(src);
    printf("\n");
#endif
    //compare src, sequence, id and data. Maybe asynchronous response from failed request
    if ((tcpips->icmps.echo_ip.u32.ip != src->u32.ip) || (tcpips->icmps.id != be2short(icmp->id_be)) || (tcpips->icmps.seq != be2short(icmp->seq_be)))
    {
        ips_release_io(tcpips, io);
        return;
    }
    icmps_echo_complete(tcpips, (memcmp(((uint8_t*)io_data(io)) + sizeof(ICMP_HEADER), __ICMP_DATA_MAGIC, ICMP_DATA_MAGIC_SIZE)) ? ERROR_CRC : ERROR_OK);
}

static inline void icmps_rx_destination_unreachable(TCPIPS* tcpips, IO* io)
{
    ICMP_HEADER* icmp = io_data(io);
    //useless if no original header provided
    if (io->data_size < sizeof(ICMP_HEADER) + sizeof(IP_HEADER) + 8)
    {
        ips_release_io(tcpips, io);
        return;
    }
#if (ICMP_DEBUG)
    printf("ICMP: Destination unreachable(%d) ", icmp->code);
#endif
    icmps_rx_error(tcpips, io, icmp->code);
}

static inline void icmps_rx_time_exceeded(TCPIPS* tcpips, IO* io)
{
    ICMP_HEADER* icmp = io_data(io);
    //useless if no original header provided
    if (io->data_size < sizeof(ICMP_HEADER) + sizeof(IP_HEADER) + 8)
    {
        ips_release_io(tcpips, io);
        return;
    }
#if (ICMP_DEBUG)
    printf("ICMP: Time exceeded in transit (%d) ", icmp->code);
#endif
    icmps_rx_error(tcpips, io, icmp->code + ICMP_ERROR_TTL_EXCEED);
}

static inline void icmps_rx_parameter_problem(TCPIPS* tcpips, IO* io)
{
    ICMP_HEADER_PARAM* icmp = io_data(io);
    //useless if no original header provided
    if (io->data_size < sizeof(ICMP_HEADER_PARAM) + sizeof(IP_HEADER) + 8)
    {
        ips_release_io(tcpips, io);
        return;
    }
#if (ICMP_DEBUG)
    printf("ICMP: Parameter problem (%d) ", icmp->param);
#endif
    icmps_rx_error(tcpips, io, icmp->code + ICMP_ERROR_PARAMETER);
}

static inline void icmps_tx_echo(TCPIPS* tcpips)
{
    ICMP_HEADER_ID_SEQ* icmp;
    IO* io = ips_allocate_io(tcpips, ICMP_DATA_MAGIC_SIZE + sizeof(ICMP_HEADER), PROTO_ICMP);
    if (io == NULL)
        icmps_echo_complete(tcpips, get_last_error());
    icmp = io_data(io);
    icmp->type = ICMP_CMD_ECHO;
    icmp->code = 0;
    short2be(icmp->id_be, tcpips->icmps.id);
    short2be(icmp->seq_be, tcpips->icmps.seq);
    memcpy(((uint8_t*)io_data(io)) + sizeof(ICMP_HEADER), __ICMP_DATA_MAGIC, ICMP_DATA_MAGIC_SIZE);
    tcpips->icmps.ttl = tcpips->seconds + ICMP_ECHO_TIMEOUT;

    io->data_size = ICMP_DATA_MAGIC_SIZE + sizeof(ICMP_HEADER);
    icmps_tx(tcpips, io, &tcpips->icmps.echo_ip);
}

static inline void icmps_tx_destination_unreachable(TCPIPS* tcpips, uint8_t code, IO* original)
{
    ICMP_HEADER* icmp;
    IP_STACK* ip_stack = io_stack(original);
    IP_HEADER* hdr = io_data(original);
#if (ICMP_DEBUG)
    printf("ICMP: Destination unreachable(%d) to ", code);
    ip_print(&hdr->src);
    printf("\n");
#endif
    IO* io = ips_allocate_io(tcpips, sizeof(ICMP_HEADER) + ip_stack->hdr_size + 8, PROTO_ICMP);
    if (io == NULL)
        return;
    icmp = io_data(io);
    icmp->type = ICMP_CMD_DESTINATION_UNREACHABLE;
    icmp->code = code;
    memcpy(((uint8_t*)io_data(io)) + sizeof(ICMP_HEADER), io_data(original), ip_stack->hdr_size + 8);
    io->data_size = ip_stack->hdr_size + 8 + sizeof(ICMP_HEADER);
    icmps_tx(tcpips, io, &hdr->src);
}

static inline void icmps_tx_time_exceed(TCPIPS* tcpips, uint8_t code, IO* original)
{
    ICMP_HEADER* icmp;
    IP_STACK* ip_stack = io_stack(original);
    IP_HEADER* hdr = io_data(original);
#if (ICMP_DEBUG)
    printf("ICMP: Time exceeded(%d) to ", code);
    ip_print(&hdr->src);
    printf("\n");
#endif
    IO* io = ips_allocate_io(tcpips, sizeof(ICMP_HEADER) + ip_stack->hdr_size + 8, PROTO_ICMP);
    if (io == NULL)
        return;
    icmp = io_data(io);
    icmp->type = ICMP_CMD_TIME_EXCEEDED;
    icmp->code = code;
    memcpy(((uint8_t*)io_data(io)) + sizeof(ICMP_HEADER), io_data(original), ip_stack->hdr_size + 8);
    io->data_size = ip_stack->hdr_size + 8 + sizeof(ICMP_HEADER);
    icmps_tx(tcpips, io, &hdr->src);
}

static inline void icmps_tx_parameter_problem(TCPIPS* tcpips, uint8_t offset, IO* original)
{
    ICMP_HEADER_PARAM* icmp;
    IP_STACK* ip_stack = io_stack(original);
    IP_HEADER* hdr = io_data(original);
#if (ICMP_DEBUG)
    printf("ICMP: Parameter problem(%d) to ", offset);
    ip_print(&hdr->src);
    printf("\n");
#endif
    IO* io = ips_allocate_io(tcpips, sizeof(ICMP_HEADER_PARAM) + ip_stack->hdr_size + 8, PROTO_ICMP);
    if (io == NULL)
        return;
    icmp = io_data(io);
    icmp->type = ICMP_CMD_PARAMETER_PROBLEM;
    icmp->code = 0;
    icmp->param = offset;
    memcpy(((uint8_t*)io_data(io)) + sizeof(ICMP_HEADER_PARAM), io_data(original), ip_stack->hdr_size + 8);
    io->data_size = ip_stack->hdr_size + 8 + sizeof(ICMP_HEADER_PARAM);
    icmps_tx(tcpips, io, &hdr->src);
}

void icmps_rx(TCPIPS* tcpips, IO *io, IP* src)
{
    ICMP_HEADER* icmp = io_data(io);
    //drop broken ICMP without control, because ICMP is control protocol itself
    if (io->data_size < sizeof(ICMP_HEADER))
    {
        ips_release_io(tcpips, io);
        return;
    }
    if (ip_checksum(io_data(io), io->data_size))
    {
        ips_release_io(tcpips, io);
        return;
    }

    switch (icmp->type)
    {
    case ICMP_CMD_ECHO_REPLY:
        icmps_rx_echo_reply(tcpips, io, src);
        break;
#if (ICMP_ECHO)
    case ICMP_CMD_ECHO:
        icmps_rx_echo(tcpips, io, src);
        break;
#endif
    case ICMP_CMD_DESTINATION_UNREACHABLE:
        icmps_rx_destination_unreachable(tcpips, io);
        break;
    case ICMP_CMD_TIME_EXCEEDED:
        icmps_rx_time_exceeded(tcpips, io);
        break;
    case ICMP_CMD_PARAMETER_PROBLEM:
        icmps_rx_parameter_problem(tcpips, io);
        break;
    default:
#if (ICMP_DEBUG)
        printf("ICMP: unhandled type %d from ", icmp->type);
        ip_print(src);
        printf("\n");
#endif
        ips_release_io(tcpips, io);
        break;

    }
}

static inline void icmps_ping(TCPIPS* tcpips, const IP* dst, HANDLE process)
{
    if (tcpips->icmps.process != INVALID_HANDLE)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    if (tcpips->icmps.echo_ip.u32.ip != dst->u32.ip)
    {
        tcpips->icmps.echo_ip.u32.ip = dst->u32.ip;
        ++tcpips->icmps.id;
        tcpips->icmps.seq = 0;
    }
    tcpips->icmps.process = process;
    icmps_tx_echo(tcpips);
    error(ERROR_SYNC);
}

void icmps_request(TCPIPS* tcpips, IPC* ipc)
{
    IP ip;
    switch (HAL_ITEM(ipc->cmd))
    {
    case ICMP_PING:
        ip.u32.ip = ipc->param1;
        icmps_ping(tcpips, &ip, ipc->process);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

void icmps_timer(TCPIPS* tcpips, unsigned int seconds)
{
    if (tcpips->icmps.process != INVALID_HANDLE && tcpips->icmps.ttl < seconds)
        icmps_echo_complete(tcpips, ERROR_TIMEOUT);
}

void icmps_link_changed(TCPIPS* tcpips, bool link)
{
    if (link)
    {
        tcpips->icmps.id = tcpips->icmps.seq = 0;
    }
    else
    {
        if (tcpips->icmps.process != INVALID_HANDLE)
            icmps_echo_complete(tcpips, ERROR_NOT_FOUND);
    }
}

static void icmps_control_prepare(TCPIPS* tcpips, uint8_t cmd, uint8_t code, IO* original)
{
    IP_STACK* ip_stack;
    ip_stack = io_stack(original);
    //cmd
    ((uint8_t*)io_data(original))[0] = cmd;
    //code
    ((uint8_t*)io_data(original))[1] = code;
    //generally unused
    *(uint32_t*)(((uint8_t*)io_data(original)) + 4) = 0;
    //64 bits + original IP header
    memmove(((uint8_t*)io_data(original)) + sizeof(ICMP_HEADER), ((uint8_t*)io_data(original)) - ip_stack->hdr_size, ip_stack->hdr_size + 8);
}

void icmps_tx_error(TCPIPS* tcpips, IO* original, ICMP_ERROR err, unsigned int offset)
{
    //unhide ip header
    IP_STACK* ip_stack;
    ip_stack = io_stack(original);
    original->data_offset -= ip_stack->hdr_size;
    original->data_size += ip_stack->hdr_size;
    switch (err)
    {
    case ICMP_ERROR_NETWORK:
    case ICMP_ERROR_HOST:
    case ICMP_ERROR_PROTOCOL:
    case ICMP_ERROR_PORT:
    case ICMP_ERROR_FRAGMENTATION:
    case ICMP_ERROR_ROUTE:
        icmps_tx_destination_unreachable(tcpips, err, original);
        break;
    case ICMP_ERROR_TTL_EXCEED:
    case ICMP_ERROR_FRAGMENT_REASSEMBLY_EXCEED:
        icmps_tx_time_exceed(tcpips, err - ICMP_ERROR_TTL_EXCEED, original);
        break;
    case ICMP_ERROR_PARAMETER:
        icmps_tx_parameter_problem(tcpips, offset, original);
        break;
    case ICMP_ERROR_OK:
        break;
    }

    original->data_offset -= ip_stack->hdr_size;
    original->data_size += ip_stack->hdr_size;
}

void icmps_time_exceeded(TCPIPS* tcpips, uint8_t code, IO* original, const IP* dst)
{
#if (ICMP_DEBUG)
    printf("ICMP: Time exceeded(%d) to ", code);
    ip_print(dst);
    printf("\n");
#endif
    icmps_control_prepare(tcpips, ICMP_CMD_TIME_EXCEEDED, code, original);
    icmps_tx(tcpips, original, dst);
}

void icmps_parameter_problem(TCPIPS* tcpips, uint8_t offset, IO* original, const IP* dst)
{
#if (ICMP_DEBUG)
    printf("ICMP: Parameter problem(%d) to ", offset);
    ip_print(dst);
    printf("\n");
#endif
    icmps_control_prepare(tcpips, ICMP_CMD_PARAMETER_PROBLEM, 0, original);
    ((uint8_t*)io_data(original))[4] = offset;
    icmps_tx(tcpips, original, dst);
}

void icmps_no_route(TCPIPS* tcpips, IO* original)
{
    icmps_rx_error_process(tcpips, original, ICMP_ERROR_HOST);
}
