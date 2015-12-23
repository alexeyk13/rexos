/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TLS_H
#define TLS_H

#include "process.h"
#include "types.h"
#include "ipc.h"

HANDLE tls_create();
bool tls_open(HANDLE tls, HANDLE tcpip);
void tls_close(HANDLE tls);


#endif // TLS_H
