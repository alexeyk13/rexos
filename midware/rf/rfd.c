/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

/* RF Driver */

#include "../../userspace/sys.h"
#include "../../userspace/stdio.h"
#include "../../userspace/stdlib.h"
#include "rfd.h"

static inline void get_device_mac(uint8_t* mac)
{
    uint32_t mac_hi_part = NRF_FICR->DEVICEADDR[0];
    uint32_t mac_lo_part = NRF_FICR->DEVICEADDR[1];
    mac[0] = (mac_hi_part >>  0) & 0x000000FF;
    mac[1] = (mac_hi_part >>  8) & 0x000000FF;
    mac[2] = (mac_hi_part >> 16) & 0x000000FF;
    mac[3] = (mac_hi_part >> 24) & 0x000000FF;
    mac[4] = (mac_lo_part >>  0) & 0x000000FF;
    mac[5] = (mac_lo_part >>  8) & 0x000000FF;
}

static inline void rfd_open(RFD* rfd, RADIO_MODE mode)
{
    rfd->mode = mode;
    get_device_mac(rfd->mac_addr);

#if (RADIO_DEBUG_INFO)
    printf("Radio mode: %d\n", mode);
    printf("Device MAC addr: ");
    for(uint8_t i = 0; i < 6; i++)
        printf("%02X ", rfd->mac_addr[i]);
    printf("\n");
#endif // RADIO_DEBUG_INFO

    ipc_post_exo(HAL_CMD(HAL_RF, IPC_OPEN), mode, 0, 0);
}

static inline void rfd_read()
{

}

static inline void rfd_write()
{

}

static inline void rfd_command(RFD* rfd, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
        case IPC_OPEN:
            rfd_open(rfd, ipc->param1);
            break;
        case IPC_READ:
            rfd_read();
            break;
        case IPC_WRITE:
            rfd_write();
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
            break;
    }
}

static inline void rfd_init(RFD* rfd)
{
    // initialize values
}

void radio_main()
{
    RFD rfd;
    IPC ipc;

#if (RADIO_DEBUG)
    open_stdout();
#endif // RADIO_DEBUG

    rfd_init(&rfd);

    for (;;)
    {
        ipc_read(&ipc);
        switch (HAL_GROUP(ipc.cmd))
        {
            case HAL_RF:
                rfd_command(&rfd, &ipc);
                break;
            case HAL_BLUETOOTH:
                break;
            default:
                break;
        }
        ipc_write(&ipc);
    }
}
