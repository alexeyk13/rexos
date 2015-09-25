/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "icmps.h"
#include "tcpips_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/ip.h"
#include "../../userspace/endian.h"
#include "../../userspace/icmp.h"
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
    ipc_post_inline(tcpips->icmps.process, HAL_CMD(HAL_ICMP, ICMP_PING), tcpips->icmps.echo_ip.u32.ip, 0, err);
    printd("ICMP: %d\n", err);
    tcpips->icmps.process = INVALID_HANDLE;
}

static void icmps_error(TCPIPS* tcpips, IO* io, ICMP_ERROR code, unsigned int offset)
{
    //TODO: major refactoring required
    IP_HEADER* hdr;
    io->data_offset += sizeof(ICMP_HEADER);
    io->data_size -= sizeof(ICMP_HEADER);
    hdr = io_data(io);
    //TODO: check proto,
    //if icmp and request is echo, fail echo

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
    printf("ICMP: DESTINATION UNREACHABLE(%d) ", icmp->code);
#endif
    icmps_error(tcpips, io, icmp->code, 0);
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
    case  ICMP_CMD_DESTINATION_UNREACHABLE:
        icmps_rx_destination_unreachable(tcpips, io);
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

static inline bool icmps_ping(TCPIPS* tcpips, const IP* dst, HANDLE process)
{
    if (tcpips->icmps.process != INVALID_HANDLE)
    {
        error(ERROR_IN_PROGRESS);
        return true;
    }
    if (tcpips->icmps.echo_ip.u32.ip != dst->u32.ip)
    {
        tcpips->icmps.echo_ip.u32.ip = dst->u32.ip;
        ++tcpips->icmps.id;
        tcpips->icmps.seq = 0;
    }
    tcpips->icmps.process = process;
    icmps_tx_echo(tcpips);
    return false;
}

bool icmps_request(TCPIPS* tcpips, IPC* ipc)
{
    bool need_post = false;
    IP ip;
    switch (HAL_ITEM(ipc->cmd))
    {
    case ICMP_PING:
        ip.u32.ip = ipc->param1;
        need_post = icmps_ping(tcpips, &ip, ipc->process);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
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

void icmps_destination_unreachable(TCPIPS* tcpips, uint8_t code, IO* original, const IP *dst)
{
#if (ICMP_DEBUG)
    printf("ICMP: Destination unreachable(%d) to ", code);
    ip_print(dst);
    printf("\n");
#endif
    icmps_control_prepare(tcpips, ICMP_CMD_DESTINATION_UNREACHABLE, code, original);
    icmps_tx(tcpips, original, dst);
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
