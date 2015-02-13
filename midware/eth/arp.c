/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "arp.h"
#include "../../userspace/ipc.h"
#include "../../userspace/sys.h"
#include "../../userspace/object.h"
#include "../../userspace/eth.h"
#include "../../userspace/stdio.h"
#include "../../userspace/file.h"
#include "sys_config.h"

typedef struct {
    HANDLE eth;
    ETH_CONN_TYPE conn;
    uint8_t own_mac[MAC_SIZE];
} ARP;

void arp_main();

const REX __ARP = {
    //name
    "ARP",
    //size
    //TODO: variable in sys_config
    512,
    //priority - midware priority
    //TODO: variable in sys_config
    150,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    //TODO: variable in sys_config
    10,
    //function
    arp_main
};

#if (SYS_INFO) || (ARP_DEBUG)
static void print_conn_status(ARP* arp, const char* head)
{
    printf("%s: ", head);
    switch (arp->conn)
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
        printf("No link detected");
        break;
    default:
        printf("remote fault");
        break;
    }
    printf("\n\r");
}
#endif

static inline void arp_open(ARP* arp, ETH_CONN_TYPE conn)
{
    fopen(arp->eth, HAL_HANDLE(HAL_ETH, 0), conn);

}

static inline void arp_link_changed(ARP* arp, ETH_CONN_TYPE conn)
{
    arp->conn = conn;
    //TODO: if conn is connected, read packets
#if (ARP_DEBUG)
    print_conn_status(arp, "ETH link changed");
#endif
}

#if (SYS_INFO)
static inline void arp_info(ARP* arp)
{
    print_conn_status(arp, "ETH connect status");
    printf("MAC: ");
    int i;
    for (i = 0; i < 6; ++i)
    {
        printf("%X", arp->own_mac[i]);
        if (i < 5)
            printf(":");
    }
    printf("\n\r");
}
#endif

void arp_init(ARP* arp)
{
    arp->eth = object_get(SYS_OBJ_ETH);
    arp->conn = ETH_NO_LINK;
    eth_get_mac(arp->own_mac);
}

bool arp_request(ARP* arp, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
#if (SYS_INFO)
    case IPC_GET_INFO:
        arp_info(arp);
        need_post = true;
        break;
#endif
    case IPC_OPEN:
        arp_open(arp, ipc->param2);
        need_post = true;
        break;
    case IPC_CLOSE:
        //TODO:
        need_post = true;
        break;
    case ETH_NOTIFY_LINK_CHANGED:
        arp_link_changed(arp, ipc->param2);
        break;
    default:
        break;
    }
    return need_post;
}

void arp_main()
{
    IPC ipc;
    ARP arp;
    bool need_post;
    arp_init(&arp);
#if (SYS_INFO) || (ARP_DEBUG)
    open_stdout();
#endif
    //TODO: maybe self object?
//    object_set_self(SYS_OBJ_ARP);
    for (;;)
    {
        error(ERROR_OK);
        need_post = false;
        ipc_read_ms(&ipc, 0, ANY_HANDLE);
        switch (ipc.cmd)
        {
        case IPC_PING:
            need_post = true;
            break;
        default:
            need_post = arp_request(&arp, &ipc);
            break;
        }
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}
