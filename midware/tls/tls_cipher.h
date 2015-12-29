/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TLS_CIPHER_H
#define TLS_CIPHER_H

#include <stdbool.h>

bool tls_decode_master(const void* premaster, const void* client_random, const void* server_random, void* out);

#endif // TLS_CIPHER_H
