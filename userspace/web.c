/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "web.h"
#include "../midware/http/webs.h"
#include "error.h"
#include <string.h>

extern void webs_main();

HANDLE web_server_create(unsigned int process_size, unsigned int priority)
{
    REX rex;
    rex.name = "Web Server";
    rex.size = process_size;
    rex.priority = priority;
    rex.flags = PROCESS_FLAGS_ACTIVE;
    rex.fn = webs_main;
    return process_create(&rex);
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

void web_server_read(HANDLE web_server, HANDLE session, IO* io, unsigned int size_max)
{
    io_read(web_server, HAL_REQ(HAL_WEBS, IPC_READ), session, io, size_max);
}

int web_server_read_sync(HANDLE web_server, HANDLE session, IO* io, unsigned int size_max)
{
    return io_read_sync(web_server, HAL_IO_REQ(HAL_WEBS, IPC_READ), session, io, size_max);
}

void web_server_write(HANDLE web_server, HANDLE session, WEB_RESPONSE code,  IO* io)
{
    *((WEB_RESPONSE*)io_push(io, sizeof(WEB_RESPONSE))) = code;
    io_write(web_server, HAL_IO_REQ(HAL_WEBS, IPC_WRITE), session, io);
}

int web_server_write_sync(HANDLE web_server, HANDLE session, WEB_RESPONSE code,  IO* io)
{
    *((WEB_RESPONSE*)io_push(io, sizeof(WEB_RESPONSE))) = code;
    return io_write_sync(web_server, HAL_IO_REQ(HAL_WEBS, IPC_WRITE), session, io);
}

char *web_server_get_param(HANDLE web_server, HANDLE session, IO* io, unsigned int size_max, char *param)
{
    unsigned int len = strlen(param);
    if (len + 1 < size_max)
    {
        error(ERROR_IO_BUFFER_TOO_SMALL);
        return NULL;
    }
    strcpy(io_data(io), param);
    io->data_size = len + 1;
    int res = io_read_sync(web_server, HAL_IO_REQ(HAL_WEBS, WEBS_GET_PARAM), session, io, size_max);
    if (res <= 0)
        return NULL;
    return io_data(io);
}

void web_server_set_param(HANDLE web_server, HANDLE session, IO* io, unsigned int size_max, const char* param, const char* value)
{
    unsigned int param_len, value_len;
    param_len = strlen(param) + 1;
    value_len = strlen(value) + 1;
    if (param_len + value_len < size_max)
    {
        error(ERROR_IO_BUFFER_TOO_SMALL);
        return;
    }
    strcpy(io_data(io), param);
    strcpy((char*)io_data(io) + param_len, value);
    io->data_size = param_len + value_len;
    io_write_sync(web_server, HAL_IO_REQ(HAL_WEBS, WEBS_SET_PARAM), session, io);
}

char *web_server_get_url(HANDLE web_server, HANDLE session, IO* io, unsigned int size_max)
{
    int res = io_read_sync(web_server, HAL_IO_REQ(HAL_WEBS, WEBS_GET_URL), session, io, size_max);
    if (res <= 0)
        return NULL;
    return io_data(io);
}
