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
#include "../crypto/sha256.h"
#include "../crypto/hmac.h"
#include "tls_private.h"

#define TLS_MAC_FAILED                                  -21
#define TLS_DECRYPT_FAILED                              -20

typedef struct {
    unsigned short iv_size;
    unsigned short hash_size;
    HMAC_CTX rx_hmac_ctx;
    HMAC_CTX tx_hmac_ctx;
    SHA1_CTX rx_hash_ctx;
    SHA1_CTX tx_hash_ctx;
    AES_KEY rx_key;
    AES_KEY tx_key;
    SHA256_CTX handshake_hash;
    unsigned long rx_sequence, tx_sequence;
} TLS_CIPHER;

void tls_cipher_init(TLS_CIPHER* tls_cipher);
void tls_cipher_hash_handshake(TLS_CIPHER* tls_cipher, const void* data, unsigned int len);
bool tls_cipher_compare_client_finished(void* master, TLS_CIPHER* tls_cipher, const void* data, unsigned int len);

bool tls_cipher_decode_master(const void* premaster, const void* client_random, const void* server_random, void* out);
void tls_cipher_decode_tls_cipher(const void* master, const void* client_random, const void* server_random, TLS_CIPHER* tls_cipher);

int tls_cipher_decrypt(TLS_CIPHER* tls_cipher, TLS_CONTENT_TYPE content_type, void* in, unsigned int len);

#endif // TLS_CIPHER_H
