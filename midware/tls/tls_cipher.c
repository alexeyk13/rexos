/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "tls_cipher.h"
#include "tls_private.h"
#include "../../userspace/tls.h"
#include "../crypto/sha256.h"
#include "../crypto/hmac.h"
#include "../crypto/pkcs.h"
#include <string.h>

#define MASTER_LABEL_LEN                                        13
static const uint8_t __MASTER_LABEL[MASTER_LABEL_LEN] =         "master secret";

static void p_hash(const void* key, unsigned int key_len, const void* seed1, unsigned int seed1_size,
                                                          const void* seed2, unsigned int seed2_size,
                                                          const void* seed3, unsigned int seed3_size,
                                                          void* out, unsigned int size)
{
    uint8_t a[SHA256_BLOCK_SIZE];
    SHA256_CTX sha256_ctx;
    HMAC_CTX hmac_ctx;
    unsigned int out_len;

    hmac_setup(&hmac_ctx, &__HMAC_SHA256, &sha256_ctx, key, key_len);

    //A(0)
    hmac_init(&hmac_ctx);
    hmac_update(&hmac_ctx, seed1, seed1_size);
    hmac_update(&hmac_ctx, seed2, seed2_size);
    hmac_update(&hmac_ctx, seed3, seed3_size);
    hmac_final(&hmac_ctx, a);

    for (out_len = 0; out_len < size; out_len += SHA256_BLOCK_SIZE)
    {
        hmac_init(&hmac_ctx);
        hmac_update(&hmac_ctx, a, SHA256_BLOCK_SIZE);
        hmac_update(&hmac_ctx, seed1, seed1_size);
        hmac_update(&hmac_ctx, seed2, seed2_size);
        hmac_update(&hmac_ctx, seed3, seed3_size);
        if (size - out_len >= SHA256_BLOCK_SIZE)
        {
            hmac_final(&hmac_ctx, (uint8_t*)out + out_len);
            //A(i) = h(A(i - 1))
            if (size - out_len > SHA256_BLOCK_SIZE)
            {
                hmac_init(&hmac_ctx);
                hmac_update(&hmac_ctx, a, SHA256_BLOCK_SIZE);
                hmac_final(&hmac_ctx, a);
            }
        }
        else
        {
            hmac_final(&hmac_ctx, a);
            memcpy((uint8_t*)out + out_len, a, size - out_len);
        }
    }
    memset(&sha256_ctx, 0x00, sizeof(SHA256_CTX));
    memset(&hmac_ctx, 0x00, sizeof(HMAC_CTX));
}

bool tls_decode_master(const void* premaster, const void* client_random, const void* server_random, void* out)
{
    if (eme_pkcs1_v1_15_decode(premaster, TLS_PREMASTER_SIZE, out, sizeof(TLS_PREMASTER)) < sizeof(TLS_PREMASTER))
        return false;
    p_hash(premaster, sizeof(TLS_PREMASTER), __MASTER_LABEL, MASTER_LABEL_LEN, client_random, 32, server_random, 32, out, TLS_MASTER_SECRET_SIZE);
    return true;
}

