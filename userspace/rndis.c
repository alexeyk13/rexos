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
