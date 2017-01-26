/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef HMAC_H
#define HMAC_H

#include <stdint.h>

#define HMAC64_BLOCK_SIZE                              64
#define HMAC64_ROUNDS                                  (64 >> 2)
#define HMAC128_BLOCK_SIZE                             128
#define HMAC128_ROUNDS                                 (128 >> 2)

typedef void (*HASH_INIT)(void*);
typedef void (*HASH_UPDATE)(void*, const void*, unsigned int);
typedef void (*HASH_FINAL)(void*, void*);

typedef struct {
    HASH_INIT hash_init;
    HASH_UPDATE hash_update;
    HASH_FINAL hash_final;
    unsigned short digest_size;
} HMAC_HASH_STRUCT;

typedef struct {
    void* hash_ctx;
    const HMAC_HASH_STRUCT* hash_struct;
    uint32_t ipad[HMAC64_ROUNDS];
    uint32_t opad[HMAC64_ROUNDS];
    //doesn't storing key itself for memory saving
} HMAC_CTX;

void hmac_setup(HMAC_CTX* ctx, const HMAC_HASH_STRUCT* hash_struct, void* hash_ctx, const void *key, unsigned int key_size);
void hmac_init(HMAC_CTX* ctx);
void hmac_update(HMAC_CTX* ctx, const void *data, unsigned int size);
void hmac_final(HMAC_CTX* ctx, void* hmac);

#endif //HMAC_H
