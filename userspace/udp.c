/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "udp.h"

HANDLE udp_listen(HANDLE tcpip, unsigned short port)
{
    return get_handle(tcpip, HAL_REQ(HAL_UDP, IPC_OPEN), port, LOCALHOST, 0);
}
