/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "tls.h"

extern const REX __TLSS;

HANDLE tls_create()
{
    return process_create(&__TLSS);
}

bool tls_open(HANDLE tls, HANDLE tcpip)
{
    return get_handle(tls, HAL_REQ(HAL_TLS, IPC_OPEN), 0, tcpip, 0) != INVALID_HANDLE;
}

void tls_close(HANDLE tls)
{
    ack(tls, HAL_REQ(HAL_TLS, IPC_CLOSE), 0, 0, 0);
}

void tls_register_cerificate(HANDLE tls, const uint8_t* const cert, unsigned int len)
{
    ack(tls, HAL_REQ(HAL_TLS, TLS_REGISTER_CERTIFICATE), 0, (unsigned int)cert, len);
}
