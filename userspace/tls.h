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

#define TLS_SERVER_RANDOM_SIZE                                          32

typedef enum {
    TLS_REGISTER_CERTIFICATE = IPC_USER,
    TLS_GENERATE_RANDOM,
    TLS_KEY_DECRYPT
} TLS_IPCS;

HANDLE tls_create();
bool tls_open(HANDLE tls, HANDLE tcpip);
void tls_close(HANDLE tls);


#endif // TLS_H
