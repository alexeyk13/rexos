/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "rndis.h"
#include "usb.h"

void rndis_set_link(HANDLE usbd, unsigned int iface, ETH_CONN_TYPE conn)
{
    ack(usbd, HAL_REQ(HAL_USBD_IFACE, RNDIS_SET_LINK), USBD_IFACE(iface, 0), (unsigned int)conn, 0);
}

void rndis_set_host_mac(HANDLE usbd, unsigned int iface, const MAC* mac)
{
    ack(usbd, HAL_REQ(HAL_USBD_IFACE, RNDIS_SET_HOST_MAC), USBD_IFACE(iface, 0), mac->u32.hi, mac->u32.lo);
}

void rndis_get_host_mac(HANDLE usbd, unsigned int iface, MAC* mac)
{
    IPC ipc;
    ipc.cmd = HAL_REQ(HAL_USBD_IFACE, RNDIS_GET_HOST_MAC);
    ipc.process = usbd;
    ipc.param1 = USBD_IFACE(iface, 0);
    call(&ipc);
    mac->u32.hi = ipc.param2;
    mac->u32.lo = ipc.param3;
}
