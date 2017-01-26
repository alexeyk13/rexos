/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TLS_H
#define TLS_H

#include "process.h"
#include "types.h"
#include "ipc.h"

#define TLS_RANDOM_SIZE                                                 32
#define TLS_RAW_PREMASTER_SIZE                                          128
#define TLS_PREMASTER_SIZE                                              48
#define TLS_MASTER_SIZE                                                 48
#define TLS_SESSION_ID_SIZE                                             32
#define TLS_IV_SEED_SIZE                                                32

typedef enum {
    TLS_REGISTER_CERTIFICATE = IPC_USER,
    TLS_GENERATE_RANDOM,
    TLS_PREMASTER_DECRYPT
} TLS_IPCS;

HANDLE tls_create();
bool tls_open(HANDLE tls, HANDLE tcpip);
void tls_close(HANDLE tls);
void tls_register_cerificate(HANDLE tls, const uint8_t* const cert, unsigned int len);


#endif // TLS_H
