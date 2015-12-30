/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "tls_cipher.h"
#include "../../userspace/tls.h"
#include "../../userspace/endian.h"
#include "../crypto/pkcs.h"
#include <string.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t client_mac[20];
    uint8_t server_mac[20];
    unsigned char client_key[16];
    unsigned char server_key[16];
    //IV is not used for block cyphering
} TLS_AES_SHA1_KEY_BLOCK;

typedef struct {
    uint8_t seq_hi_be[4];
    uint8_t seq_lo_be[4];
    TLS_RECORD record;
} TLS_HMAC_HEADER;
#pragma pack(pop)

#define MASTER_LABEL_LEN                                        13
static const uint8_t __MASTER_LABEL[MASTER_LABEL_LEN] =         "master secret";
#define KEY_BLOCK_LABEL_LEN                                     13
static const uint8_t __KEY_BLOCK_LABEL[KEY_BLOCK_LABEL_LEN ] =  "key expansion";
#define CLIENT_LABEL_LEN                                        15
static const uint8_t __CLIENT_LABEL[CLIENT_LABEL_LEN ] =        "client finished";

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

void tls_cipher_init(TLS_CIPHER* tls_cipher)
{
    sha256_init(&tls_cipher->handshake_hash);
}

void tls_cipher_hash_handshake(TLS_CIPHER* tls_cipher, const void* data, unsigned int len)
{
    sha256_update(&tls_cipher->handshake_hash, data, len);
}

bool tls_cipher_compare_client_finished(TLS_CIPHER* tls_cipher, const void* data, unsigned int len)
{
    uint8_t dig[12];
    uint8_t hash[SHA256_BLOCK_SIZE];
    //TODO:
    sha256_final(&tls_cipher->handshake_hash, hash);
    p_hash(tls_cipher->master, TLS_MASTER_SIZE, __CLIENT_LABEL, CLIENT_LABEL_LEN, hash, SHA256_BLOCK_SIZE, NULL, 0, dig, 12);
    dump(dig, 12);
    return true;
}

bool tls_cipher_decode_key_block(const void* premaster, TLS_CIPHER *tls_cipher)
{
    TLS_AES_SHA1_KEY_BLOCK raw;

    //decode pkcs padding
    if (eme_pkcs1_v1_15_decode(premaster, TLS_RAW_PREMASTER_SIZE, tls_cipher->master, TLS_PREMASTER_SIZE) < sizeof(TLS_PREMASTER_SIZE))
        return false;
    //decode master from premaster
    p_hash(tls_cipher->master, TLS_PREMASTER_SIZE, __MASTER_LABEL, MASTER_LABEL_LEN,
                               tls_cipher->client_random, TLS_RANDOM_SIZE,
                               tls_cipher->server_random, TLS_RANDOM_SIZE,
                               tls_cipher->master, TLS_MASTER_SIZE);

    //genarate raw key block. Server here goes first
    p_hash(tls_cipher->master, TLS_MASTER_SIZE, __KEY_BLOCK_LABEL, KEY_BLOCK_LABEL_LEN,
                                tls_cipher->server_random, TLS_RANDOM_SIZE,
                                tls_cipher->client_random, TLS_RANDOM_SIZE,
                                &raw, sizeof(TLS_AES_SHA1_KEY_BLOCK));

    hmac_setup(&tls_cipher->rx_hmac_ctx, &__HMAC_SHA1, &tls_cipher->rx_hash_ctx, raw.client_mac, SHA1_BLOCK_SIZE);
    hmac_setup(&tls_cipher->tx_hmac_ctx, &__HMAC_SHA1, &tls_cipher->tx_hash_ctx, raw.server_mac, SHA1_BLOCK_SIZE);
    AES_set_decrypt_key(raw.client_key, 128, &tls_cipher->rx_key);
    AES_set_encrypt_key(raw.server_key, 128, &tls_cipher->tx_key);
    tls_cipher->rx_sequence = tls_cipher->tx_sequence = 0;
    tls_cipher->iv_size = AES_BLOCK_SIZE;
    tls_cipher->hash_size = SHA1_BLOCK_SIZE;
    return true;
}

int tls_cipher_decrypt(TLS_CIPHER* tls_cipher, TLS_CONTENT_TYPE content_type, void* in, unsigned int len)
{
    int raw_len, m_len;
    TLS_HMAC_HEADER hdr;
    uint8_t mac[tls_cipher->hash_size];
    void* data = (uint8_t*)in + tls_cipher->iv_size;
    raw_len = len - tls_cipher->iv_size;
    if ((len <= tls_cipher->iv_size) || (len % tls_cipher->iv_size))
        return TLS_DECRYPT_FAILED;
    AES_cbc_encrypt(data, data, raw_len, &tls_cipher->rx_key, in, AES_DECRYPT);
    m_len = pkcs7_decode(data, raw_len);
    if (m_len <= 0)
        return TLS_DECRYPT_FAILED;
    //padding byte itself
    if (((uint8_t*)data)[m_len - 1] != raw_len - m_len)
        return TLS_DECRYPT_FAILED;
    --m_len;
    if (m_len < tls_cipher->hash_size)
        return TLS_DECRYPT_FAILED;
    m_len -= tls_cipher->hash_size;

    //check MAC
    int2be(hdr.seq_hi_be, 0);
    int2be(hdr.seq_lo_be, tls_cipher->rx_sequence++);
    hdr.record.content_type = content_type;
    hdr.record.version.major = 3;
    hdr.record.version.minor = 3;
    short2be(hdr.record.record_length_be, m_len);
    hmac_init(&tls_cipher->rx_hmac_ctx);
    hmac_update(&tls_cipher->rx_hmac_ctx, &hdr, sizeof(TLS_HMAC_HEADER));
    hmac_update(&tls_cipher->rx_hmac_ctx, data, m_len);
    hmac_final(&tls_cipher->rx_hmac_ctx, mac);
    if (memcmp((uint8_t*)data + m_len, mac, tls_cipher->hash_size))
        return TLS_MAC_FAILED;
    return m_len;
}
