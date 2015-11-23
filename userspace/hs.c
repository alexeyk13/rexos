/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "hs.h"
#include "../midware/http/hss.h"
#include <string.h>

HANDLE hs_create()
{
    return process_create(&__HSS);
}

bool hs_open(HANDLE hs, uint16_t port, HANDLE tcpip)
{
    return get_handle(hs, HAL_REQ(HAL_HTTP, IPC_OPEN), port, tcpip, 0) != INVALID_HANDLE;
}

void hs_close(HANDLE hs)
{
    ack(hs, HAL_REQ(HAL_HTTP, IPC_CLOSE), 0, 0, 0);
}

HANDLE hs_create_obj(HANDLE hs, HANDLE parent, const char* name, unsigned int flags)
{
    HANDLE res;
    IO* io = io_create(strlen(name) + 1);
    if (io == NULL)
        return INVALID_HANDLE;
    strcpy(io_data(io), name);
    if (get_size(hs, HAL_IO_REQ(HAL_HTTP, HTTP_CREATE_OBJ), parent, (unsigned int)io, flags) >= sizeof(HANDLE))
        res = *((HANDLE*)io_data(io));
    else
        res = INVALID_HANDLE;
    io_destroy(io);
    return res;
}

void hs_destroy_obj(HANDLE hs, HANDLE obj)
{
    ack(hs, HAL_REQ(HAL_HTTP, HTTP_DESTROY_OBJ), obj, 0, 0);
}

HS_RESPONSE* hs_prepare_response(IO* io)
{
    HS_RESPONSE* resp;
    io_reset(io);
    resp = io_push(io, sizeof(HS_RESPONSE));
    resp->response = HTTP_RESPONSE_OK;
    resp->content_size = 0;
    resp->content_type = HTTP_CONTENT_NONE;
    resp->encoding_type = HTTP_ENCODING_NONE;
    return resp;
}

void hs_respond(HANDLE hs, HANDLE session, unsigned int method, IO* io, IO* user_io)
{
    HS_RESPONSE* resp = io_stack(io);
    resp->content_size = user_io->data_size;
    //send response (ok)
    io_complete(hs, HAL_IO_CMD(HAL_HTTP, method), session, io);
    io_write_sync(hs, HAL_IO_REQ(HAL_HTTP, IPC_WRITE), session, user_io);
}

void hs_respond_error(HANDLE hs, HANDLE session, unsigned int method, IO* io, HTTP_RESPONSE code)
{
    HS_RESPONSE* resp = hs_prepare_response(io);

    resp->response = code;
    //send response
    io_complete(hs, HAL_IO_CMD(HAL_HTTP, method), session, io);
}

void hs_register_error(HANDLE hs, HTTP_RESPONSE code, const char* html)
{
    ack(hs, HAL_REQ(HAL_HTTP, HTTP_REGISTER_ERROR), (unsigned int)code, (unsigned int)html, 0);
}

void hs_unregister_error(HANDLE hs, HTTP_RESPONSE code)
{
    ack(hs, HAL_REQ(HAL_HTTP, HTTP_UNREGISTER_ERROR), (unsigned int)code, 0, 0);
}
