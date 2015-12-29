/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TLS_CIPHER_H
#define TLS_CIPHER_H

#include <stdbool.h>
#include <stdint.h>
#include "../crypto/aes.h"
#include "../crypto/sha1.h"
#include "../crypto/hmac.h"

typedef struct {
    HMAC_CTX client_hmac_ctx;
    HMAC_CTX server_hmac_ctx;
    SHA1_CTX client_hash_ctx;
    SHA1_CTX server_hash_ctx;
    AES_KEY client_key;
    AES_KEY server_key;
    unsigned long client_sequence, server_sequence;
} TLS_KEY_BLOCK;

bool tls_decode_master(const void* premaster, const void* client_random, const void* server_random, void* out);
void tls_decode_key_block(const void* master, const void* client_random, const void* server_random, TLS_KEY_BLOCK* key_block);

int tls_decrypt(TLS_KEY_BLOCK* key_block, void* in, unsigned int len, void* out);

#endif // TLS_CIPHER_H
