/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "rndis.h"
#include "usb.h"
#include "io.h"
#include <string.h>

void rndis_set_link(HANDLE usbd, unsigned int iface, ETH_CONN_TYPE conn)
{
    ack(usbd, HAL_REQ(HAL_USBD_IFACE, RNDIS_SET_LINK), USBD_IFACE(iface, 0), (unsigned int)conn, 0);
}

void rndis_set_vendor_id(HANDLE usbd, unsigned int iface, uint8_t a, uint8_t b, uint8_t c)
{
    ack(usbd, HAL_REQ(HAL_USBD_IFACE, RNDIS_SET_VENDOR_ID), USBD_IFACE(iface, 0), (a << 16) | (b << 8) | c, 0);
}

void rndis_set_vendor_description(HANDLE usbd, unsigned int iface, const char* vendor)
{
    IO* io;
    unsigned int size = strlen(vendor) + 1;
    io = io_create(size);
    strcpy(io_data(io), vendor);
    io->data_size = size;
    io_write_sync(usbd, HAL_IO_REQ(HAL_USBD_IFACE, RNDIS_SET_VENDOR_DESCRIPTION), USBD_IFACE(iface, 0), io);
    io_destroy(io);
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
