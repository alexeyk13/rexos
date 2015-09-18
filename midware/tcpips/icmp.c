/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "icmp.h"
#include "tcpips_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/ip.h"
#include <string.h>
#include "arps.h"

#define ICMP_HEADER_SIZE                                            8
#define ICMP_TYPE(buf)                                              ((buf)[0])
#define ICMP_CODE(buf)                                              ((buf)[1])
#define ICMP_ID(buf)                                                (((buf)[4] << 8) | ((buf)[5]))
#define ICMP_SEQ(buf)                                               (((buf)[6] << 8) | ((buf)[7]))

#if (ICMP_ECHO)
//64 - ip header - mac header - 2 inverted sequence number?!
#define ICMP_DATA_MAGIC_SIZE                                        22
static const uint8_t __ICMP_DATA_MAGIC[ICMP_DATA_MAGIC_SIZE] =      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22};
#endif

void icmp_init(TCPIPS* tcpips)
{
    tcpips->icmp.id = 0;
    tcpips->icmp.seq_count = 0;
}

static void icmp_tx(TCPIPS* tcpips, IO* io, const IP* dst)
{
    ((uint8_t*)io_data(io))[2] = ((uint8_t*)io_data(io))[3] = 0;
    uint16_t cs = ip_checksum(((uint8_t*)io_data(io)), io->data_size);
    ((uint8_t*)io_data(io))[2] = (cs >> 8) & 0xff;
    ((uint8_t*)io_data(io))[3] = cs & 0xff;
    ips_tx(tcpips, io, dst);
}

#if (ICMP_ECHO_REPLY)
static inline void icmp_cmd_echo(TCPIPS* tcpips, IO* io, IP* src)
{
#if (ICMP_DEBUG)
    printf("ICMP: ECHO from ");
    ip_print(src);
    printf("\n");
#endif
    ((uint8_t*)io_data(io))[0] = ICMP_CMD_ECHO_REPLY;
    icmp_tx(tcpips, io, src);
}
#endif

#if (ICMP_ECHO)
static bool icmp_cmd_echo_request(TCPIPS* tcpips)
{
    IO* io = ips_allocate_io(tcpips, ICMP_DATA_MAGIC_SIZE + ICMP_HEADER_SIZE, PROTO_ICMP);
    if (io == NULL)
        return false;
    io->data_size = ICMP_DATA_MAGIC_SIZE + ICMP_HEADER_SIZE;
    //type
    ((uint8_t*)io_data(io))[0] = ICMP_CMD_ECHO;
    //code
    ((uint8_t*)io_data(io))[1] = 0;
    //id
    ((uint8_t*)io_data(io))[4] = (tcpips->icmp.id >> 8) & 0xff;
    ((uint8_t*)io_data(io))[5] = tcpips->icmp.id & 0xff;
    //sequence
    ((uint8_t*)io_data(io))[6] = (tcpips->icmp.seq >> 8) & 0xff;
    ((uint8_t*)io_data(io))[7] = tcpips->icmp.seq & 0xff;
    memcpy(((uint8_t*)io_data(io)) + ICMP_HEADER_SIZE, __ICMP_DATA_MAGIC, ICMP_DATA_MAGIC_SIZE);
    tcpips->icmp.ttl = tcpips_seconds(tcpips) + ICMP_ECHO_TIMEOUT;
    icmp_tx(tcpips, io, &tcpips->icmp.dst);
    return true;
}

static void icmp_echo_next(TCPIPS* tcpips)
{
    if (tcpips->icmp.seq < tcpips->icmp.seq_count)
    {
        ++tcpips->icmp.seq;
        if (!icmp_cmd_echo_request(tcpips))
        {
            ipc_post_inline(tcpips->icmp.process, HAL_CMD(HAL_ICMP, ICMP_PING), tcpips->icmp.dst.u32.ip, tcpips->icmp.success_count, ERROR_OUT_OF_MEMORY);
            tcpips->icmp.seq_count = 0;
        }
    }
    else
    {
        ipc_post_inline(tcpips->icmp.process, HAL_CMD(HAL_ICMP, ICMP_PING), tcpips->icmp.dst.u32.ip, tcpips->icmp.success_count, tcpips->icmp.seq_count);
        tcpips->icmp.seq_count = 0;
    }
}

static inline void icmp_ping_request(TCPIPS* tcpips, const IP* dst, unsigned int count, HANDLE process)
{
    if (tcpips->icmp.seq_count)
    {
        ipc_post_inline(process, HAL_CMD(HAL_ICMP, ICMP_PING), dst->u32.ip, 0, ERROR_IN_PROGRESS);
        return;
    }
    tcpips->icmp.seq = 0;
    tcpips->icmp.seq_count = count;
    tcpips->icmp.success_count = 0;
    ++tcpips->icmp.id;
    tcpips->icmp.process = process;
    tcpips->icmp.dst.u32.ip = dst->u32.ip;
    icmp_echo_next(tcpips);
}

static inline void icmp_cmd_echo_reply(TCPIPS* tcpips, IO* io, IP* src)
{
    bool success = true;
#if (ICMP_DEBUG)
    printf("ICMP: ECHO REPLY from ");
    ip_print(src);
    printf("\n");
#endif
    //compare src, sequence, id and data
    if (tcpips->icmp.dst.u32.ip != src->u32.ip)
        success = false;
    if (tcpips->icmp.id != ICMP_ID(((uint8_t*)io_data(io))) || tcpips->icmp.seq != ICMP_SEQ(((uint8_t*)io_data(io))))
        success = false;
    if (memcmp(((uint8_t*)io_data(io)) + ICMP_HEADER_SIZE, __ICMP_DATA_MAGIC, ICMP_DATA_MAGIC_SIZE))
        success = false;
    if (success)
        ++tcpips->icmp.success_count;
    ips_release_io(tcpips, io);
    icmp_echo_next(tcpips);
}
#endif

#if (ICMP_FLOW_CONTROL)
static inline void icmp_cmd_destination_unreachable(TCPIPS* tcpips, IO* io)
{
    IP dst;
    //useless if no original header provided
    if (io->data_size < ICMP_HEADER_SIZE + sizeof(IP_HEADER))
    {
        ips_release_io(tcpips, io);
        return;
    }
    dst.u32.ip = *((uint32_t*)(((uint8_t*)io_data(io)) + ICMP_HEADER_SIZE + 16));
#if (ICMP_DEBUG)
    printf("ICMP: DESTINATION UNREACHABLE(%d) ", ICMP_CODE(((uint8_t*)io_data(io))));
    ip_print(&dst);
    printf("\n");
#endif
    switch (ICMP_CODE(((uint8_t*)io_data(io))))
    {
    case ICMP_NET_UNREACHABLE:
    case ICMP_HOST_UNREACHABLE:
    case ICMP_SOURCE_ROUTE_FAILED:
        arps_remove_route(tcpips, &dst);
        break;
    default:
        break;
    }
}
#endif //ICMP_FLOW_CONTROL

bool icmp_request(TCPIPS* tcpips, IPC* ipc)
{
    bool need_post = false;
    IP ip;
    switch (HAL_ITEM(ipc->cmd))
    {
#if (ICMP_ECHO)
    case ICMP_PING:
        ip.u32.ip = ipc->param1;
        icmp_ping_request(tcpips, &ip, ipc->param2, ipc->process);
        break;
#endif
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

void icmp_timer(TCPIPS* tcpips, unsigned int seconds)
{
#if (ICMP_ECHO)
    if (tcpips->icmp.seq_count && tcpips->icmp.ttl < seconds)
    {
        //ping timeout, send next sequence
        icmp_echo_next(tcpips);
    }
#endif
}

void icmp_rx(TCPIPS* tcpips, IO *io, IP* src)
{
    //drop broken ICMP without control, because ICMP is control protocol itself
    if (io->data_size < ICMP_HEADER_SIZE)
    {
        ips_release_io(tcpips, io);
        return;
    }
    if (ip_checksum(((uint8_t*)io_data(io)), io->data_size))
    {
        ips_release_io(tcpips, io);
        return;
    }

    switch (ICMP_TYPE(((uint8_t*)io_data(io))))
    {
#if (ICMP_ECHO)
    case ICMP_CMD_ECHO_REPLY:
        icmp_cmd_echo_reply(tcpips, io, src);
        break;
#endif
#if (ICMP_ECHO_REPLY)
    case ICMP_CMD_ECHO:
        icmp_cmd_echo(tcpips, io, src);
        break;
#endif
#if (ICMP_FLOW_CONTROL)
    case  ICMP_CMD_DESTINATION_UNREACHABLE:
        icmp_cmd_destination_unreachable(tcpips, io);
        break;
#endif //ICMP_FLOW_CONTROL
    default:
#if (ICMP_DEBUG)
        printf("ICMP: unhandled type %d from ", ICMP_TYPE(((uint8_t*)io_data(io))));
        ip_print(src);
        printf("\n");
#endif
        ips_release_io(tcpips, io);
        break;

    }
}

#if (ICMP_FLOW_CONTROL)
static void icmp_control_prepare(TCPIPS* tcpips, uint8_t cmd, uint8_t code, IO* original)
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
    memmove(((uint8_t*)io_data(original)) + ICMP_HEADER_SIZE, ((uint8_t*)io_data(original)) - ip_stack->hdr_size, ip_stack->hdr_size + 8);
}


void icmp_destination_unreachable(TCPIPS* tcpips, uint8_t code, IO* original, const IP *dst)
{
#if (ICMP_DEBUG)
    printf("ICMP: Destination unreachable(%d) to ", code);
    ip_print(dst);
    printf("\n");
#endif
    icmp_control_prepare(tcpips, ICMP_CMD_DESTINATION_UNREACHABLE, code, original);
    icmp_tx(tcpips, original, dst);
}

void icmp_time_exceeded(TCPIPS* tcpips, uint8_t code, IO* original, const IP* dst)
{
#if (ICMP_DEBUG)
    printf("ICMP: Time exceeded(%d) to ", code);
    ip_print(dst);
    printf("\n");
#endif
    icmp_control_prepare(tcpips, ICMP_CMD_TIME_EXCEEDED, code, original);
    icmp_tx(tcpips, original, dst);
}

void icmp_parameter_problem(TCPIPS* tcpips, uint8_t offset, IO* original, const IP* dst)
{
#if (ICMP_DEBUG)
    printf("ICMP: Parameter problem(%d) to ", offset);
    ip_print(dst);
    printf("\n");
#endif
    icmp_control_prepare(tcpips, ICMP_CMD_PARAMETER_PROBLEM, 0, original);
    ((uint8_t*)io_data(original))[4] = offset;
    icmp_tx(tcpips, original, dst);
}

#endif //ICMP_FLOW_CONTROL
