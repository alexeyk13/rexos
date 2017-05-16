/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "web.h"
#include "../midware/http/webs.h"
#include <string.h>

extern const REX __WEBS;

HANDLE web_server_create()
{
    return process_create(&__WEBS);
}

bool web_server_open(HANDLE web_server, uint16_t port, HANDLE tcpip)
{
    return get_handle(web_server, HAL_REQ(HAL_WEBS, IPC_OPEN), port, tcpip, 0) != INVALID_HANDLE;
}

void web_server_close(HANDLE web_server)
{
    ack(web_server, HAL_REQ(HAL_WEBS, IPC_CLOSE), 0, 0, 0);
}

HANDLE web_server_create_node(HANDLE web_server, HANDLE parent, const char* name, unsigned int flags)
{
    HANDLE res;
    IO* io = io_create(strlen(name) + 1);
    if (io == NULL)
        return INVALID_HANDLE;
    strcpy(io_data(io), name);
    if (get_size(web_server, HAL_IO_REQ(HAL_WEBS, WEBS_CREATE_NODE), parent, (unsigned int)io, flags) >= sizeof(HANDLE))
        res = *((HANDLE*)io_data(io));
    else
        res = INVALID_HANDLE;
    io_destroy(io);
    return res;
}

void web_server_destroy_node(HANDLE web_server, HANDLE obj)
{
    ack(web_server, HAL_REQ(HAL_WEBS, WEBS_DESTROY_NODE), obj, 0, 0);
}

void web_server_register_error(HANDLE web_server, WEB_RESPONSE code, const char* html)
{
    ack(web_server, HAL_REQ(HAL_WEBS, WEBS_REGISTER_RESPONSE), (unsigned int)code, (unsigned int)html, 0);
}

void web_server_unregister_error(HANDLE web_server, WEB_RESPONSE code)
{
    ack(web_server, HAL_REQ(HAL_WEBS, WEBS_UNREGISTER_RESPONSE), (unsigned int)code, 0, 0);
}

void web_server_write(HANDLE web_server, HANDLE session, WEB_RESPONSE code,  IO* io)
{
    *((WEB_RESPONSE*)io_push(io, sizeof(WEB_RESPONSE))) = code;
    io_write(web_server, HAL_REQ(HAL_WEBS, IPC_WRITE), session, io);
}

int web_server_write_sync(HANDLE web_server, HANDLE session, WEB_RESPONSE code,  IO* io)
{
    *((WEB_RESPONSE*)io_push(io, sizeof(WEB_RESPONSE))) = code;
    return io_write_sync(web_server, HAL_REQ(HAL_WEBS, IPC_WRITE), session, io);
}
