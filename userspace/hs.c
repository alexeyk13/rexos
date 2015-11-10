/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "hs.h"
#include "../midware/http/hss.h"

HANDLE hs_create()
{
    return process_create(&__HSS);
}

bool hs_open(HANDLE hs, HANDLE tcpip)
{
    return get_handle(hs, HAL_REQ(HAL_HTTP, IPC_OPEN), 0, tcpip, 0) != INVALID_HANDLE;
}

void hs_close(HANDLE hs)
{
    ack(hs, HAL_REQ(HAL_HTTP, IPC_CLOSE), 0, 0, 0);
}
