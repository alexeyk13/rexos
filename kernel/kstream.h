/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KSTREAM_H
#define KSTREAM_H

#include "../userspace/lib/rb.h"
#include "../userspace/lib/dlist.h"
#include "kprocess.h"
#include "dbg.h"

typedef enum {
    STREAM_MODE_IDLE,
    STREAM_MODE_READ,
    STREAM_MODE_WRITE
}STREAM_MODE;

typedef struct {
    MAGIC;
    RB rb;
    char* data;
    //list actually
    struct _STREAM_HANDLE* write_waiters;
    struct _STREAM_HANDLE* read_waiters;
    //item
    PROCESS* listener;
    void* listener_param;
}STREAM;

typedef struct _STREAM_HANDLE{
    DLIST list;
    MAGIC;
    STREAM* stream;
    PROCESS* process;
    STREAM_MODE mode;
    char* buf;
    unsigned int size;
}STREAM_HANDLE;

//called from kprocess
void kstream_lock_release(STREAM_HANDLE* handle, PROCESS* process);

//called from svc
void kstream_create(STREAM** stream, unsigned int size);
void kstream_open(STREAM* stream, STREAM_HANDLE** handle);
void kstream_close(STREAM_HANDLE* handle);
void kstream_get_size(STREAM* stream, unsigned int* size);
void kstream_get_free(STREAM* stream, unsigned int *size);
void kstream_listen(STREAM* stream, void *param);
void kstream_stop_listen(STREAM* stream);
void kstream_write(STREAM_HANDLE* handle, char* buf, unsigned int size);
void kstream_read(STREAM_HANDLE* handle, char* buf, unsigned int size);
void kstream_flush(STREAM* stream);
void kstream_destroy(STREAM* stream);

#endif // KSTREAM_H
