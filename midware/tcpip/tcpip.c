/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "tcpip.h"
#include "tcpip_private.h"
#include "../../userspace/ipc.h"
#include "../../userspace/sys.h"
#include "../../userspace/object.h"
#include "../../userspace/stdio.h"
#include "../../userspace/file.h"
#include "../../userspace/block.h"
#include "../../userspace/timer.h"
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
    //ipc size
    TCPIP_PROCESS_IPC_COUNT,
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
    printf("\n\r");
}
#endif

static HANDLE tcpip_allocate_io_internal(TCPIP* tcpip)
{
    HANDLE res = INVALID_HANDLE;
    if (array_size(tcpip->free_blocks))
    {
        res = *(HANDLE*)array_at(tcpip->free_blocks, array_size(tcpip->free_blocks) - 1);
        array_remove(&tcpip->free_blocks, array_size(tcpip->free_blocks) - 1);
    }
    return res;
}

uint8_t* tcpip_allocate_io(TCPIP* tcpip, TCPIP_IO* io)
{
    io->buf = NULL;
    io->size = 0;
    io->block = tcpip_allocate_io_internal(tcpip);

    if (io->block == INVALID_HANDLE)
    {
        if (tcpip->blocks_allocated < TCPIP_MAX_FRAMES_COUNT)
        {
            io->block = block_create(FRAME_MAX_SIZE);
            if (io->block != INVALID_HANDLE)
                ++tcpip->blocks_allocated;
#if (TCPIP_DEBUG_ERRORS)
            else
                printf("TCPIP: out of memory\n\r");
#endif
        }
        //try to drop first in queue, waiting for resolve
        else if (route_drop(tcpip))
        {
            io->block = tcpip_allocate_io_internal(tcpip);
#if (TCPIP_DEBUG)
            printf("TCPIP warning: block dropped from route queue\n\r");
#endif
        }
        else if (array_size(tcpip->tx_queue))
        {
            array_remove(&tcpip->tx_queue, 0);
            --tcpip->tx_count;
            io->block = tcpip_allocate_io_internal(tcpip);
#if (TCPIP_DEBUG)
            printf("TCPIP warning: block dropped from tx queue\n\r");
#endif
        }
        else
        {
#if (TCPIP_DEBUG_ERRORS)
            printf("TCPIP: too many blocks\n\r");
#endif
        }
    }
    if (io->block != INVALID_HANDLE)
        io->buf = block_open(io->block);
    return io->buf;
}

static void tcpip_release_block(TCPIP* tcpip, HANDLE block)
{
    array_append(&tcpip->free_blocks);
    *(void**)array_at(tcpip->free_blocks, array_size(tcpip->free_blocks) - 1) = (void*)block;
}

void tcpip_release_io(TCPIP* tcpip, TCPIP_IO* io)
{
    tcpip_release_block(tcpip, io->block);
}

static void tcpip_rx_next(TCPIP* tcpip)
{
    TCPIP_IO io;
    if (tcpip_allocate_io(tcpip, &io) == NULL)
        return;
    fread_async(tcpip->eth, HAL_ETH, 0, io.block, FRAME_MAX_SIZE);
}

void tcpip_tx(TCPIP* tcpip, TCPIP_IO* io)
{
    TCPIP_IO* queue_io;
#if (ETH_DOUBLE_BUFFERING)
    if (++tcpip->tx_count > 2)
#else
    if (++tcpip->tx_count > 1)
#endif
    {
        //add to queue
        array_append(&tcpip->tx_queue);
        queue_io = (TCPIP_IO*)array_at(tcpip->tx_queue, array_size(tcpip->tx_queue));
        queue_io->block = io->block;
        queue_io->buf = io->buf;
        queue_io->size = io->size;
    }
    else
        fwrite_async(tcpip->eth, HAL_ETH, 0, io->block, io->size);
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
    fopen(tcpip->eth, HAL_ETH, 0, conn);
    tcpip->active = true;
    tcpip->seconds = 0;
    timer_start_ms(tcpip->timer, 1000, 0);
}

static inline void tcpip_rx(TCPIP* tcpip, HANDLE block, int size)
{
    TCPIP_IO io;
    if (tcpip->connected)
        tcpip_rx_next(tcpip);
    if (size < 0)
    {
        tcpip_release_block(tcpip, block);
        return;
    }
    io.block = block;
    io.size = (unsigned int)size;
    io.buf = block_open(block);
    if (io.buf == NULL)
        return;
    //forward to MAC
    mac_rx(tcpip, &io);
}

static inline void tcpip_tx_complete(TCPIP* tcpip, HANDLE block)
{
    TCPIP_IO* queue_io;
    tcpip_release_block(tcpip, block);
#if (ETH_DOUBLE_BUFFERING)
    if (--tcpip->tx_count >= 2)
#else
    if (--tcpip->tx_count >= 1)
#endif
    {
        //send next in queue
        queue_io = (TCPIP_IO*)array_at(tcpip->tx_queue, 0);
        fwrite_async(tcpip->eth, HAL_ETH, 0, queue_io->block, queue_io->size);
        array_remove(&tcpip->tx_queue, 0);
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
    tcpip->blocks_allocated = 0;
#if (ETH_DOUBLE_BUFFERING)
    //2 rx + 2 tx + 1 for processing
    array_create(&tcpip->free_blocks, sizeof(void*), 5);
#else
    //1 rx + 1 tx + 1 for processing
    array_create(&tcpip->free_blocks, sizeof(void*), 3);
#endif
    array_create(&tcpip->tx_queue, sizeof(TCPIP_IO), 1);
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
    case IPC_READ:
        tcpip_rx(tcpip, ipc->param2, (int)ipc->param3);
        break;
    case IPC_WRITE:
        tcpip_tx_complete(tcpip, ipc->param2);
        break;
    case IPC_TIMEOUT:
        tcpip_timer(tcpip);
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
        error(ERROR_OK);
        need_post = false;
        ipc_read_ms(&ipc, 0, ANY_HANDLE);
        if (ipc.cmd == HAL_CMD(HAL_SYSTEM, IPC_PING))
            need_post = true;
        else
            switch (HAL_GROUP(ipc.cmd))
            {
            case HAL_ETH:
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
