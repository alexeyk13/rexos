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

#if (SYS_INFO) || (TCPIP_DEBUG)
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

HANDLE tcpip_allocate_block(TCPIP* tcpip)
{
    HANDLE res = INVALID_HANDLE;
    if (void_array_size(tcpip->free_blocks))
    {
        res = (HANDLE)void_array_data(tcpip->free_blocks)[void_array_size(tcpip->free_blocks) - 1];
        void_array_remove(&tcpip->free_blocks, void_array_size(tcpip->free_blocks) - 1, 1);
    }
    else if (tcpip->blocks_allocated < TCPIP_MAX_FRAMES_COUNT)
    {
        res = block_create(FRAME_MAX_SIZE);
        if (res != INVALID_HANDLE)
            ++tcpip->blocks_allocated;
#if (TCPIP_DEBUG_ERRORS)
        else
            printf("TCPIP: out of memory\n\r");
#endif
    }
#if (TCPIP_DEBUG_ERRORS)
    else
        printf("TCPIP: too many blocks\n\r");
#endif
    return res;
}

void tcpip_release_block(TCPIP* tcpip, HANDLE block)
{
    void_array_add(&tcpip->free_blocks, 1);
    void_array_data(tcpip->free_blocks)[void_array_size(tcpip->free_blocks) - 1] = (void*)block;
}

static void tcpip_rx_next_block(TCPIP* tcpip)
{
    HANDLE block;
    block = tcpip_allocate_block(tcpip);
    if (block == INVALID_HANDLE)
        return;
    fread_async(tcpip->eth, HAL_HANDLE(HAL_ETH, 0), block, FRAME_MAX_SIZE);
}

void tcpip_tx(TCPIP* tcpip, HANDLE block, unsigned int size)
{
    //TODO: queue
    fwrite_async(tcpip->eth, HAL_HANDLE(HAL_ETH, 0), block, size);
}

static inline void tcpip_open(TCPIP* tcpip, ETH_CONN_TYPE conn)
{
    if (tcpip->active)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    fopen(tcpip->eth, HAL_HANDLE(HAL_ETH, 0), conn);
    tcpip->active = true;
}

static inline void tcpip_rx(TCPIP* tcpip, HANDLE block, int size)
{
    if (tcpip->connected)
        tcpip_rx_next_block(tcpip);
    if (size < 0)
    {
        tcpip_release_block(tcpip, block);
        return;
    }
    uint8_t* buf = block_open(block);
    if (buf == NULL)
        return;
    //forward to MAC
    tcpip_mac_rx(tcpip, buf, size, block);
}

static inline void tcpip_tx_complete(TCPIP* tcpip, HANDLE block)
{
    tcpip_release_block(tcpip, block);
    //TODO: send next in queue (if connected), flush if not
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
        tcpip_rx_next_block(tcpip);
#if (ETH_DOUBLE_BUFFERING)
        tcpip_rx_next_block(tcpip);
#endif
    }
}

#if (SYS_INFO)
static inline void tcpip_info(TCPIP* tcpip)
{
    print_conn_status(tcpip, "ETH connect status");
    mac_info(tcpip);
}
#endif

void tcpip_init(TCPIP* tcpip)
{
    tcpip->eth = object_get(SYS_OBJ_ETH);
    tcpip->conn = ETH_NO_LINK;
    tcpip->connected = tcpip->active = false;
    tcpip->blocks_allocated = 0;
#if (ETH_DOUBLE_BUFFERING)
    //2 rx + 2 tx + 1 for processing
    void_array_create(&tcpip->free_blocks, 5);
#else
    //1 rx + 1 tx + 1 for processing
    void_array_create(&tcpip->free_blocks, 3);
#endif
    tcpip_mac_init(tcpip);
    arp_init(tcpip);
    tcpip_ip_init(tcpip);
}

static inline bool tcpip_request(TCPIP* tcpip, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
#if (SYS_INFO)
    case IPC_GET_INFO:
        tcpip_info(tcpip);
        need_post = true;
        break;
#endif
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
    case ETH_NOTIFY_LINK_CHANGED:
        tcpip_link_changed(tcpip, ipc->param2);
        break;
    default:
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
#if (SYS_INFO) || (TCPIP_DEBUG)
    open_stdout();
#endif
    object_set_self(SYS_OBJ_TCPIP);
    for (;;)
    {
        error(ERROR_OK);
        need_post = false;
        ipc_read_ms(&ipc, 0, ANY_HANDLE);
        if (ipc.cmd == IPC_PING)
            need_post = true;
        else
            switch (ipc.cmd < IPC_USER ? HAL_GROUP(ipc.param1) : HAL_IPC_GROUP(ipc.cmd))
            {
            case HAL_ETH:
            case HAL_TCPIP:
                need_post = tcpip_request(&tcpip, &ipc);
                break;
            case HAL_TCPIP_IP:
                need_post = tcpip_ip_request(&tcpip, &ipc);
                break;
            default:
                break;
            }
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}
