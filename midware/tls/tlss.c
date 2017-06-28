/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "tls_private.h"
#include "tls_cipher.h"
#include "sys_config.h"
#include "../../userspace/tls.h"
#include "../../userspace/process.h"
#include "../../userspace/stdio.h"
#include "../../userspace/sys.h"
#include "../../userspace/io.h"
#include "../../userspace/so.h"
#include "../../userspace/tcp.h"
#include "../../userspace/endian.h"
#include "../crypto/aes.h"
#include <string.h>

void tlss_main();

typedef enum {
    TLSS_STATE_CLIENT_HELLO = 0,
    TLSS_STATE_GENERATE_SERVER_RANDOM,
    TLSS_STATE_GENERATE_SESSION_ID,
    TLSS_STATE_SERVER_HELLO,
    TLSS_STATE_CLIENT_KEY_EXCHANGE,
    TLSS_STATE_DECRYPT_PREMASTER,
    TLSS_STATE_CLIENT_CHANGE_CIPHER_SPEC,
    TLSS_STATE_GENERATE_IV_SEED,
    TLSS_STATE_SERVER_CHANGE_CIPHER_SPEC,
    TLSS_STATE_READY,
    TLSS_STATE_PENDING,
    TLSS_STATE_CLOSE_NOTIFY,
    TLSS_STATE_CLOSING,
    TLSS_STATE_MAX
} TLSS_STATE;

typedef struct {
    HANDLE handle;
    IO* rx;
    IO* tx;
#if (TLS_DEBUG_REQUESTS)
    IP remote_addr;
#endif //TLS_DEBUG_REQUESTS
    unsigned int rx_size, tx_offset;
    TLS_PROTOCOL_VERSION version;
    TLS_CIPHER tls_cipher;
    uint8_t session_id[TLS_SESSION_ID_SIZE];
    TLSS_STATE state;
    uint16_t cipher_suite;
    bool server_secure, client_secure;
} TLSS_TCB;

typedef struct {
    HANDLE tcpip, user, owner, tcb_handle;
    uint8_t* cert;
    void* pending_data;
    unsigned int cert_len;
    unsigned int offset, pending_len;
    IO* rx;
    IO* tx;
    SO tcbs;
    bool rx_busy, tx_busy;
} TLSS;

const REX __TLSS = {
    //name
    "TLS Server",
    //size
    TLS_PROCESS_SIZE,
    //priority - midware priority
    TLS_PROCESS_PRIORITY,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_FLAG_PERSISTENT_NAME,
    //function
    tlss_main
};

#if (TLS_DEBUG_REQUESTS)
static const char* const __TLSS_STATES[TLSS_STATE_MAX] =           {"CLIENT_HELLO",
                                                                    "GENERATE_SERVER_RANDOM",
                                                                    "GENERATE_SESSION_ID",
                                                                    "SERVER_HELLO",
                                                                    "CLIENT_KEY_EXCHANGE",
                                                                    "DECRYPT_PREMASTER",
                                                                    "CLIENT_CHANGE_CIPHER_SPEC",
                                                                    "GENERATE_IV_SEED",
                                                                    "SERVER_CHANGE_CIPHER_SPEC",
                                                                    "READY",
                                                                    "PENDING",
                                                                    "CLOSE_NOTIFY",
                                                                    "CLOSING"};

static const char* const __TLS_key_exchange[] =                     {"NULL",
                                                                    "RSA",
                                                                    "DH_DSS",
                                                                    "DH_RSA",
                                                                    "DHE_DSS",
                                                                    "DHE_RSA",
                                                                    "DH_anon",
                                                                    "KRB5",
                                                                    "PSK",
                                                                    "DHE_PSK",
                                                                    "RSA_PSK",
                                                                    "ECDH_ECDSA",
                                                                    "ECDHE_ECDSA",
                                                                    "ECDH_RSA",
                                                                    "ECDHE_RSA",
                                                                    "ECDH_anon",
                                                                    "SRP_SHA",
                                                                    "SRP_SHA_RSA",
                                                                    "SRP_SHA_DSS",
                                                                    "ECDHE_PSK"};


static const char* const __TLS_CIPHER[] =                          {"NULL",
                                                                    "RC4_40",
                                                                    "RC4_128",
                                                                    "RC2_CBC_40",
                                                                    "IDEA_CBC",
                                                                    "DES40_CBC",
                                                                    "DES_CBC_40",
                                                                    "DES_CBC",
                                                                    "3DES_EDE_CBC",
                                                                    "SEED_CBC",
                                                                    "AES_128_CBC",
                                                                    "AES_256_CBC",
                                                                    "AES_128_GCM",
                                                                    "AES_256_GCM",
                                                                    "AES_128_CCM",
                                                                    "AES_256_CCM",
                                                                    "AES_128_CCM_8",
                                                                    "AES_256_CCM_8",
                                                                    "CAMELIA_128_CBC",
                                                                    "CAMELIA_256_CBC",
                                                                    "CAMELIA_128_GCM",
                                                                    "CAMELIA_256_GCM",
                                                                    "ARIA_128_CBC",
                                                                    "ARIA_256_CBC",
                                                                    "ARIA_128_GCM",
                                                                    "ARIA_256_GCM"};

static const char* const __TLS_HASH[] =                            {"NULL",
                                                                    "MD5",
                                                                    "SHA",
                                                                    "SHA256",
                                                                    "SHA384"};

#if (TLS_DEBUG_SECRETS) || (TLS_DEBUG_REQUESTS)
static void tlss_dump(void* data, unsigned int len)
{
    int i;
    for (i = 0; i < len; ++i)
    {
        if (i % 16)
            printf(" ");
        printf("%02x", ((uint8_t*)data)[i]);
        if (((i % 16) == (16 - 1)) || (i +1 == len))
            printf("\n");
    }
}
#endif //(TLS_DEBUG_SECRETS) || (TLS_DEBUG_REQUESTS)

#if (TLS_DEBUG_REQUESTS)
static void tlss_print_cipher_suite(uint16_t cipher_suite)
{
    TLS_KEY_EXCHANGE_TYPE key_exchange;
    TLS_CIPHER_TYPE cipher;
    TLS_HASH_TYPE hash;
    bool exported;

    switch (cipher_suite)
    {
    case TLS_EMPTY_RENEGOTIATION_INFO_SCSV:
        printf("TLS_EMPTY_RENEGOTIATION_INFO_SCSV\n");
        return;
    case TLS_FALLBACK_SCSV:
        printf("TLS_FALLBACK_SCSV\n");
        return;
    default:
        break;
    }

    switch (cipher_suite)
    {
    case TLS_RSA_EXPORT_WITH_RC4_40_MD5:
    case TLS_RSA_EXPORT_WITH_RC2_CBC_40_MD5:
    case TLS_RSA_EXPORT_WITH_DES40_CBC_SHA:
    case TLS_DH_DSS_EXPORT_WITH_DES40_CBC_SHA:
    case TLS_DH_RSA_EXPORT_WITH_DES40_CBC_SHA:
    case TLS_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA:
    case TLS_DHE_RSA_EXPORT_WITH_DES40_CBC_SHA:
    case TLS_DH_anon_EXPORT_WITH_RC4_40_MD5:
    case TLS_DH_anon_EXPORT_WITH_DES40_CBC_SHA:
    case TLS_KRB5_EXPORT_WITH_DES_CBC_40_SHA:
    case TLS_KRB5_EXPORT_WITH_RC2_CBC_40_SHA:
    case TLS_KRB5_EXPORT_WITH_RC4_40_SHA:
    case TLS_KRB5_EXPORT_WITH_DES_CBC_40_MD5:
    case TLS_KRB5_EXPORT_WITH_RC2_CBC_40_MD5:
    case TLS_KRB5_EXPORT_WITH_RC4_40_MD5:
        exported = true;
        break;
    default:
        exported = false;
    }

    tls_cipher_decode_key_hash_cipher(cipher_suite, &key_exchange, &cipher, &hash);

    if (key_exchange != TLS_KEY_EXCHANGE_UNKNOWN)
    {
        printf("TLS_%s_", __TLS_key_exchange[key_exchange]);
        if (exported)
            printf("EXPORT_");
        printf("WITH_%s", __TLS_CIPHER[cipher]);
        if (hash != TLS_HASH_NIL)
            printf("_%s", __TLS_HASH[hash]);
        printf("\n");
    }
    else
        printf("{%#02X, %#02X}\n", (cipher_suite >> 8) & 0xff, cipher_suite & 0xff);
}
#endif //(TLS_DEBUG_REQUESTS)

static void tlss_print_compression_method(uint8_t compression_method)
{
    switch (compression_method)
    {
    case TLS_COMPRESSION_NULL:
        printf("NULL\n");
        break;
    case TLS_COMPRESSION_DEFLATE:
        printf("DEFLATE\n");
        break;
    case TLS_COMPRESSION_LZS:
        printf("LZS\n");
        break;
    default:
        printf("%#02X\n", compression_method);
    }
}
#endif //TLS_DEBUG_REQUESTS

static void tlss_set_state(TLSS_TCB* tcb, TLSS_STATE new_state)
{
#if (TLS_DEBUG_REQUESTS)
    printf("TLS: %s -> %s\n", __TLSS_STATES[tcb->state], __TLSS_STATES[new_state]);
#endif //TLS_DEBUG_REQUESTS
    tcb->state = new_state;
}

static HANDLE tlss_create_tcb(TLSS* tlss, HANDLE handle)
{
    TLSS_TCB* tcb;
    HANDLE tcb_handle;
    //TODO: multiple sessions support
    if (so_count(&tlss->tcbs))
        return INVALID_HANDLE;
    tcb_handle = so_allocate(&tlss->tcbs);
    if (tcb_handle == INVALID_HANDLE)
        return INVALID_HANDLE;
    tcb = so_get(&tlss->tcbs, tcb_handle);
    tls_cipher_init(&tcb->tls_cipher);
    memset(tcb, 0x00, sizeof(TLSS_TCB));
    tcb->handle = handle;
    tcb->state = TLSS_STATE_CLIENT_HELLO;
    tcb->version = TLS_PROTOCOL_VERSION_UNSUPPORTED;
    tcb->cipher_suite = TLS_NULL_WITH_NULL_NULL;
    tcb->tls_cipher.max_data_size = TLS_IO_SIZE - sizeof(TLS_RECORD);
    return tcb_handle;
}

static void tlss_destroy_tcb(TLSS* tlss, HANDLE tcb_handle)
{
    TLSS_TCB* tcb = so_get(&tlss->tcbs, tcb_handle);
#if (TLS_DEBUG_REQUESTS)
    printf("TLS: %s -> 0\n", __TLSS_STATES[tcb->state]);
#endif //TLS_DEBUG_REQUESTS
    tls_cipher_destroy(&tcb->tls_cipher);
    memset(tcb, 0x00, sizeof(TLSS_TCB));
    so_free(&tlss->tcbs, tcb_handle);
}

static HANDLE tlss_find_tcb_handle(TLSS* tlss, HANDLE handle)
{
    TLSS_TCB* tcb;
    HANDLE tcb_handle = INVALID_HANDLE;
    for (tcb_handle = so_first(&tlss->tcbs); tcb_handle != INVALID_HANDLE; tcb_handle = so_next(&tlss->tcbs, tcb_handle))
    {
        tcb = so_get(&tlss->tcbs, tcb_handle);
        if (tcb->handle == handle)
            return tcb_handle;
    }
    return tcb_handle;
}

static void tlss_flush(TLSS* tlss, HANDLE tcb_handle)
{
    TLSS_TCB* tcb = so_get(&tlss->tcbs, tcb_handle);
    if (tcb->rx != NULL)
    {
        io_complete_ex(tlss->user, HAL_IO_CMD(HAL_TCP, IPC_READ), tlss->tcb_handle, tcb->rx, ERROR_IO_CANCELLED);
        tcb->rx = NULL;
    }
    if (tcb->tx != NULL)
    {
        io_complete_ex(tlss->user, HAL_IO_CMD(HAL_TCP, IPC_WRITE), tlss->tcb_handle, tcb->tx, ERROR_IO_CANCELLED);
        tcb->tx = NULL;
    }
    if (tcb->state == TLSS_STATE_PENDING)
        tcb->state = TLSS_STATE_READY;
    //active session
    if (tlss->tcb_handle == tcb_handle)
        tlss->tcb_handle = INVALID_HANDLE;
    if (tcb->state == TLSS_STATE_READY)
        tcp_flush(tlss->tcpip, tcb->handle);
}

static void tlss_close_session(TLSS* tlss, HANDLE tcb_handle, bool close_tcp)
{
    TLSS_TCB* tcb = so_get(&tlss->tcbs, tcb_handle);
    tlss_flush(tlss, tcb_handle);
    if (close_tcp)
        tcp_close(tlss->tcpip, tcb->handle);
    //inform user on connection close if established before
    if (tcb->server_secure)
        ipc_post_inline(tlss->user, HAL_CMD(HAL_TCP, IPC_CLOSE), tcb_handle, 0, 0);
    tlss_destroy_tcb(tlss, tcb_handle);
}

static void tlss_tcp_rx(TLSS* tlss)
{
    HANDLE tcb_handle;
    TLSS_TCB* tcb;
    //already reading (wakeup by tx complete, user request, etc)
    if (tlss->rx_busy)
        return;
    //TODO: refactor to listen handles for multiple sessions support
    tlss->offset = 0;
    if ((tcb_handle = so_first(&tlss->tcbs)) != INVALID_HANDLE)
    {
        tcb = so_get(&tlss->tcbs, tcb_handle);
        tlss->rx_busy = true;
        tcp_read(tlss->tcpip, tcb->handle, tlss->rx, TLS_IO_SIZE);
    }
}

static void tlss_tcp_tx(TLSS* tlss, TLSS_TCB* tcb)
{
    TCP_STACK* stack;
    tlss->tx_busy = true;
    stack = io_push(tlss->tx, sizeof(TCP_STACK));
    stack->flags = TCP_PSH;
    tcp_write(tlss->tcpip, tcb->handle, tlss->tx);
}

static inline void tlss_connection_established(TLSS* tlss, TLSS_TCB* tcb)
{
    ipc_post_inline(tlss->user, HAL_CMD(HAL_TCP, IPC_OPEN), tlss->tcb_handle, tlss->tcb_handle, 0);
}

static unsigned int tlss_get_size(TLS_SIZE* tls_size)
{
    unsigned int res = (unsigned int)(tls_size->size_hi) << 16;
    res += (unsigned int)(tls_size->size_lo_be[0]) << 8;
    res += (unsigned int)(tls_size->size_lo_be[1]) << 0;
    return res;
}

static void tlss_set_size(TLS_SIZE* tls_size, unsigned int value)
{
    tls_size->size_hi = (value >> 16) & 0xff;
    tls_size->size_lo_be[0] = (value >> 8) & 0xff;
    tls_size->size_lo_be[1] = (value >> 0) & 0xff;
}

static void* tlss_allocate_record(TLSS* tlss, TLSS_TCB* tcb, TLS_CONTENT_TYPE content_type)
{
    TLS_RECORD* rec = (TLS_RECORD*)((uint8_t*)io_data(tlss->tx) + tlss->tx->data_size);
    rec->content_type = content_type;
    rec->version.major = 3;
    rec->version.minor = (uint8_t)tcb->version;
    short2be(rec->record_length_be, 0);
    return (uint8_t*)io_data(tlss->tx) + tlss->tx->data_size + sizeof(TLS_RECORD) + (tcb->server_secure ? tcb->tls_cipher.block_size : 0);
}

static void tlss_send_record(TLSS* tlss, TLSS_TCB* tcb, unsigned int len)
{
    TLS_RECORD* rec = (TLS_RECORD*)((uint8_t*)io_data(tlss->tx) + tlss->tx->data_size);

    if (tcb->server_secure)
        len = tls_cipher_encrypt(&tcb->tls_cipher, rec->content_type, (uint8_t*)io_data(tlss->tx) + tlss->tx->data_size + sizeof(TLS_RECORD), len);
    //Update full record len
    short2be(rec->record_length_be, len);
    tlss->tx->data_size += len + sizeof(TLS_RECORD);
}

static void tlss_user_tx(TLSS* tlss, HANDLE tcb_handle, TLSS_TCB* tcb)
{
    unsigned int to_write;
    void* data = tlss_allocate_record(tlss, tcb, TLS_CONTENT_APP);

    to_write = tcb->tx->data_size - tcb->tx_offset;
    if (to_write > tcb->tls_cipher.max_data_size)
        to_write = tcb->tls_cipher.max_data_size;

    memcpy(data, (uint8_t*)io_data(tcb->tx) + tcb->tx_offset, to_write);
    tcb->tx_offset += to_write;
    tlss_send_record(tlss, tcb, to_write);
    tlss_tcp_tx(tlss, tcb);
    if (tcb->tx_offset >= tcb->tx->data_size)
    {
        io_complete(tlss->user, HAL_IO_CMD(HAL_TCP, IPC_WRITE), tcb_handle, tcb->tx);
        tcb->tx = NULL;
    }
}

static unsigned int tlss_append_server_hello(TLSS* tlss, TLSS_TCB* tcb, void* data)
{
    TLS_HANDSHAKE* handshake;
    TLS_HELLO* hello;
    TLS_EXTENSION* ext;
    unsigned int len;

    handshake = data;
    len = sizeof(TLS_HANDSHAKE);
    handshake->message_type = TLS_HANDSHAKE_SERVER_HELLO;
    tlss_set_size(&handshake->message_length_be, 0);

    //1. Append generic header
    hello = (TLS_HELLO*)((uint8_t*)data + len);
    len += sizeof(TLS_HELLO);
    hello->version.major = 3;
    hello->version.minor = (uint8_t)tcb->version;
    memcpy(&hello->random, tcb->tls_cipher.server_random, TLS_RANDOM_SIZE);
    //session id
    hello->session_id_length = TLS_SESSION_ID_SIZE;

    //2. Append session id, nothing to append
    memcpy((uint8_t*)data + len, tcb->session_id, TLS_SESSION_ID_SIZE);
    len += TLS_SESSION_ID_SIZE;

    //3. Append cipher suite
    short2be((uint8_t*)data + len, tcb->cipher_suite);
    len += 2;
    //4. append compression method
    *((uint8_t*)data + len) = TLS_COMPRESSION_NULL;
    ++len;

    //5. Append renegotiation_info extension to make openSSL happy
    short2be((uint8_t*)data + len, sizeof(TLS_EXTENSION) + 1);
    len += 2;

    ext = (TLS_EXTENSION*)((uint8_t*)data + len);
    len += sizeof(TLS_EXTENSION);
    short2be(ext->code_be, TLS_EXTENSION_RENEGOTIATION_INFO);
    short2be(ext->len_be, 1);
    *((uint8_t*)data + len) = 0x00;
    ++len;

    //6. Update len at end
    tlss_set_size(&handshake->message_length_be, len - sizeof(TLS_HANDSHAKE));
    tls_cipher_hash_handshake(&tcb->tls_cipher, data, len);

#if (TLS_DEBUG_REQUESTS)
    printf("TLS: serverHello\n");
    printf("Protocol version: 1.%d\n", hello->version.minor - 1);
#if (TLS_DEBUG_SECRETS)
    printf("Server random:\n");
    tlss_dump(hello->random, TLS_RANDOM_SIZE);
#endif //TLS_DEBUG_SECRETS
    printf("Session ID:\n");
    tlss_dump(tcb->session_id, TLS_SESSION_ID_SIZE);
    printf("cipher suite: ");
    tlss_print_cipher_suite(TLS_RSA_WITH_AES_128_CBC_SHA);
    printf("Compression method: NULL\n");
    printf("Extensions:\n");
    printf("Ext 65281: 00\n");
#endif //TLS_DEBUG_REQUESTS
    return len;
}

static unsigned int tlss_append_certificate(TLSS* tlss, TLSS_TCB* tcb, void* data)
{
    TLS_HANDSHAKE* handshake;
    unsigned int len;
    //1. Handshake record
    handshake = data;
    handshake->message_type = TLS_HANDSHAKE_CERTIFICATE;
    tlss_set_size(&handshake->message_length_be, 0);
    len = sizeof(TLS_HANDSHAKE);

    //2. Certificate list
    tlss_set_size((TLS_SIZE*)((uint8_t*)data + len), sizeof(TLS_SIZE) + tlss->cert_len);
    len += sizeof(TLS_SIZE);

    //3. Certificate list
    tlss_set_size((TLS_SIZE*)((uint8_t*)data + len), tlss->cert_len);
    len += sizeof(TLS_SIZE);

    //4. Certificate itself
    memcpy((uint8_t*)data + len, tlss->cert, tlss->cert_len);
    len += tlss->cert_len;
    tlss_set_size(&handshake->message_length_be, len - sizeof(TLS_HANDSHAKE));
    tls_cipher_hash_handshake(&tcb->tls_cipher, data, len);
#if (TLS_DEBUG_REQUESTS)
    printf("TLS: certificate\n");
#endif //TLS_DEBUG_REQUESTS
    return len;
}

static unsigned int tlss_append_server_hello_done(TLSS* tlss, TLSS_TCB* tcb, void* data)
{
    TLS_HANDSHAKE* handshake;
    handshake = data;
    handshake->message_type = TLS_HANDSHAKE_SERVER_HELLO_DONE;
    tlss_set_size(&handshake->message_length_be, 0);
    tls_cipher_hash_handshake(&tcb->tls_cipher, data, sizeof(TLS_HANDSHAKE));
#if (TLS_DEBUG_REQUESTS)
    printf("TLS: serverHelloDone\n");
#endif //TLS_DEBUG_REQUESTS
    return sizeof(TLS_HANDSHAKE);
}

static inline void tlss_tx_server_hello(TLSS* tlss, TLSS_TCB* tcb)
{
    void* data;
    unsigned int len = 0;
    data = tlss_allocate_record(tlss, tcb, TLS_CONTENT_HANDSHAKE);
    len += tlss_append_server_hello(tlss, tcb, (uint8_t*)data + len);
    len += tlss_append_certificate(tlss, tcb, (uint8_t*)data + len);
    len += tlss_append_server_hello_done(tlss, tcb, (uint8_t*)data + len);
    tlss_send_record(tlss, tcb, len);
    tlss_set_state(tcb, TLSS_STATE_CLIENT_KEY_EXCHANGE);
    tlss_tcp_tx(tlss, tcb);
}

static unsigned int tlss_append_server_change_cipher_spec(TLSS* tlss, TLSS_TCB* tcb, void* data)
{
    *((uint8_t*)data) = TLS_CHANGE_CIPHER_SPEC;
#if (TLS_DEBUG_REQUESTS)
    printf("TLS: (server) changeCipherSpec\n");
#endif //TLS_DEBUG_REQUESTS
    return 1;
}

static unsigned int tlss_append_server_finished(TLSS* tlss, TLSS_TCB* tcb, void* data)
{
    TLS_HANDSHAKE* handshake;
    unsigned int len = 0;

    handshake = (TLS_HANDSHAKE*)data;
    len += sizeof(TLS_HANDSHAKE);
    handshake->message_type = TLS_HANDSHAKE_FINISHED;
    tlss_set_size(&handshake->message_length_be, TLS_FINISHED_DIGEST_SIZE);
    tls_cipher_generate_finished(&tcb->tls_cipher, TLS_SERVER_FINISHED, (uint8_t*)data + len);
    len += TLS_FINISHED_DIGEST_SIZE;

    //this message itself not included in hash
    tls_cipher_hash_handshake(&tcb->tls_cipher, data, sizeof(TLS_HANDSHAKE));
#if (TLS_DEBUG_REQUESTS)
    printf("TLS: (server) finished\n");
#endif //TLS_DEBUG_REQUESTS
    return len;
}

static inline void tlss_tx_server_change_cipher_spec(TLSS* tlss, TLSS_TCB* tcb)
{
    void* data;
    unsigned int len = 0;
    data = tlss_allocate_record(tlss, tcb, TLS_CONTENT_CHANGE_CIPHER);
    len += tlss_append_server_change_cipher_spec(tlss, tcb, (uint8_t*)data + len);
    tlss_send_record(tlss, tcb, len);
    //set after, not encrypt change cipher itself
    tcb->server_secure = true;

    len = 0;
    data = tlss_allocate_record(tlss, tcb, TLS_CONTENT_HANDSHAKE);
    len += tlss_append_server_finished(tlss, tcb, (uint8_t*)data + len);
    tlss_send_record(tlss, tcb, len);

    tlss_set_state(tcb, TLSS_STATE_READY);
    tlss_tcp_tx(tlss, tcb);

    tlss_connection_established(tlss, tcb);
}

static void tlss_tx_alert(TLSS* tlss, TLSS_TCB* tcb, TLS_ALERT_LEVEL alert_level, TLS_ALERT_DESCRIPTION alert_description)
{
    TLS_ALERT* alert = tlss_allocate_record(tlss, tcb, TLS_CONTENT_ALERT);
    alert->alert_level = alert_level;
    alert->alert_description = alert_description;
#if (TLS_DEBUG_REQUESTS)
    if (alert_description == TLS_ALERT_CLOSE_NOTIFY)
        printf("TLS: tx close notify\n");
    else
        printf("TLS: tx %s alert: %d\n", alert->alert_level == TLS_ALERT_LEVEL_WARNING ? "warning" : "fatal", alert->alert_description);
#endif //TLS_DEBUG_REQUEST
    tlss_send_record(tlss, tcb, sizeof(TLS_ALERT));
    tlss_tcp_tx(tlss, tcb);
}

static void tlss_fatal(TLSS* tlss, TLSS_TCB* tcb, TLS_ALERT_DESCRIPTION alert_description)
{
    tcb->state = TLSS_STATE_CLOSING;
    tlss_tx_alert(tlss, tcb, TLS_ALERT_LEVEL_FATAL, alert_description);
}

static inline void tlss_rx_change_cipher(TLSS* tlss, TLSS_TCB* tcb, void* data, unsigned int len)
{
    if ((tcb->state != TLSS_STATE_CLIENT_CHANGE_CIPHER_SPEC) || (len != 1) || (*((uint8_t*)data) != TLS_CHANGE_CIPHER_SPEC) || (tcb->client_secure))
    {
        tlss_fatal(tlss, tcb, TLS_ALERT_UNEXPECTED_MESSAGE);
        return;
    }
    tcb->client_secure = true;
#if (TLS_DEBUG_REQUESTS)
    printf("TLS: (client) changeCipherSpec\n");
#endif //TLS_DEBUG_REQUESTS
}

static inline void tlss_rx_alert(TLSS* tlss, TLSS_TCB* tcb, void* data, unsigned int len)
{
    TLS_ALERT* alert = data;
    if (len < sizeof(TLS_ALERT))
    {
        tlss_fatal(tlss, tcb, TLS_ALERT_UNEXPECTED_MESSAGE);
        return;
    }
    if (alert->alert_description == TLS_ALERT_CLOSE_NOTIFY)
    {
#if (TLS_DEBUG_REQUESTS)
        printf("TLS: rx close_notify\n");
#endif //TLS_DEBUG_REQUEST
        //rx answer on our close notify
        if (tcb->state == TLSS_STATE_CLOSE_NOTIFY)
            tlss_close_session(tlss, tlss->tcb_handle, true);
        else
        {
            //tx close notify
            tlss_tx_alert(tlss, tcb, TLS_ALERT_LEVEL_WARNING, TLS_ALERT_CLOSE_NOTIFY);
            tlss_set_state(tcb, TLSS_STATE_CLOSING);
        }
    }
    else
    {
#if (TLS_DEBUG_REQUESTS)
        printf("TLS: rx %s alert: %d\n", alert->alert_level == TLS_ALERT_LEVEL_WARNING ? "warning" : "fatal", alert->alert_description);
#endif //TLS_DEBUG_REQUESTS
        tlss_close_session(tlss, tlss->tcb_handle, true);
    }
}

static inline void tlss_rx_client_hello(TLSS* tlss, TLSS_TCB* tcb, void* data, unsigned int len)
{
    int i;
    unsigned short cipher_suites_len;
    uint8_t compression_len;
    uint16_t tmp;
    uint8_t* cipher_suites;
    uint8_t* compression;
    uint8_t* extensions;
    uint16_t extensions_len;
    TLS_HELLO* hello;
    TLS_EXTENSION* ext;
    hello = data;
    //1. Check state and clientHello header size
    if ((tcb->state != TLSS_STATE_CLIENT_HELLO) || (len < sizeof(TLS_HELLO)))
    {
        tlss_fatal(tlss, tcb, TLS_ALERT_UNEXPECTED_MESSAGE);
        return;
    }
    //2. Check protocol version
    if ((hello->version.major < 3) || (hello->version.minor < 1))
    {
#if (TLS_DEBUG_ERRORS)
        printf("TLS: Protocol version too old\n");
#endif //TLS_DEBUG_REQUESTS
        tlss_fatal(tlss, tcb, TLS_ALERT_PROTOCOL_VERSION);
        return;
    }
    if ((hello->version.major > 3) || (hello->version.minor > 3))
        tcb->version = TLS_PROTOCOL_1_2;
    else if ((hello->version.major < 3) || (hello->version.minor < 3))
    {
#if (TLS_DEBUG_ERRORS)
        printf("TLS: protocol version 1.2 not supported by client, closing\n");
#endif //TLS_DEBUG_ERRORS
        tlss_fatal(tlss, tcb, TLS_ALERT_PROTOCOL_VERSION);
        return;
    }
    else
        tcb->version = TLS_PROTOCOL_1_2;
    //3. Copy random
    memcpy(tcb->tls_cipher.client_random, &hello->random, TLS_RANDOM_SIZE);
    //4. Ignore session, just check size
    data += sizeof(TLS_HELLO);
    len -= sizeof(TLS_HELLO);
    if (len < hello->session_id_length + 2)
    {
        tlss_fatal(tlss, tcb, TLS_ALERT_UNEXPECTED_MESSAGE);
        return;
    }
    data += hello->session_id_length;
    len -= hello->session_id_length;
    //5. Decode cipher suites and apply
    cipher_suites_len = be2short(data);
    data += 2;
    len -= 2;
    //also byte for compression len
    if (len <= cipher_suites_len)
    {
        tlss_fatal(tlss, tcb, TLS_ALERT_UNEXPECTED_MESSAGE);
        return;
    }
    cipher_suites = data;
    data += cipher_suites_len;
    len -= cipher_suites_len;
    for (i = 0; (i < cipher_suites_len) && (tcb->cipher_suite == TLS_NULL_WITH_NULL_NULL); i += 2)
    {
        tmp = be2short(cipher_suites + i);
        switch (tmp)
        {
#if (TLS_RSA_WITH_AES_128_CBC_SHA_CIPHER_SUITE)
        case TLS_RSA_WITH_AES_128_CBC_SHA:
#endif //(TLS_RSA_WITH_AES_128_CBC_SHA_CIPHER_SUITE)
#if (TLS_RSA_WITH_AES_128_CBC_SHA256_CIPHER_SUITE)
        case TLS_RSA_WITH_AES_128_CBC_SHA256:
#endif //(TLS_RSA_WITH_AES_128_CBC_SHA256_CIPHER_SUITE)
            tcb->cipher_suite = tmp;
        default:
            break;
        }
    }
    if (tcb->cipher_suite == TLS_NULL_WITH_NULL_NULL)
    {
#if (TLS_DEBUG_ERRORS)
        printf("TLS: Supported cipher suite not found\n");
#endif //TLS_DEBUG_ERRORS
        tlss_fatal(tlss, tcb, TLS_ALERT_HANDSHAKE_FAILURE);
        return;
    }
    //6. Decode and check compression
    compression_len = *((uint8_t*)data);
    ++data;
    --len;
    //also 2 bytes for extensions len
    if (len < compression_len + 2)
    {
        tlss_fatal(tlss, tcb, TLS_ALERT_UNEXPECTED_MESSAGE);
        return;
    }
    compression = data;
    data += compression_len;
    len -= compression_len;
    tmp = 0xffff;
    for (i = 0; i < compression_len; ++i)
    {
        //no compression supported for now
        if (compression[i] == TLS_COMPRESSION_NULL)
        {
            tmp = compression[i];
            break;
        }
    }
    if (tmp == 0xffff)
    {
#if (TLS_DEBUG_ERRORS)
        printf("TLS: Supported compression method not found\n");
#endif //TLS_DEBUG_ERRORS
        tlss_fatal(tlss, tcb, TLS_ALERT_HANDSHAKE_FAILURE);
        return;
    }

    //7. Process extensions
    extensions_len = be2short(data);
    data += 2;
    len -= 2;
    extensions = data;
    if (len < extensions_len)
    {
        tlss_fatal(tlss, tcb, TLS_ALERT_UNEXPECTED_MESSAGE);
        return;
    }

    for (i = 0; i < extensions_len; i += tmp + sizeof(TLS_EXTENSION))
    {
        if (len < sizeof(TLS_EXTENSION))
        {
            tlss_fatal(tlss, tcb, TLS_ALERT_UNEXPECTED_MESSAGE);
            return;
        }
        ext = (TLS_EXTENSION*)((uint8_t*)extensions + i);
        len -= sizeof(TLS_EXTENSION);
        tmp = be2short(ext->len_be);
        if (len < tmp)
        {
            tlss_fatal(tlss, tcb, TLS_ALERT_UNEXPECTED_MESSAGE);
            return;
        }
        len -= tmp;
    }

    if (!tls_cipher_create(&tcb->tls_cipher, tcb->cipher_suite))
    {
        tlss_fatal(tlss, tcb, TLS_ALERT_INTERNAL_ERROR);
        return;
    }
    tlss_set_state(tcb, TLSS_STATE_GENERATE_SERVER_RANDOM);
#if (TLS_DEBUG_REQUESTS)
    printf("TLS: clientHello\n");
    printf("Protocol version: %d.%d\n", hello->version.major - 2, hello->version.minor - 1);
#if (TLS_DEBUG_SECRETS)
    printf("Client random:\n");
    tlss_dump(hello->random, TLS_RANDOM_SIZE);
#endif //TLS_DEBUG_SECRETS
    printf("Session ID:\n");
    if (hello->session_id_length == 0)
        printf("NULL\n");
    else
        tlss_dump((uint8_t*)hello + sizeof(TLS_HELLO), hello->session_id_length);
    printf("cipher suites:\n");
    for (i = 0; i < cipher_suites_len; i += 2)
    {
        tmp = be2short(cipher_suites + i);
        if (tmp == tcb->cipher_suite)
            printf("*");
        tlss_print_cipher_suite(tmp);
    }
    printf("Compression methods:\n");
    for (i = 0; i < compression_len; ++i)
    {
        if (compression[i] == TLS_COMPRESSION_NULL)
            printf("*");
        tlss_print_compression_method(i);
    }
    printf("Extensions:\n");
    for (i = 0; i < extensions_len; i += tmp + sizeof(TLS_EXTENSION))
    {
        ext = (TLS_EXTENSION*)((uint8_t*)extensions + i);
        tmp = be2short(ext->len_be);
        printf("Ext %d: ", be2short(ext->code_be));
        tlss_dump((uint8_t*)extensions + i + sizeof(TLS_EXTENSION), tmp);
    }
#endif //TLS_DEBUG_REQUESTS
}

static inline void tlss_rx_client_key_exchange(TLSS* tlss, TLSS_TCB* tcb, void* data, unsigned int len)
{
    if ((tcb->state != TLSS_STATE_CLIENT_KEY_EXCHANGE) || (len < TLS_RAW_PREMASTER_SIZE + 2) || (be2short(data) != TLS_RAW_PREMASTER_SIZE))
    {
        tlss_fatal(tlss, tcb, TLS_ALERT_UNEXPECTED_MESSAGE);
        return;
    }
    io_reset(tlss->tx);
    memcpy(io_data(tlss->tx), (uint8_t*)data + 2, TLS_RAW_PREMASTER_SIZE);
    tlss->tx->data_size = TLS_RAW_PREMASTER_SIZE;
    tlss_set_state(tcb, TLSS_STATE_DECRYPT_PREMASTER);
#if (TLS_DEBUG_REQUESTS)
    printf("TLS: clientKeyExchange\n");
#endif //TLS_DEBUG_REQUESTS
}

static inline void tlss_rx_finished(TLSS* tlss, TLSS_TCB* tcb, void* data, unsigned int len)
{
    if ((tcb->state != TLSS_STATE_CLIENT_CHANGE_CIPHER_SPEC) || (len != TLS_FINISHED_DIGEST_SIZE) || (!tcb->client_secure))
    {
        tlss_fatal(tlss, tcb, TLS_ALERT_UNEXPECTED_MESSAGE);
        return;
    }
    if (!tls_cipher_compare_finished(&tcb->tls_cipher, TLS_CLIENT_FINISHED, data))
    {
#if (TLS_DEBUG_ERRORS)
        printf("TLS: (client) finished data mismatch\n");
#endif //TLS_DEBUG_ERRORS
        tlss_fatal(tlss, tcb, TLS_ALERT_HANDSHAKE_FAILURE);
        return;
    }

    tlss_set_state(tcb, TLSS_STATE_GENERATE_IV_SEED);
#if (TLS_DEBUG_REQUESTS)
    printf("TLS: (client) finished\n");
#endif //TLS_DEBUG_REQUESTS
}

static inline void tlss_rx_handshakes(TLSS* tlss, TLSS_TCB* tcb, void* data, unsigned int len)
{
    unsigned short offset, len_cur;
    TLS_HANDSHAKE* handshake;
    void* data_cur;
    //iterate through handshake messages
    for (offset = 0; offset < len; offset += len_cur + sizeof(TLS_HANDSHAKE))
    {
        if (len - offset < sizeof(TLS_HANDSHAKE))
        {
            tlss_fatal(tlss, tcb, TLS_ALERT_UNEXPECTED_MESSAGE);
            return;
        }
        handshake = (TLS_HANDSHAKE*)((uint8_t*)data + offset);
        len_cur = tlss_get_size(&handshake->message_length_be);
        if (len_cur + sizeof(TLS_HANDSHAKE) > len - offset)
        {
            tlss_fatal(tlss, tcb, TLS_ALERT_UNEXPECTED_MESSAGE);
            return;
        }
        data_cur = (uint8_t*)data + offset + sizeof(TLS_HANDSHAKE);
        switch (handshake->message_type)
        {
        case TLS_HANDSHAKE_CLIENT_HELLO:
            tlss_rx_client_hello(tlss, tcb, data_cur, len_cur);
            break;
        case TLS_HANDSHAKE_CLIENT_KEY_EXCHANGE:
            tlss_rx_client_key_exchange(tlss, tcb, data_cur, len_cur);
            break;
        case TLS_HANDSHAKE_FINISHED:
            tlss_rx_finished(tlss, tcb, data_cur, len_cur);
            break;
        default:
#if (TLS_DEBUG_ERRORS)
            printf("TLS: unexpected handshake type: %d\n", handshake->message_type);
#endif //TLS_DEBUG_ERRORS
            tlss_fatal(tlss, tcb, TLS_ALERT_UNEXPECTED_MESSAGE);
            return;
        }
        tls_cipher_hash_handshake(&tcb->tls_cipher, (uint8_t*)data + offset, len_cur + sizeof(TLS_HANDSHAKE));
    }
}

static inline void tlss_rx_app(TLSS* tlss, TLSS_TCB* tcb, void* data, unsigned int len)
{
    TCP_STACK* stack;
    unsigned int to_read;
#if (TLS_DEBUG_REQUESTS)
    printf("TLS: rx app data - %d\n", len);
#endif //TLS_DEBUG_REQUEST
    if (tcb->rx != NULL)
    {
        to_read = len;
        if (to_read > tcb->rx_size)
            to_read = tcb->rx_size;
        memcpy(io_data(tcb->rx), data, to_read);
        tcb->rx->data_size = to_read;
        stack = io_push(tcb->rx, sizeof(TCP_STACK));
        stack->flags = 0;
        data = (uint8_t*)data + to_read;
        len -= to_read;
        io_complete(tlss->user, HAL_IO_CMD(HAL_TCP, IPC_READ), tlss->tcb_handle, tcb->rx);
        tcb->rx = NULL;
    }
    if (len)
    {
        tlss->pending_data = data;
        tlss->pending_len = len;
        tlss_set_state(tcb, TLSS_STATE_PENDING);
    }
}

static inline void tlss_process_user_tx(TLSS* tlss)
{
    HANDLE tcb_handle;
    TLSS_TCB* tcb;
    for (tcb_handle = so_first(&tlss->tcbs); tcb_handle != INVALID_HANDLE; tcb_handle = so_next(&tlss->tcbs, tcb_handle))
    {
        tcb = so_get(&tlss->tcbs, tcb_handle);
        if (tcb->tx != NULL)
        {
            tlss_user_tx(tlss, tcb_handle, tcb);
            return;
        }
    }
}

static inline bool tlss_rx_next(TLSS* tlss, TLSS_TCB* tcb)
{
    TLS_RECORD* rec;
    int len;
    void* data;
    if (tlss->offset >= tlss->rx->data_size)
        tlss->tcb_handle = INVALID_HANDLE;
    if (tlss->tcb_handle == INVALID_HANDLE)
    {
        //read next record(s)
        tlss_tcp_rx(tlss);
        if (!tlss->tx_busy)
            tlss_process_user_tx(tlss);
        return false;
    }

    do {
        //Empty records disabled by TLS
        if ((tlss->rx->data_size - tlss->offset) <= sizeof(TLS_RECORD))
        {
            tlss_fatal(tlss, tcb, TLS_ALERT_UNEXPECTED_MESSAGE);
            break;
        }
        rec = (TLS_RECORD*)((uint8_t*)io_data(tlss->rx) + tlss->offset);
        //check TLS 1.0 - 1.2
        if ((rec->version.major != 3) || (rec->version.minor == 0) || (rec->version.minor > 3))
        {
            tlss_fatal(tlss, tcb, TLS_ALERT_PROTOCOL_VERSION);
            break;
        }
        len = be2short(rec->record_length_be);
        tlss->offset += sizeof(TLS_RECORD);
        if (len > tlss->rx->data_size - tlss->offset)
        {
            tlss_fatal(tlss, tcb, TLS_ALERT_UNEXPECTED_MESSAGE);
            break;
        }
        data = (uint8_t*)io_data(tlss->rx) + tlss->offset;
        tlss->offset += len;
        if (tcb->client_secure)
        {
            len = tls_cipher_decrypt(&tcb->tls_cipher, rec->content_type, data, len);
            data += tcb->tls_cipher.block_size;
            if (len < 0)
            {
                tlss_fatal(tlss, tcb, -len);
#if (TLS_DEBUG_ERRORS)
                if (len == TLS_DECRYPT_FAILED)
                    printf("TLS: Decrypt record failed\n");
                else
                    printf("TLS: Record MAC check failed\n");
#endif //TLS_DEBUG_ERRORS
                break;
            }
        }
        switch (rec->content_type)
        {
        case TLS_CONTENT_CHANGE_CIPHER:
            tlss_rx_change_cipher(tlss, tcb, data, len);
            break;
        case TLS_CONTENT_ALERT:
            tlss_rx_alert(tlss, tcb, data, len);
            break;
        case TLS_CONTENT_HANDSHAKE:
            tlss_rx_handshakes(tlss, tcb, data, len);
            break;
        case TLS_CONTENT_APP:
            tlss_rx_app(tlss, tcb, data, len);
            break;
        default:
#if (TLS_DEBUG_ERRORS)
            printf("TLS: unexpected message type: %d\n", rec->content_type);
#endif //TLS_DEBUG_ERRORS
            tlss_fatal(tlss, tcb, TLS_ALERT_UNEXPECTED_MESSAGE);
        }
    } while (false);
    return true;
}

static void tlss_fsm(TLSS* tlss)
{
    TLSS_TCB* tcb;
    //message after close request
    if (tlss->tcpip == INVALID_HANDLE)
        return;
    //no active handle? rx next
    if (tlss->tcb_handle == INVALID_HANDLE)
    {
        tlss_tcp_rx(tlss);
        return;
    }
    tcb = so_get(&tlss->tcbs, tlss->tcb_handle);

    for (;;)
    {
        //can't process if user sending data on other connection at the same time
        //tcp_tx_complete will recall FSM
        if (tlss->tx_busy)
            return;

        switch (tcb->state)
        {
        case TLSS_STATE_GENERATE_SERVER_RANDOM:
            io_read(tlss->owner, HAL_IO_REQ(HAL_TLS, TLS_GENERATE_RANDOM), tlss->tcb_handle, tlss->tx, TLS_RANDOM_SIZE);
            return;
        case TLSS_STATE_GENERATE_SESSION_ID:
            io_read(tlss->owner, HAL_IO_REQ(HAL_TLS, TLS_GENERATE_RANDOM), tlss->tcb_handle, tlss->tx, TLS_SESSION_ID_SIZE);
            return;
        case TLSS_STATE_GENERATE_IV_SEED:
            io_read(tlss->owner, HAL_IO_REQ(HAL_TLS, TLS_GENERATE_RANDOM), tlss->tcb_handle, tlss->tx, TLS_IV_SEED_SIZE);
            return;
        case TLSS_STATE_SERVER_HELLO:
            tlss_tx_server_hello(tlss, tcb);
            break;
        case TLSS_STATE_DECRYPT_PREMASTER:
            io_write(tlss->owner, HAL_IO_REQ(HAL_TLS, TLS_PREMASTER_DECRYPT), tlss->tcb_handle, tlss->tx);
            return;
        case TLSS_STATE_SERVER_CHANGE_CIPHER_SPEC:
            tlss_tx_server_change_cipher_spec(tlss, tcb);
            break;
        case TLSS_STATE_PENDING:
            return;
        case TLSS_STATE_CLOSE_NOTIFY:
            tlss_close_session(tlss, tlss->tcb_handle, true);
            break;
        default:
            if (!tlss_rx_next(tlss, tcb))
                return;
        }
    }
}

static inline void tlss_init(TLSS* tlss)
{
    tlss->tcpip = INVALID_HANDLE;
    tlss->user = INVALID_HANDLE;
    tlss->owner = INVALID_HANDLE;
    tlss->tcb_handle = INVALID_HANDLE;
    tlss->rx = NULL;
    tlss->tx = NULL;
    tlss->tx_busy = tlss->rx_busy = false;
    tlss->cert = NULL;
    tlss->cert_len = 0;
    tlss->offset = 0;
    //relative time will be set on first clientHello request
    so_create(&tlss->tcbs, sizeof(TLSS_TCB), 1);
}

static inline void tlss_open(TLSS* tlss, HANDLE tcpip, HANDLE owner)
{
    if (tlss->tcpip != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    if (tlss->cert == NULL || tlss->cert_len == 0)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    tlss->rx = io_create(TLS_IO_SIZE + sizeof(TCP_STACK));
    tlss->tx = io_create(TLS_IO_SIZE + sizeof(TCP_STACK));
    tlss->tcpip = tcpip;
    tlss->owner = owner;
}

static inline void tlss_close(TLSS* tlss)
{
    HANDLE tcb_handle;
    if (tlss->tcpip == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    while ((tcb_handle = so_first(&tlss->tcbs)) != INVALID_HANDLE)
        tlss_close_session(tlss, tcb_handle, true);
    tlss->tcpip = INVALID_HANDLE;
    tlss->owner = INVALID_HANDLE;
    io_destroy(tlss->rx);
    io_destroy(tlss->tx);
    tlss->rx = NULL;
    tlss->tx = NULL;
}

static inline void tlss_generate_server_random(TLSS* tlss, TLSS_TCB* tcb, void* random)
{
    memcpy(tcb->tls_cipher.server_random, io_data(tlss->tx), TLS_RANDOM_SIZE);
    tlss_set_state(tcb, TLSS_STATE_GENERATE_SESSION_ID);
}

static inline void tlss_generate_session_id(TLSS* tlss, TLSS_TCB* tcb, void* random)
{
    memcpy(tcb->session_id, io_data(tlss->tx), TLS_SESSION_ID_SIZE);
    tlss_set_state(tcb, TLSS_STATE_SERVER_HELLO);
}

static inline void tlss_generate_iv_seed(TLSS* tlss, TLSS_TCB* tcb, void* random)
{
    memcpy(tcb->tls_cipher.iv_seed, io_data(tlss->tx), TLS_IV_SEED_SIZE);
    tlss_set_state(tcb, TLSS_STATE_SERVER_CHANGE_CIPHER_SPEC);
}

static inline void tlss_generate_random(TLSS* tlss, HANDLE tcb_handle)
{
    TLSS_TCB* tcb;
    do {
        //closed already
        if (tlss->tcb_handle != tcb_handle)
            break;
        tcb = so_get(&tlss->tcbs, tlss->tcb_handle);
        if (tlss->tx->data_size < TLS_RANDOM_SIZE)
            break;
        switch (tcb->state)
        {
        case TLSS_STATE_GENERATE_SERVER_RANDOM:
            tlss_generate_server_random(tlss, tcb, io_data(tlss->tx));
            break;
        case TLSS_STATE_GENERATE_SESSION_ID:
            tlss_generate_session_id(tlss, tcb, io_data(tlss->tx));
            break;
        case TLSS_STATE_GENERATE_IV_SEED:
            tlss_generate_iv_seed(tlss, tcb, io_data(tlss->tx));
            break;
        default:
            break;
        }
    } while (false);
    io_reset(tlss->tx);
    tlss->tx_busy = false;
    tlss_fsm(tlss);
}

static inline void tlss_register_certificate(TLSS* tlss, uint8_t* cert, unsigned int len)
{
    if (tlss->cert != NULL)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    if (cert == NULL || len == 0)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    tlss->cert = cert;
    tlss->cert_len = len;
}

static inline void tlss_premaster_decrypt(TLSS* tlss, HANDLE tcb_handle)
{
    TLSS_TCB* tcb;
    do {
        //closed already
        if (tlss->tcb_handle != tcb_handle)
            break;
        tcb = so_get(&tlss->tcbs, tlss->tcb_handle);
        if (tlss->tx->data_size < sizeof(TLS_RAW_PREMASTER_SIZE))
            break;
        if (!tls_cipher_decode_key_block(io_data(tlss->tx), &tcb->tls_cipher))
        {
            tlss_fatal(tlss, tcb, TLS_ALERT_DECRYPTION_FAILED);
#if (TLS_DEBUG_ERRORS)
            printf("TLS: premaster decryption failed\n");
#endif //TLS_DEBUG_ERRORS
            break;
        }
#if (TLS_DEBUG_SECRETS)
        printf("TLS: master secret:\n");
        tlss_dump(tcb->tls_cipher.master, TLS_MASTER_SIZE);
#endif //TLS_DEBUG_SECRETS
        tlss_set_state(tcb, TLSS_STATE_CLIENT_CHANGE_CIPHER_SPEC);
    } while (false);
    io_reset(tlss->tx);
    tlss->tx_busy = false;
    tlss_fsm(tlss);
}

static inline void tlss_request(TLSS* tlss, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        tlss_open(tlss, (HANDLE)ipc->param2, ipc->process);
        break;
    case IPC_CLOSE:
        tlss_close(tlss);
        break;
    case TLS_GENERATE_RANDOM:
        tlss_generate_random(tlss, (HANDLE)ipc->param1);
        break;
    case TLS_REGISTER_CERTIFICATE:
        tlss_register_certificate(tlss, (uint8_t*)ipc->param2, ipc->param3);
        break;
    case TLS_PREMASTER_DECRYPT:
        tlss_premaster_decrypt(tlss, (HANDLE)ipc->param1);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

static inline void tlss_tcp_open(TLSS* tlss, HANDLE handle)
{
#if (TLS_DEBUG_REQUESTS)
    TLSS_TCB* tcb;
#endif //TLS_DEBUG_REQUESTS
    HANDLE tcb_handle = tlss_create_tcb(tlss, handle);
    if (tcb_handle == INVALID_HANDLE)
    {
        tcp_close(tlss->tcpip, handle);
        return;
    }
#if (TLS_DEBUG_REQUESTS)
    tcb = so_get(&tlss->tcbs, tcb_handle);
    tcp_get_remote_addr(tlss->tcpip, handle, &tcb->remote_addr);
    printf("TLS: new session from ");
    ip_print(&tcb->remote_addr);
    printf("\n");
#endif //TLS_DEBUG_REQUESTS
    tlss_fsm(tlss);
}

static inline void tlss_tcp_close(TLSS* tlss, HANDLE handle)
{
#if (TLS_DEBUG_ERRORS)
    TLSS_TCB* tcb;
#endif //TLS_DEBUG_ERRORS

    HANDLE tcb_handle = tlss_find_tcb_handle(tlss, handle);
    //closed before, normal processing, ignore message
    if (tcb_handle == INVALID_HANDLE)
        return;

#if (TLS_DEBUG_ERRORS)
    tcb = so_get(&tlss->tcbs, tcb_handle);
    //close may arrive earlier than transmit complete
    if (tcb->state != TLSS_STATE_CLOSING)
    {
        printf("TLS: unexpected session close: %d\n ", tcb->state);
        ip_print(&tcb->remote_addr);
        printf("\n");
    }
#endif //TLS_DEBUG_ERRORS
    tlss_close_session(tlss, tcb_handle, false);
    tlss_fsm(tlss);
}

static void tlss_tcp_rx_complete(TLSS* tlss, HANDLE handle, int size)
{
    tlss->rx_busy = false;
    if (size < 0)
    {
        tlss_tcp_rx(tlss);
        return;
    }
    tlss->tcb_handle = tlss_find_tcb_handle(tlss, handle);
    tlss_fsm(tlss);
}

static inline void tlss_tcp_tx_complete(TLSS* tlss, HANDLE handle)
{
    HANDLE tcb_handle;
    TLSS_TCB* tcb;
    io_reset(tlss->tx);
    tlss->tx_busy = false;
    tcb_handle = tlss_find_tcb_handle(tlss, handle);

    //doesn't matter delivered close or closed by other side first
    if (tcb_handle != INVALID_HANDLE)
    {
        tcb = so_get(&tlss->tcbs, tcb_handle);
        if (tcb->state == TLSS_STATE_CLOSING)
            tlss_close_session(tlss, tlss->tcb_handle, true);
    }
    //wakeup if some data write pending
    tlss_fsm(tlss);
}

static inline void tlss_tcp_request(TLSS* tlss, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        tlss_tcp_open(tlss, (HANDLE)ipc->param1);
        break;
    case IPC_CLOSE:
        tlss_tcp_close(tlss, (HANDLE)ipc->param1);
        break;
    case IPC_READ:
        tlss_tcp_rx_complete(tlss, (HANDLE)ipc->param1, (int)ipc->param3);
        break;
    case IPC_WRITE:
        tlss_tcp_tx_complete(tlss, (HANDLE)ipc->param1);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

static inline void tlss_user_listen(TLSS* tlss, IPC* ipc)
{
    //just forward to tcp
    if (tlss->tcpip == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    if (tlss->user != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    tlss->user = ipc->process;
    //just forward to tcp
    ipc->param2 = tcp_listen(tlss->tcpip, ipc->param1);
}

static inline void tlss_user_close_listen(TLSS* tlss, IPC* ipc)
{
    //just forward to tcp
    if (tlss->user == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    tlss->user = INVALID_HANDLE;
    //just forward to tcp
    tcp_close_listen(tlss->tcpip, ipc->param1);
}

static TLSS_TCB* tlss_user_get_tcb(TLSS* tlss, HANDLE tcb_handle)
{
    TLSS_TCB* tcb;
    if (tlss->user == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return NULL;
    }
    if ((tcb = so_get(&tlss->tcbs, tcb_handle)) == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return NULL;
    }
    return tcb;
}

static inline void tlss_user_get_remote_addr(TLSS* tlss, IPC* ipc)
{
    TLSS_TCB* tcb;
    if ((tcb = tlss_user_get_tcb(tlss, ipc->param1)) == NULL)
        return;
    //just forward to tcp
    ipc->param2 = get(tlss->tcpip, HAL_REQ(HAL_TCP, TCP_GET_REMOTE_ADDR), tcb->handle, 0, 0);
}

static inline void tlss_user_get_remote_port(TLSS* tlss, IPC* ipc)
{
    TLSS_TCB* tcb;
    if ((tcb = tlss_user_get_tcb(tlss, ipc->param1)) == NULL)
        return;
    //just forward to tcp
    ipc->param2 = tcp_get_remote_port(tlss->tcpip, tcb->handle);
}

static inline void tlss_user_get_local_port(TLSS* tlss, IPC* ipc)
{
    TLSS_TCB* tcb;
    if ((tcb = tlss_user_get_tcb(tlss, ipc->param1)) == NULL)
        return;
    //just forward to tcp
    ipc->param2 = tcp_get_local_port(tlss->tcpip, tcb->handle);
}

static inline void tlss_user_close(TLSS* tlss, HANDLE tcb_handle)
{
    TLSS_TCB* tcb = so_get(&tlss->tcbs, tcb_handle);
    if (tcb == NULL)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
    tlss_flush(tlss, tcb_handle);
    tlss_set_state(tcb, TLSS_STATE_CLOSE_NOTIFY);
    tlss_tx_alert(tlss, tcb, TLS_ALERT_LEVEL_WARNING, TLS_ALERT_CLOSE_NOTIFY);
    tlss->tcb_handle = tcb_handle;
    error(ERROR_SYNC);
}

static inline void tlss_user_read(TLSS* tlss, HANDLE tcb_handle, IO* io, unsigned int size)
{
    TLSS_TCB* tcb;
    TCP_STACK* stack;
    unsigned int to_read;
    if ((tcb = tlss_user_get_tcb(tlss, tcb_handle)) == NULL)
        return;
    if (tcb->rx != NULL)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    io->data_size = 0;
    if (size > io_get_free(io) - sizeof(TCP_STACK))
        size = io_get_free(io) - sizeof(TCP_STACK);
    if (tcb->state == TLSS_STATE_PENDING)
    {
        to_read = tlss->pending_len;
        if (to_read > size)
            to_read = size;
        memcpy(io_data(io), tlss->pending_data, to_read);
        stack = io_push(io, sizeof(TCP_STACK));
        stack->flags = 0;
        io->data_size = to_read;
        io_complete(tlss->user, HAL_IO_CMD(HAL_TCP, IPC_READ), tcb_handle, io);

        if (to_read == tlss->pending_len)
        {
            tlss_set_state(tcb, TLSS_STATE_READY);
            tlss_fsm(tlss);
        }
        else
        {
            tlss->pending_data = (uint8_t*)tlss->pending_data + to_read;
            tlss->pending_len -= to_read;
        }
    }
    else
    {
        tcb->rx = io;
        tcb->rx_size = size;
    }
    error(ERROR_SYNC);
}

static inline void tlss_user_write(TLSS* tlss, HANDLE tcb_handle, IO* io)
{
    TLSS_TCB* tcb;
    if ((tcb = tlss_user_get_tcb(tlss, tcb_handle)) == NULL)
        return;
    if (tcb->tx != NULL)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    tcb->tx_offset = 0;
    tcb->tx = io;
    if (!tlss->tx_busy)
        tlss_user_tx(tlss, tcb_handle, tcb);
    error(ERROR_SYNC);
}

static inline void tlss_user_flush(TLSS* tlss, HANDLE handle)
{
    HANDLE tcb_handle = tlss_find_tcb_handle(tlss, handle);
    if (tcb_handle == INVALID_HANDLE)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
    tlss_flush(tlss, tcb_handle);
}

void tlss_user_request(TLSS* tlss, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case TCP_LISTEN:
        tlss_user_listen(tlss, ipc);
        break;
    case TCP_CLOSE_LISTEN:
        tlss_user_close_listen(tlss, ipc);
        break;
    case TCP_GET_REMOTE_ADDR:
        tlss_user_get_remote_addr(tlss, ipc);
        break;
    case TCP_GET_REMOTE_PORT:
        tlss_user_get_remote_port(tlss, ipc);
        break;
    case TCP_GET_LOCAL_PORT:
        tlss_user_get_local_port(tlss, ipc);
        break;
    case IPC_CLOSE:
        tlss_user_close(tlss, (HANDLE)ipc->param1);
        break;
    case IPC_READ:
        tlss_user_read(tlss, (HANDLE)ipc->param1, (IO*)ipc->param2, ipc->param3);
        break;
    case IPC_WRITE:
        tlss_user_write(tlss, (HANDLE)ipc->param1, (IO*)ipc->param2);
        break;
    case IPC_FLUSH:
        tlss_user_flush(tlss, (HANDLE)ipc->param1);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

void tlss_main()
{
    IPC ipc;
    TLSS tlss;
    tlss_init(&tlss);
#if (TLS_DEBUG_ERRORS) || (TLS_DEBUG_REQUESTS) || (TLS_DEBUG_SECRETS)
    open_stdout();
#endif //TLS_DEBUG

    for (;;)
    {
        ipc_read(&ipc);
        switch (HAL_GROUP(ipc.cmd))
        {
        case HAL_TLS:
            tlss_request(&tlss, &ipc);
            break;
        case HAL_TCP:
            if (ipc.process == tlss.tcpip)
                tlss_tcp_request(&tlss, &ipc);
            else
                tlss_user_request(&tlss, &ipc);
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
            break;
        }
        ipc_write(&ipc);
    }
}
