/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "hmac.h"
#include <string.h>

#define IPAD                        0x36363636
#define OPAD                        0x5c5c5c5c

void hmac_setup(HMAC_CTX* ctx, const HMAC_HASH_STRUCT* hash_struct, void* hash_ctx, const void* key, unsigned int key_size)
{
    int i;
    memset(ctx->ipad, 0x00, HMAC64_BLOCK_SIZE);
    ctx->hash_struct = hash_struct;
    ctx->hash_ctx = hash_ctx;
    if (key_size <= HMAC64_BLOCK_SIZE)
        memcpy(ctx->ipad, key, key_size);
    else
    {
        ctx->hash_struct->hash_init(ctx->hash_ctx);
        ctx->hash_struct->hash_update(ctx->hash_ctx, key, key_size);
        ctx->hash_struct->hash_final(ctx->hash_ctx, ctx->ipad);
    }
    for (i = 0; i < HMAC64_ROUNDS; ++i)
    {
        ctx->opad[i] = ctx->ipad[i] ^ OPAD;
        ctx->ipad[i] ^= IPAD;
    }
}

void hmac_init(HMAC_CTX* ctx)
{
    ctx->hash_struct->hash_init(ctx->hash_ctx);
    ctx->hash_struct->hash_update(ctx->hash_ctx, ctx->ipad, HMAC64_BLOCK_SIZE);
}

void hmac_update(HMAC_CTX* ctx, const void* data, unsigned int size)
{
    ctx->hash_struct->hash_update(ctx->hash_ctx, data, size);
}

void hmac_final(HMAC_CTX* ctx, void* hmac)
{
    //hmac here used as temporal storage to save stack space
    ctx->hash_struct->hash_final(ctx->hash_ctx, hmac);

    ctx->hash_struct->hash_init(ctx->hash_ctx);
    ctx->hash_struct->hash_update(ctx->hash_ctx, ctx->opad, HMAC64_BLOCK_SIZE);
    ctx->hash_struct->hash_update(ctx->hash_ctx, hmac, ctx->hash_struct->digest_size);
    ctx->hash_struct->hash_final(ctx->hash_ctx, hmac);
}
