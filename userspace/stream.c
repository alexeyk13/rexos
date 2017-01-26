/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "stream.h"
#include "process.h"
#include "svc.h"
#include "error.h"

HANDLE stream_create(unsigned int size)
{
    HANDLE stream = 0;
    svc_call(SVC_STREAM_CREATE, (unsigned int)&stream, size, 0);
    return stream;
}

HANDLE stream_open(HANDLE stream)
{
    HANDLE handle = 0;
    svc_call(SVC_STREAM_OPEN, (unsigned int)stream, (unsigned int)&handle, 0);
    return handle;
}

void stream_close(HANDLE handle)
{
    svc_call(SVC_STREAM_CLOSE, (unsigned int)handle, 0, 0);
}

unsigned int stream_get_size(HANDLE stream)
{
    unsigned int size;
    svc_call(SVC_STREAM_GET_SIZE, (unsigned int)stream, (unsigned int)&size, 0);
    return size;
}

unsigned int stream_get_free(HANDLE stream)
{
    unsigned int size;
    svc_call(SVC_STREAM_GET_FREE, (unsigned int)stream, (unsigned int)&size, 0);
    return size;
}

void stream_listen(HANDLE stream, unsigned int param, HAL hal)
{
    svc_call(SVC_STREAM_LISTEN, (unsigned int)stream, param, (unsigned int)hal);
}

void stream_ilisten(HANDLE stream, unsigned int param, HAL hal)
{
    __GLOBAL->svc_irq(SVC_STREAM_LISTEN, (unsigned int)stream, param, (unsigned int)hal);
}

void stream_stop_listen(HANDLE stream)
{
    svc_call(SVC_STREAM_STOP_LISTEN, (unsigned int)stream, 0, 0);
}

unsigned int stream_write_no_block(HANDLE handle, const char* buf, unsigned int size)
{
    unsigned int written = size;
    svc_call(SVC_STREAM_WRITE_NO_BLOCK, (unsigned int)handle, (unsigned int)buf, (unsigned int)(&written));
    return written;
}

unsigned int stream_iwrite_no_block(HANDLE handle, const char* buf, unsigned int size)
{
    unsigned int written = size;
    __GLOBAL->svc_irq(SVC_STREAM_WRITE_NO_BLOCK, (unsigned int)handle, (unsigned int)buf, (unsigned int)(&written));
    return written;
}

bool stream_write(HANDLE handle, const char* buf, unsigned int size)
{
    error(ERROR_OK);
    svc_call(SVC_STREAM_WRITE, (unsigned int)handle, (unsigned int)buf, (unsigned int)size);
    return get_last_error() == ERROR_OK;
}

unsigned int stream_read_no_block(HANDLE handle, const char* buf, unsigned int size)
{
    unsigned int readed = size;
    svc_call(SVC_STREAM_READ_NO_BLOCK, (unsigned int)handle, (unsigned int)buf, (unsigned int)(&readed));
    return readed;
}

unsigned int stream_iread_no_block(HANDLE handle, const char* buf, unsigned int size)
{
    unsigned int readed = size;
    __GLOBAL->svc_irq(SVC_STREAM_READ_NO_BLOCK, (unsigned int)handle, (unsigned int)buf, (unsigned int)(&readed));
    return readed;
}

bool stream_read(HANDLE handle, char* buf, unsigned int size)
{
    error(ERROR_OK);
    svc_call(SVC_STREAM_READ, (unsigned int)handle, (unsigned int)buf, (unsigned int)size);
    return get_last_error() == ERROR_OK;
}

void stream_flush(HANDLE stream)
{
    svc_call(SVC_STREAM_FLUSH, (unsigned int)stream, 0, 0);
}

void stream_destroy(HANDLE stream)
{
    svc_call(SVC_STREAM_DESTROY, (unsigned int)stream, 0, 0);
}
