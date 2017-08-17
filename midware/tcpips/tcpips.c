/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "tcpips.h"
#include "tcpips_private.h"
#include "../../userspace/tcpip.h"
#include "../../userspace/ipc.h"
#include "../../userspace/object.h"
#include "../../userspace/stdio.h"
#include "../../userspace/systime.h"
#include "../../userspace/sys.h"
#include "sys_config.h"
#include "macs.h"
#include "arps.h"
#include "routes.h"
#include "ips.h"
#include "udps.h"
#include "dhcps.h"
#include "tcps.h"

#define FRAME_MAX_SIZE                          (TCPIP_MTU + sizeof(MAC_HEADER) + sizeof(IP_STACK))

const IP __LOCALHOST =                          {{127, 0, 0, 1}};
const IP __BROADCAST =                          {{255, 255, 255, 255}};

#if (TCPIP_DEBUG)
static void print_conn_status(TCPIPS* tcpips, const char* head, ETH_CONN_TYPE conn)
{
    printf("%s: ", head);
    switch (conn)
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
            io = io_create(FRAME_MAX_SIZE + tcpips->eth_header_size);
            if (io != NULL)
            {
                ++tcpips->io_allocated;
                io->data_offset += tcpips->eth_header_size;
            }
#if (TCPIP_DEBUG_ERRORS)
            else
                printf("TCPIP: out of memory\n");
#endif
        }
        //try to drop first in queue, waiting for resolve
        else if (routes_drop(tcpips))
        {
            io = tcpips_allocate_io_internal(tcpips);
#if (TCPIP_DEBUG)
            printf("TCPIP warning: io dropped from route queue\n");
#endif
        }
        else if (array_size(tcpips->tx_queue))
        {
            io = *((IO**)array_at(tcpips->tx_queue, 0));
            array_remove(&tcpips->tx_queue, 0);
            tcpips_release_io(tcpips, io);
            io = tcpips_allocate_io_internal(tcpips);
#if (TCPIP_DEBUG)
            printf("TCPIP warning: io dropped from tx queue\n");
#endif
        }
        else
        {
            error(ERROR_TOO_MANY_HANDLES);
#if (TCPIP_DEBUG_ERRORS)
            printf("TCPIP: too many ios\n");
#endif
        }
    }
    return io;
}

void tcpips_release_io(TCPIPS* tcpips, IO* io)
{
    IO** iop;
    io_reset(io);
    io->data_offset += tcpips->eth_header_size;
    iop = array_append(&tcpips->free_io);
    if (iop)
        *iop = io;
}

static void tcpips_rx_next(TCPIPS* tcpips)
{
    IO* io = tcpips_allocate_io(tcpips);
    if (io == NULL)
        return;
    io_read(tcpips->eth, HAL_IO_REQ(HAL_ETH, IPC_READ), tcpips->eth_handle, io, FRAME_MAX_SIZE);
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
        io_write(tcpips->eth, HAL_IO_REQ(HAL_ETH, IPC_WRITE), tcpips->eth_handle, io);
}

static inline void tcpips_open(TCPIPS* tcpips, unsigned int eth_handle, HANDLE eth, ETH_CONN_TYPE conn, HANDLE app)
{
    if (tcpips->app != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    tcpips->timer = timer_create(0, HAL_TCPIP);
    if (tcpips->timer == INVALID_HANDLE)
        return;
    tcpips->eth = eth;
    tcpips->eth_handle = eth_handle;
    tcpips->app = app;
    ack(tcpips->eth, HAL_REQ(HAL_ETH, IPC_OPEN), tcpips->eth_handle, conn, 0);
    tcpips->eth_header_size = eth_get_header_size(tcpips->eth, tcpips->eth_handle);
}

static void tcpips_close_internal(TCPIPS* tcpips)
{
    tcpips->app = INVALID_HANDLE;
    timer_destroy(tcpips->timer);
}

static inline void tcpips_close(TCPIPS* tcpips)
{
    if (tcpips->app == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    ack(tcpips->eth, HAL_REQ(HAL_ETH, IPC_CLOSE), tcpips->eth_handle, 0, 0);
    tcpips_close_internal(tcpips);
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
    macs_rx(tcpips, io);
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
        io_write(tcpips->eth, HAL_IO_REQ(HAL_ETH, IPC_WRITE), tcpips->eth_handle, queue_io);
    }
}

static void tcpips_link_changed_internal(TCPIPS* tcpips, ETH_CONN_TYPE conn)
{
    bool was_connected = tcpips->connected;
    tcpips->conn = conn;
    tcpips->connected = ((conn != ETH_NO_LINK) && (conn != ETH_REMOTE_FAULT));
    if (was_connected == tcpips->connected)
        return;

    if (tcpips->connected)
    {
        tcpips_rx_next(tcpips);
#if (ETH_DOUBLE_BUFFERING)
        tcpips_rx_next(tcpips);
#endif
    }
    else
    {
        //flush TX queue
        while (array_size(tcpips->tx_queue))
        {
            tcpips_release_io(tcpips, array_at(tcpips->tx_queue, 0));
            array_remove(&tcpips->tx_queue, 0);
            --tcpips->tx_count;
        }
    }
    macs_link_changed(tcpips, tcpips->connected);
    arps_link_changed(tcpips, tcpips->connected);
    routes_link_changed(tcpips, tcpips->connected);
#if (ICMP)
    icmps_link_changed(tcpips, tcpips->connected);
#endif //ICMP
    ips_link_changed(tcpips, tcpips->connected);
#if (UDP)
    udps_link_changed(tcpips, tcpips->connected);
#endif //UDP
    tcps_link_changed(tcpips, tcpips->connected);
    if (tcpips->connected)
    {
        tcpips->seconds = 0;
        timer_start_ms(tcpips->timer, 1000);
    }
}

static inline void tcpips_link_changed(TCPIPS* tcpips, ETH_CONN_TYPE conn)
{
#if (TCPIP_DEBUG)
    print_conn_status(tcpips, "ETH link changed", conn);
#endif
    tcpips_link_changed_internal(tcpips, conn);
}

static inline void tcpips_eth_remote_close(TCPIPS* tcpips)
{
#if (TCPIP_DEBUG)
    printf("ETH remote close\n");
#endif
    tcpips_link_changed_internal(tcpips, ETH_NO_LINK);
    tcpips_close_internal(tcpips);
}

void tcpips_init(TCPIPS* tcpips)
{
    tcpips->app = INVALID_HANDLE;
    tcpips->timer = INVALID_HANDLE;
    tcpips->conn = ETH_NO_LINK;
    tcpips->connected = false;
    tcpips->io_allocated = 0;
    tcpips->eth_header_size = 0;
#if (ETH_DOUBLE_BUFFERING)
    //2 rx + 2 tx + 1 for processing
    array_create(&tcpips->free_io, sizeof(IO*), 5);
#else
    //1 rx + 1 tx + 1 for processing
    array_create(&tcpips->free_io, sizeof(IO*), 3);
#endif
    array_create(&tcpips->tx_queue, sizeof(IO*), 1);
    tcpips->tx_count = 0;
    macs_init(tcpips);
    arps_init(tcpips);
    routes_init(tcpips);
    ips_init(tcpips);
#if (ICMP)
    icmps_init(tcpips);
#endif //ICMP
#if (UDP)
    udps_init(tcpips);
#endif //UDP
#if (DHCPS)
    dhcps_init(tcpips);
#endif //UDP
#if (DNSS)
    dnss_init(tcpips);
#endif //UDP
    tcps_init(tcpips);
}

static inline void tcpips_timer(TCPIPS* tcpips)
{
    if (tcpips->connected)
    {
        ++tcpips->seconds;
        //forward to others
        arps_timer(tcpips, tcpips->seconds);
        icmps_timer(tcpips, tcpips->seconds);
#if (IP_FRAGMENTATION)
        ips_timer(tcpips, tcpips->seconds);
#endif //IP_FRAGMENTATION
        timer_start_ms(tcpips->timer, 1000);
    }
}

static inline void tcpips_get_conn_state(TCPIPS* tcpips, HANDLE process)
{
    ipc_post_inline(process, HAL_CMD(HAL_TCPIP, TCPIP_GET_CONN_STATE), 0, tcpips->conn, 0);
    error(ERROR_SYNC);
}

static inline void tcpips_request(TCPIPS* tcpips, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        tcpips_open(tcpips, ipc->param1, ipc->param2, ipc->param3, ipc->process);
        break;
    case IPC_CLOSE:
        tcpips_close(tcpips);
        break;
    case IPC_TIMEOUT:
        tcpips_timer(tcpips);
        break;
    case TCPIP_GET_CONN_STATE:
        tcpips_get_conn_state(tcpips, ipc->process);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

static inline void tcpips_driver_event(TCPIPS* tcpips, IPC* ipc)
{
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
    case IPC_CLOSE:
        tcpips_eth_remote_close(tcpips);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

void tcpips_main()
{
    IPC ipc;
    TCPIPS tcpips;
    tcpips_init(&tcpips);
#if (TCPIP_DEBUG)
    open_stdout();
#endif
    for (;;)
    {
        ipc_read(&ipc);
        switch (HAL_GROUP(ipc.cmd))
        {
        case HAL_ETH:
            tcpips_driver_event(&tcpips, &ipc);
            break;
        case HAL_TCPIP:
            tcpips_request(&tcpips, &ipc);
            break;
        case HAL_MAC:
            macs_request(&tcpips, &ipc);
            break;
        case HAL_ARP:
            arps_request(&tcpips, &ipc);
            break;
        case HAL_IP:
            ips_request(&tcpips, &ipc);
            break;
#if (ICMP)
        case HAL_ICMP:
            icmps_request(&tcpips, &ipc);
            break;
#endif //ICMP
#if (UDP)
        case HAL_UDP:
            udps_request(&tcpips, &ipc);
            break;
#endif //UDP
#if (DNSS)
        case HAL_DNS:
            dnss_request(&tcpips, &ipc);
            break;
#endif //DNSS
#if (DHCPS)
        case HAL_DHCPS:
            dhcps_request(&tcpips, &ipc);
            break;
#endif //DHCPS
        case HAL_TCP:
            tcps_request(&tcpips, &ipc);
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
            break;
        }
        ipc_write(&ipc);
    }
}
