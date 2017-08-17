/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
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
    uint8_t client_random[TLS_RANDOM_SIZE];
    uint8_t server_random[TLS_RANDOM_SIZE];
    uint8_t master[TLS_MASTER_SIZE];
    unsigned short key_size, block_size;
    unsigned short max_data_size;
    AES_KEY rx_key;
    AES_KEY tx_key;
    SHA256_CTX handshake_hash;
    unsigned int rx_sequence_lo, tx_sequence_lo, rx_sequence_hi, tx_sequence_hi;
    uint8_t iv_seed[TLS_IV_SEED_SIZE];

    //HMAC based
    HMAC_CTX rx_hmac_ctx;
    HMAC_CTX tx_hmac_ctx;
    void *rx_hash_ctx, *tx_hash_ctx;
    const HMAC_HASH_STRUCT* hash_struct;
    unsigned short hash_size, hash_ctx_size;
} TLS_CIPHER;

typedef enum {
    TLS_CLIENT_FINISHED,
    TLS_SERVER_FINISHED
} TLS_FINISHED_MODE;

bool tls_cipher_decode_key_hash_cipher(uint16_t cipher_suite, TLS_KEY_EXCHANGE_TYPE* key_exchange, TLS_CIPHER_TYPE* cipher, TLS_HASH_TYPE* hash);
void tls_cipher_init(TLS_CIPHER* tls_cipher);
bool tls_cipher_create(TLS_CIPHER* tls_cipher, uint16_t cipher_suite);
void tls_cipher_destroy(TLS_CIPHER* tls_cipher);
void tls_cipher_hash_handshake(TLS_CIPHER* tls_cipher, const void* data, unsigned int len);
void tls_cipher_generate_finished(TLS_CIPHER* tls_cipher, TLS_FINISHED_MODE mode, void* out);
bool tls_cipher_compare_finished(TLS_CIPHER* tls_cipher, TLS_FINISHED_MODE mode, const void* data);

bool tls_cipher_decode_key_block(const void* premaster, TLS_CIPHER *tls_cipher);

int tls_cipher_decrypt(TLS_CIPHER* tls_cipher, TLS_CONTENT_TYPE content_type, void* in, unsigned int len);
unsigned int tls_cipher_encrypt(TLS_CIPHER* tls_cipher, TLS_CONTENT_TYPE content_type, void* in, unsigned int len);

#endif // TLS_CIPHER_H
