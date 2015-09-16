/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "tcpips.h"
#include "tcpips_private.h"
#include "../../userspace/ipc.h"
#include "../../userspace/object.h"
#include "../../userspace/stdio.h"
#include "../../userspace/systime.h"
#include "../../userspace/sys.h"
#include "sys_config.h"
#include "mac.h"
#include "arp.h"
#include "route.h"
#include "ips.h"

#define FRAME_MAX_SIZE                          (TCPIP_MTU + MAC_HEADER_SIZE)

void tcpips_main();

const REX __TCPIP = {
    //name
    "TCP/IP stack",
    //size
    TCPIP_PROCESS_SIZE,
    //priority - midware priority
    TCPIP_PROCESS_PRIORITY,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_FLAG_PERSISTENT_NAME,
    //function
    tcpips_main
};

#if (TCPIP_DEBUG)
static void print_conn_status(TCPIPS* tcpips, const char* head)
{
    printf("%s: ", head);
    switch (tcpips->conn)
    {
    case ETH_10_HALF:
        printf("10BASE_T Half duplex");
        break;
    case ETH_10_FULL:
        printf("10BASE_T Full duplex");
        break;
    case ETH_100_HALF:
        printf("100BASE_TX Half duplex");
        break;
    case ETH_100_FULL:
        printf("100BASE_TX Full duplex");
        break;
    case ETH_NO_LINK:
        printf("no link detected");
        break;
    default:
        printf("remote fault");
        break;
    }
    printf("\n");
}
#endif

static IO* tcpips_allocate_io_internal(TCPIPS* tcpips)
{
    IO* io = NULL;
    if (array_size(tcpips->free_io))
    {
        io = *((IO**)array_at(tcpips->free_io, array_size(tcpips->free_io) - 1));
        array_remove(&tcpips->free_io, array_size(tcpips->free_io) - 1);
    }
    return io;
}

IO* tcpips_allocate_io(TCPIPS* tcpips)
{
    IO* io = tcpips_allocate_io_internal(tcpips);
    if (io == NULL)
    {
        if (tcpips->io_allocated < TCPIP_MAX_FRAMES_COUNT)
        {
            io = io_create(FRAME_MAX_SIZE);
            if (io != NULL)
                ++tcpips->io_allocated;
#if (TCPIP_DEBUG_ERRORS)
            else
                printf("TCPIP: out of memory\n");
#endif
        }
        //try to drop first in queue, waiting for resolve
        else if (route_drop(tcpips))
        {
            io = tcpips_allocate_io_internal(tcpips);
#if (TCPIP_DEBUG)
            printf("TCPIP warning: io dropped from route queue\n");
#endif
        }
        else if (array_size(tcpips->tx_queue))
        {
            array_remove(&tcpips->tx_queue, 0);
            --tcpips->tx_count;
            io = tcpips_allocate_io_internal(tcpips);
#if (TCPIP_DEBUG)
            printf("TCPIP warning: io dropped from tx queue\n");
#endif
        }
        else
        {
#if (TCPIP_DEBUG_ERRORS)
            printf("TCPIP: too many ios\n");
#endif
        }
    }
    return io;
}

void tcpips_release_io(TCPIPS* tcpips, IO* io)
{
    io_reset(io);
    array_append(&tcpips->free_io);
    *((IO**)array_at(tcpips->free_io, array_size(tcpips->free_io) - 1)) = io;
}

static void tcpips_rx_next(TCPIPS* tcpips)
{
    IO* io = tcpips_allocate_io(tcpips);
    if (io == NULL)
        return;
    io_read(tcpips->eth, HAL_IO_CMD(HAL_ETH, IPC_READ), 0, io, FRAME_MAX_SIZE);
}

void tcpips_tx(TCPIPS* tcpips, IO *io)
{
#if (ETH_DOUBLE_BUFFERING)
    if (++tcpips->tx_count > 2)
#else
    if (++tcpips->tx_count > 1)
#endif
    {
        //add to queue
        array_append(&tcpips->tx_queue);
        *((IO**)array_at(tcpips->tx_queue, array_size(tcpips->tx_queue) - 1)) = io;
    }
    else
        io_write(tcpips->eth, HAL_IO_CMD(HAL_ETH, IPC_WRITE), 0, io);
}

unsigned int tcpips_seconds(TCPIPS* tcpips)
{
    return tcpips->seconds;
}

static inline void tcpips_open(TCPIPS* tcpips, ETH_CONN_TYPE conn)
{
    if (tcpips->active)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    tcpips->timer = timer_create(0, HAL_TCPIP);
    if (tcpips->timer == INVALID_HANDLE)
        return;
    ack(tcpips->eth, HAL_CMD(HAL_ETH, IPC_OPEN), 0, conn, 0);
    tcpips->active = true;
    tcpips->seconds = 0;
    timer_start_ms(tcpips->timer, 1000);
}

static inline void tcpips_eth_rx(TCPIPS* tcpips, IO* io, int param3)
{
    if (tcpips->connected)
        tcpips_rx_next(tcpips);
    if (param3 < 0)
    {
        tcpips_release_io(tcpips, io);
        return;
    }
    //forward to MAC
    mac_rx(tcpips, io);
}

static inline void tcpips_eth_tx_complete(TCPIPS* tcpips, IO* io, int param3)
{
    IO* queue_io;
    tcpips_release_io(tcpips, io);
#if (ETH_DOUBLE_BUFFERING)
    if (--tcpips->tx_count >= 2)
#else
    if (--tcpips->tx_count >= 1)
#endif
    {
        //send next in queue
        queue_io = *((IO**)array_at(tcpips->tx_queue, 0));
        array_remove(&tcpips->tx_queue, 0);
        io_write(tcpips->eth, HAL_IO_CMD(HAL_ETH, IPC_WRITE), 0, queue_io);
    }
}

static inline void tcpips_link_changed(TCPIPS* tcpips, ETH_CONN_TYPE conn)
{
    tcpips->conn = conn;
    tcpips->connected = ((conn != ETH_NO_LINK) && (conn != ETH_REMOTE_FAULT));

#if (TCPIP_DEBUG)
    print_conn_status(tcpips, "ETH link changed");
#endif

    if (tcpips->connected)
    {
        tcpips_rx_next(tcpips);
#if (ETH_DOUBLE_BUFFERING)
        tcpips_rx_next(tcpips);
#endif
    }
    arp_link_event(tcpips, tcpips->connected);
}

void tcpips_init(TCPIPS* tcpips)
{
    tcpips->eth = object_get(SYS_OBJ_ETH);
    tcpips->timer = INVALID_HANDLE;
    tcpips->conn = ETH_NO_LINK;
    tcpips->connected = tcpips->active = false;
    tcpips->io_allocated = 0;
#if (ETH_DOUBLE_BUFFERING)
    //2 rx + 2 tx + 1 for processing
    array_create(&tcpips->free_io, sizeof(IO*), 5);
#else
    //1 rx + 1 tx + 1 for processing
    array_create(&tcpips->free_io, sizeof(IO*), 3);
#endif
    array_create(&tcpips->tx_queue, sizeof(IO*), 1);
    tcpips->tx_count = 0;
    mac_init(tcpips);
    arp_init(tcpips);
    route_init(tcpips);
    ips_init(tcpips);
#if (ICMP)
    icmp_init(tcpips);
#endif
}

static inline void tcpips_timer(TCPIPS* tcpips)
{
    ++tcpips->seconds;
    //forward to others
    arp_timer(tcpips, tcpips->seconds);
    icmp_timer(tcpips, tcpips->seconds);
    timer_start_ms(tcpips->timer, 1000);
}

static inline bool tcpips_request(TCPIPS* tcpips, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        tcpips_open(tcpips, ipc->param2);
        need_post = true;
        break;
    case IPC_CLOSE:
        //TODO:
        need_post = true;
        break;
    case IPC_TIMEOUT:
        tcpips_timer(tcpips);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

static inline bool tcpips_driver_event(TCPIPS* tcpips, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_READ:
        tcpips_eth_rx(tcpips, (IO*)ipc->param2, (int)ipc->param3);
        break;
    case IPC_WRITE:
        tcpips_eth_tx_complete(tcpips, (IO*)ipc->param2, (int)ipc->param3);
        break;
    case ETH_NOTIFY_LINK_CHANGED:
        tcpips_link_changed(tcpips, ipc->param2);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

void tcpips_main()
{
    IPC ipc;
    TCPIPS tcpips;
    bool need_post;
    tcpips_init(&tcpips);
#if (TCPIP_DEBUG)
    open_stdout();
#endif
    for (;;)
    {
        ipc_read(&ipc);
        need_post = false;
        switch (HAL_GROUP(ipc.cmd))
        {
        case HAL_ETH:
            need_post = tcpips_driver_event(&tcpips, &ipc);
            break;
        case HAL_TCPIP:
            need_post = tcpips_request(&tcpips, &ipc);
            break;
        case HAL_MAC:
            need_post = mac_request(&tcpips, &ipc);
            break;
        case HAL_ARP:
            need_post = arp_request(&tcpips, &ipc);
            break;
        case HAL_IP:
            need_post = ips_request(&tcpips, &ipc);
            break;
        case HAL_ICMP:
            need_post = icmp_request(&tcpips, &ipc);
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
            need_post = true;
            break;
        }
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}
