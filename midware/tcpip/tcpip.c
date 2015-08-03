/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "tcpip.h"
#include "tcpip_private.h"
#include "../../userspace/ipc.h"
#include "../../userspace/object.h"
#include "../../userspace/stdio.h"
#include "../../userspace/systime.h"
#include "sys_config.h"
#include "mac.h"
#include "arp.h"
#include "route.h"
#include "ip.h"

#define FRAME_MAX_SIZE                          (TCPIP_MTU + MAC_HEADER_SIZE)

void tcpip_main();

const REX __TCPIP = {
    //name
    "TCP/IP stack",
    //size
    TCPIP_PROCESS_SIZE,
    //priority - midware priority
    TCPIP_PROCESS_PRIORITY,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //function
    tcpip_main
};

#if (TCPIP_DEBUG)
static void print_conn_status(TCPIP* tcpip, const char* head)
{
    printf("%s: ", head);
    switch (tcpip->conn)
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

static IO* tcpip_allocate_io_internal(TCPIP* tcpip)
{
    IO* io = NULL;
    if (array_size(tcpip->free_io))
    {
        io = *((IO**)array_at(tcpip->free_io, array_size(tcpip->free_io) - 1));
        array_remove(&tcpip->free_io, array_size(tcpip->free_io) - 1);
    }
    return io;
}

IO* tcpip_allocate_io(TCPIP* tcpip)
{
    IO* io = tcpip_allocate_io_internal(tcpip);
    if (io == NULL)
    {
        if (tcpip->io_allocated < TCPIP_MAX_FRAMES_COUNT)
        {
            io = io_create(FRAME_MAX_SIZE);
            if (io != NULL)
                ++tcpip->io_allocated;
#if (TCPIP_DEBUG_ERRORS)
            else
                printf("TCPIP: out of memory\n");
#endif
        }
        //try to drop first in queue, waiting for resolve
        else if (route_drop(tcpip))
        {
            io = tcpip_allocate_io_internal(tcpip);
#if (TCPIP_DEBUG)
            printf("TCPIP warning: block dropped from route queue\n");
#endif
        }
        else if (array_size(tcpip->tx_queue))
        {
            array_remove(&tcpip->tx_queue, 0);
            --tcpip->tx_count;
            io = tcpip_allocate_io_internal(tcpip);
#if (TCPIP_DEBUG)
            printf("TCPIP warning: block dropped from tx queue\n");
#endif
        }
        else
        {
#if (TCPIP_DEBUG_ERRORS)
            printf("TCPIP: too many blocks\n");
#endif
        }
    }
    return io;
}

void tcpip_release_io(TCPIP* tcpip, IO* io)
{
    io_reset(io);
    array_append(&tcpip->free_io);
    *((IO**)array_at(tcpip->free_io, array_size(tcpip->free_io) - 1)) = io;
}

static void tcpip_rx_next(TCPIP* tcpip)
{
    IO* io = tcpip_allocate_io(tcpip);
    if (io == NULL)
        return;
    io_read(tcpip->eth, HAL_CMD(HAL_ETH, IPC_READ), 0, io, FRAME_MAX_SIZE);
}

void tcpip_tx(TCPIP* tcpip, IO *io)
{
#if (ETH_DOUBLE_BUFFERING)
    if (++tcpip->tx_count > 2)
#else
    if (++tcpip->tx_count > 1)
#endif
    {
        //add to queue
        array_append(&tcpip->tx_queue);
        *((IO**)array_at(tcpip->tx_queue, array_size(tcpip->tx_queue) - 1)) = io;
    }
    else
        io_write(tcpip->eth, HAL_CMD(HAL_ETH, IPC_WRITE), 0, io);
}

unsigned int tcpip_seconds(TCPIP* tcpip)
{
    return tcpip->seconds;
}

static inline void tcpip_open(TCPIP* tcpip, ETH_CONN_TYPE conn)
{
    if (tcpip->active)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    tcpip->timer = timer_create(0, HAL_TCPIP);
    if (tcpip->timer == INVALID_HANDLE)
        return;
    ack(tcpip->eth, HAL_CMD(HAL_ETH, IPC_OPEN), 0, conn, 0);
    tcpip->active = true;
    tcpip->seconds = 0;
    timer_start_ms(tcpip->timer, 1000, 0);
}

static inline void tcpip_eth_rx(TCPIP* tcpip, IO* io, int param3)
{
    if (tcpip->connected)
        tcpip_rx_next(tcpip);
    if (param3 < 0)
    {
        tcpip_release_io(tcpip, io);
        return;
    }
    //forward to MAC
    mac_rx(tcpip, io);
}

static inline void tcpip_eth_tx_complete(TCPIP* tcpip, IO* io, int param3)
{
    IO* queue_io;
    tcpip_release_io(tcpip, io);
#if (ETH_DOUBLE_BUFFERING)
    if (--tcpip->tx_count >= 2)
#else
    if (--tcpip->tx_count >= 1)
#endif
    {
        //send next in queue
        queue_io = *((IO**)array_at(tcpip->tx_queue, 0));
        array_remove(&tcpip->tx_queue, 0);
        io_write(tcpip->eth, HAL_CMD(HAL_ETH, IPC_WRITE), 0, queue_io);
    }
}

static inline void tcpip_link_changed(TCPIP* tcpip, ETH_CONN_TYPE conn)
{
    tcpip->conn = conn;
    tcpip->connected = ((conn != ETH_NO_LINK) && (conn != ETH_REMOTE_FAULT));

#if (TCPIP_DEBUG)
    print_conn_status(tcpip, "ETH link changed");
#endif

    if (tcpip->connected)
    {
        tcpip_rx_next(tcpip);
#if (ETH_DOUBLE_BUFFERING)
        tcpip_rx_next(tcpip);
#endif
    }
    arp_link_event(tcpip, tcpip->connected);
}

void tcpip_init(TCPIP* tcpip)
{
    tcpip->eth = object_get(SYS_OBJ_ETH);
    tcpip->timer = INVALID_HANDLE;
    tcpip->conn = ETH_NO_LINK;
    tcpip->connected = tcpip->active = false;
    tcpip->io_allocated = 0;
#if (ETH_DOUBLE_BUFFERING)
    //2 rx + 2 tx + 1 for processing
    array_create(&tcpip->free_io, sizeof(IO*), 5);
#else
    //1 rx + 1 tx + 1 for processing
    array_create(&tcpip->free_io, sizeof(IO*), 3);
#endif
    array_create(&tcpip->tx_queue, sizeof(IO*), 1);
    tcpip->tx_count = 0;
    mac_init(tcpip);
    arp_init(tcpip);
    route_init(tcpip);
    ip_init(tcpip);
#if (ICMP)
    icmp_init(tcpip);
#endif
}

static inline void tcpip_timer(TCPIP* tcpip)
{
    ++tcpip->seconds;
    //forward to others
    arp_timer(tcpip, tcpip->seconds);
    icmp_timer(tcpip, tcpip->seconds);
    timer_start_ms(tcpip->timer, 1000, 0);
}

static inline bool tcpip_request(TCPIP* tcpip, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        tcpip_open(tcpip, ipc->param2);
        need_post = true;
        break;
    case IPC_CLOSE:
        //TODO:
        need_post = true;
        break;
    case IPC_TIMEOUT:
        tcpip_timer(tcpip);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

static inline bool tcpip_driver_event(TCPIP* tcpip, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_READ:
        tcpip_eth_rx(tcpip, (IO*)ipc->param2, (int)ipc->param3);
        break;
    case IPC_WRITE:
        tcpip_eth_tx_complete(tcpip, (IO*)ipc->param2, (int)ipc->param3);
        break;
    case ETH_NOTIFY_LINK_CHANGED:
        tcpip_link_changed(tcpip, ipc->param2);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

void tcpip_main()
{
    IPC ipc;
    TCPIP tcpip;
    bool need_post;
    tcpip_init(&tcpip);
#if (TCPIP_DEBUG)
    open_stdout();
#endif
    object_set_self(SYS_OBJ_TCPIP);
    for (;;)
    {
        ipc_read(&ipc);
        need_post = false;
        switch (HAL_GROUP(ipc.cmd))
        {
        case HAL_ETH:
            need_post = tcpip_driver_event(&tcpip, &ipc);
            break;
        case HAL_TCPIP:
            need_post = tcpip_request(&tcpip, &ipc);
            break;
        case HAL_MAC:
            need_post = mac_request(&tcpip, &ipc);
            break;
        case HAL_ARP:
            need_post = arp_request(&tcpip, &ipc);
            break;
        case HAL_IP:
            need_post = ip_request(&tcpip, &ipc);
            break;
        case HAL_ICMP:
            need_post = icmp_request(&tcpip, &ipc);
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
