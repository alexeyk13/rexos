/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2018, Alexey Kramarenko
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#include "ccid.h"

void ccid_give_buffer(HANDLE usbd, unsigned int iface, unsigned int slot, IO* io)
{
    io->data_size = 0;
    io_read(usbd, HAL_IO_REQ(HAL_USBD_IFACE, IPC_READ), USBD_IFACE(iface, slot), io, io_get_free(io));
}

void ccid_respond(HANDLE usbd, unsigned int cmd, unsigned int iface, unsigned int slot, IO* io)
{
    io_write(usbd, HAL_IO_CMD(HAL_USBD_IFACE, cmd), USBD_IFACE(iface, slot), io);
}

void ccid_respond_ex(HANDLE usbd, unsigned int cmd, unsigned int iface, unsigned int slot, IO* io, int param)
{
    io_complete_ex(usbd, HAL_IO_CMD(HAL_USBD_IFACE, cmd), USBD_IFACE(iface, slot), io, param);
}

