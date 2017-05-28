/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef WEB_H
#define WEB_H

#include "process.h"
#include "ipc.h"
#include "io.h"

typedef enum {
    WEBS_GET = IPC_USER,
    WEBS_HEAD,
    WEBS_POST,
    WEBS_PUT,
    WEBS_DELETE,
    WEBS_CONNECT,
    WEBS_OPTIONS,
    WEBS_TRACE,
    WEBS_CREATE_NODE,
    WEBS_DESTROY_NODE,
    WEBS_REGISTER_RESPONSE,
    WEBS_UNREGISTER_RESPONSE,
    WEBS_GET_PARAM,
    WEBS_SET_PARAM,
    WEBS_GET_URL
} WEBS_IPCS;

typedef enum {
    WEB_METHOD_GET = 0,
    WEB_METHOD_HEAD,
    WEB_METHOD_POST,
    WEB_METHOD_PUT,
    WEB_METHOD_DELETE,
    WEB_METHOD_CONNECT,
    WEB_METHOD_OPTIONS,
    WEB_METHOD_TRACE
} WEB_METHOD;

#define WEB_FLAG(method)           (1 << (method))

#define WEB_GENERIC_ERROR           0
#define WEB_ROOT_NODE                INVALID_HANDLE
#define WEB_OBJ_WILDCARD            "*"

typedef enum {
    HTTP_CONTENT_PLAIN_TEXT = 0,
    HTTP_CONTENT_HTML,
    HTTP_CONTENT_FORM,
    HTTP_CONTENT_NONE,
    HTTP_CONTENT_UNKNOWN
} HTTP_CONTENT_TYPE;

typedef enum {
    HTTP_ENCODING_NONE = 0,
    HTTP_ENCODING_GZIP,
    HTTP_ENCODING_LZW,
    HTTP_ENCODING_ZLIB,
} HTTP_ENCODING_TYPE;

typedef enum {
    WEB_RESPONSE_CONTINUE = 100,
    WEB_RESPONSE_SWITCHING_PROTOCOLS = 101,
    WEB_RESPONSE_OK = 200,
    WEB_RESPONSE_CREATED = 201,
    WEB_RESPONSE_ACCEPTED = 202,
    WEB_RESPONSE_NON_AUTHORITATIVE_INFORMATION = 203,
    WEB_RESPONSE_NO_CONTENT = 204,
    WEB_RESPONSE_RESET_CONTENT = 205,
    WEB_RESPONSE_PARTIAL_CONTENT = 206,
    WEB_RESPONSE_MULTIPLE_CHOICES = 300,
    WEB_RESPONSE_MOVED_PERMANENTLY = 301,
    WEB_RESPONSE_FOUND = 302,
    WEB_RESPONSE_SEE_OTHER = 303,
    WEB_RESPONSE_NOT_MODIFIED = 304,
    WEB_RESPONSE_USE_PROXY = 305,
    WEB_RESPONSE_TEMPORARY_REDIRECT = 307,
    WEB_RESPONSE_BAD_REQUEST = 400,
    WEB_RESPONSE_UNATHORIZED = 401,
    WEB_RESPONSE_PAYMENT_REQUIRED = 402,
    WEB_RESPONSE_FORBIDDEN = 403,
    WEB_RESPONSE_NOT_FOUND = 404,
    WEB_RESPONSE_METHOD_NOT_ALLOWED = 405,
    WEB_RESPONSE_NOT_ACCEPTABLE = 406,
    WEB_RESPONSE_PROXY_AUTHENTICATION_REQUIRED = 407,
    WEB_RESPONSE_REQUEST_TIMEOUT = 408,
    WEB_RESPONSE_CONFLICT = 409,
    WEB_RESPONSE_GONE = 410,
    WEB_RESPONSE_LENGTH_REQUIRED = 411,
    WEB_RESPONSE_PRECONDITION_FAILED = 412,
    WEB_RESPONSE_PAYLOAD_TOO_LARGE = 413,
    WEB_RESPONSE_URI_TOO_LONG = 414,
    WEB_RESPONSE_UNSUPPORTED_MEDIA_TYPE = 415,
    WEB_RESPONSE_RANGE_NOT_SATISFIABLE = 416,
    WEB_RESPONSE_EXPECTATION_FAILED = 417,
    WEB_RESPONSE_UPGRADE_REQUIRED = 426,
    WEB_RESPONSE_INTERNAL_SERVER_ERROR = 500,
    WEB_RESPONSE_NOT_IMPLEMENTED = 501,
    WEB_RESPONSE_BAD_GATEWAY = 502,
    WEB_RESPONSE_SERVICE_UNAVAILABLE = 503,
    WEB_RESPONSE_GATEWAY_TIMEOUT = 504,
    WEB_RESPONSE_HTTP_VERSION_NOT_SUPPORTED = 505
} WEB_RESPONSE;

typedef struct {
    unsigned int processed, content_size;
    HTTP_CONTENT_TYPE content_type;
    HTTP_ENCODING_TYPE encoding_type;
    HANDLE obj;
} HS_STACK;

HANDLE web_server_create(unsigned int process_size, unsigned int priority);
bool web_server_open(HANDLE web_server, uint16_t port, HANDLE tcpip);
void web_server_close(HANDLE web_server);
HANDLE web_server_create_node(HANDLE web_server, HANDLE parent, const char* name, unsigned int flags);
void web_server_destroy_node(HANDLE web_server, HANDLE obj);
//html must be located in flash
void web_server_register_error(HANDLE web_server, WEB_RESPONSE code, const char *html);
void web_server_unregister_error(HANDLE web_server, WEB_RESPONSE code);

void web_server_read(HANDLE web_server, HANDLE session, IO* io, unsigned int size_max);
int web_server_read_sync(HANDLE web_server, HANDLE session, IO* io, unsigned int size_max);
void web_server_write(HANDLE web_server, HANDLE session, WEB_RESPONSE code,  IO* io);
int web_server_write_sync(HANDLE web_server, HANDLE session, WEB_RESPONSE code,  IO* io);
char* web_server_get_param(HANDLE web_server, HANDLE session, IO* io, unsigned int size_max, char* param);
void web_server_set_param(HANDLE web_server, HANDLE session, IO* io, unsigned int size_max, const char* param, const char* value);
char* web_server_get_url(HANDLE web_server, HANDLE session, IO* io, unsigned int size_max);

#endif // WEB_H
