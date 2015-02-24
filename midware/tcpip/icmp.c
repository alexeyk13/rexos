/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "icmp.h"
#include "tcpip_private.h"
#include "tcpip_private.h"
#include "../../userspace/stdio.h"
#include <string.h>

#define ICMP_HEADER_SIZE                                            4
#define ICMP_TYPE(buf)                                              ((buf)[0])

#if (ICMP_ECHO)
//64 - ip header - mac header - 2 inverted sequence number?!
#define ICMP_DATA_MAGIC_SIZE                                        22
#define ICMP_ECHO_HEADER_SIZE                                       8

static const uint8_t __ICMP_DATA_MAGIC[ICMP_DATA_MAGIC_SIZE] =      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22};
#endif

void icmp_init(TCPIP* tcpip)
{
    tcpip->icmp.id = 0;
    tcpip->icmp.seq_count = 0;
}

static void icmp_tx(TCPIP* tcpip, IP_IO* ip_io, IP* dst)
{
    ip_io->io.buf[2] = ip_io->io.buf[3] = 0;
    uint16_t cs = ip_checksum(ip_io->io.buf, ip_io->io.size);
    ip_io->io.buf[2] = (cs >> 8) & 0xff;
    ip_io->io.buf[3] = cs & 0xff;
    ip_tx(tcpip, ip_io, dst);
}

#if (ICMP_ECHO)
static inline void icmp_cmd_echo(TCPIP* tcpip, IP_IO* ip_io, IP* src)
{
#if (ICMP_DEBUG)
    printf("ICMP: ECHO from ", ICMP_TYPE(ip_io->io.buf));
    ip_print(src);
    printf("\n\r");
#endif
    ip_io->io.buf[0] = ICMP_CMD_ECHO_REPLY;
    icmp_tx(tcpip, ip_io, src);
}

static bool icmp_cmd_echo_request(TCPIP* tcpip)
{
    IP_IO ip_io;
    if (ip_allocate_io(tcpip, &ip_io, ICMP_DATA_MAGIC_SIZE + ICMP_ECHO_HEADER_SIZE, PROTO_ICMP) == NULL)
        return false;
    ip_io.io.size = ICMP_DATA_MAGIC_SIZE + ICMP_ECHO_HEADER_SIZE;
    //type
    ip_io.io.buf[0] = ICMP_CMD_ECHO;
    //code
    ip_io.io.buf[1] = 0;
    //id
    ip_io.io.buf[4] = (tcpip->icmp.id >> 8) & 0xff;
    ip_io.io.buf[5] = tcpip->icmp.id & 0xff;
    //sequence
    ip_io.io.buf[6] = (tcpip->icmp.seq >> 8) & 0xff;
    ip_io.io.buf[7] = tcpip->icmp.seq & 0xff;
    ++tcpip->icmp.seq;
    memcpy(ip_io.io.buf + ICMP_ECHO_HEADER_SIZE, __ICMP_DATA_MAGIC, ICMP_DATA_MAGIC_SIZE);
    tcpip->icmp.ttl = tcpip_seconds(tcpip) + ICMP_ECHO_TIMEOUT;
    icmp_tx(tcpip, &ip_io, &tcpip->icmp.dst);
    return true;
}
#endif

static void icmp_echo_next(TCPIP* tcpip)
{
    if (tcpip->icmp.seq <= tcpip->icmp.seq_count)
    {
        if (!icmp_cmd_echo_request(tcpip))
        {
            ipc_post_inline(tcpip->icmp.process, ICMP_PING, tcpip->icmp.dst.u32.ip, tcpip->icmp.seq_count, tcpip->icmp.success_count);
            tcpip->icmp.seq_count = 0;
        }
    }
    else
    {
        ipc_post_inline(tcpip->icmp.process, ICMP_PING, tcpip->icmp.dst.u32.ip, tcpip->icmp.seq_count, tcpip->icmp.success_count);
        tcpip->icmp.seq_count = 0;
    }
}

static inline void icmp_ping(TCPIP* tcpip, const IP* dst, unsigned int count, HANDLE process)
{
    if (tcpip->icmp.seq_count)
    {
        ipc_post_inline(process, ICMP_PING, dst->u32.ip, count, ERROR_IN_PROGRESS);
        return;
    }
    tcpip->icmp.seq = 1;
    tcpip->icmp.seq_count = count;
    tcpip->icmp.success_count = 0;
    ++tcpip->icmp.id;
    tcpip->icmp.process = process;
    tcpip->icmp.dst.u32.ip = dst->u32.ip;
    icmp_echo_next(tcpip);
}

bool icmp_request(TCPIP* tcpip, IPC* ipc)
{
    bool need_post = false;
    IP ip;
    switch (ipc->cmd)
    {
//TODO: ICMP_INFO
#if (ICMP_ECHO)
    case ICMP_PING:
        ip.u32.ip = ipc->param1;
        icmp_ping(tcpip, &ip, ipc->param2, ipc->process);
        break;
#endif
    default:
        break;
    }
    return need_post;
}

void icmp_rx(TCPIP* tcpip, IP_IO* ip_io, IP* src)
{
    //drop broken ICMP without control, because ICMP is control protocol itself
    if (ip_io->io.size < ICMP_HEADER_SIZE)
    {
        ip_release_io(tcpip, ip_io);
        return;
    }
    if (ip_checksum(ip_io->io.buf, ip_io->io.size))
    {
        ip_release_io(tcpip, ip_io);
        return;
    }

    switch (ICMP_TYPE(ip_io->io.buf))
    {
#if (ICMP_ECHO)
    case ICMP_CMD_ECHO:
        icmp_cmd_echo(tcpip, ip_io, src);
        break;
#endif
/*    case ICMP_CMD_ECHO_REPLY:
    case ICMP_CMD_DESTINATION_UNREACHABLE:
    case ICMP_CMD_SOURCE_QUENCH:
    case ICMP_CMD_REDIRECT:
    case ICMP_CMD_TIME_EXCEEDED:
    case ICMP_CMD_PARAMETER_PROBLEM:
    case ICMP_CMD_TIMESTAMP:
    case ICMP_CMD_TIMESTAMP_REPLY:
    case ICMP_CMD_INFORMATION_REQUEST:
    case ICMP_CMD_INFORMATION_REPLY:*/
    default:
#if (ICMP_DEBUG)
        printf("ICMP: unhandled type %d from ", ICMP_TYPE(ip_io->io.buf));
        ip_print(src);
        printf("\n\r");
#endif
        ip_release_io(tcpip, ip_io);
        break;

    }
}
