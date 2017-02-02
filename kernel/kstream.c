/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "kstream.h"
#include "kernel.h"
#include "kstdlib.h"
#include "kipc.h"
#include "kso.h"
#include "../userspace/error.h"
#include <string.h>
#include "../userspace/rb.h"
#include "../userspace/dlist.h"
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
    HANDLE listener;
    unsigned int listener_param;
    HAL listener_hal;
}STREAM;

typedef struct _STREAM_HANDLE{
    DLIST list;
    MAGIC;
    STREAM* stream;
    HANDLE process;
    STREAM_MODE mode;
    char* buf;
    unsigned int size;
}STREAM_HANDLE;

unsigned int kstream_get_size_internal(STREAM* stream)
{
    unsigned int size;
    disable_interrupts();
    size = rb_size(&stream->rb);
    enable_interrupts();
    return size;
}

unsigned int kstream_get_size(HANDLE s)
{
    STREAM* stream = (STREAM*)s;
    CHECK_MAGIC(stream, MAGIC_STREAM);
    return kstream_get_size_internal(stream);
}

unsigned int kstream_get_free(HANDLE s)
{
    unsigned int size;
    STREAM* stream = (STREAM*)s;
    CHECK_MAGIC(stream, MAGIC_STREAM);
    disable_interrupts();
    size = rb_free(&stream->rb);
    enable_interrupts();
    return size;
}

void kstream_lock_release(HANDLE h, HANDLE process)
{
    STREAM_HANDLE* handle = (STREAM_HANDLE*)h;
    switch (handle->mode)
    {
    case STREAM_MODE_READ:
        dlist_remove((DLIST**)&(handle->stream->read_waiters), (DLIST*)handle);
        break;
    case STREAM_MODE_WRITE:
        dlist_remove((DLIST**)&(handle->stream->write_waiters), (DLIST*)handle);
        break;
    default:
        break;
    }
    handle->mode = STREAM_MODE_IDLE;
}

HANDLE kstream_create(unsigned int size)
{
    STREAM* stream = kmalloc(sizeof(STREAM));
    if (stream == NULL)
        return INVALID_HANDLE;
    //allocate stream data
    stream->data = kmalloc(size);
    if (stream->data == NULL)
    {
        kfree(stream);
        return INVALID_HANDLE;
    }
    DO_MAGIC(stream, MAGIC_STREAM);
    rb_init(&stream->rb, size);
    stream->listener = INVALID_HANDLE;
    stream->write_waiters = stream->read_waiters = NULL;
    return (HANDLE)stream;
}

HANDLE kstream_open(HANDLE process, HANDLE s)
{
    STREAM_HANDLE* handle;
    STREAM* stream = (STREAM*)s;
    if (s == INVALID_HANDLE)
        return INVALID_HANDLE;
    CHECK_MAGIC(stream, MAGIC_STREAM);
    handle = kmalloc(sizeof(STREAM_HANDLE));
    if (handle == NULL)
        return INVALID_HANDLE;

    DO_MAGIC(handle, MAGIC_STREAM_HANDLE);
    handle->process = process;
    handle->stream = stream;
    handle->mode = STREAM_MODE_IDLE;
    return (HANDLE)handle;
}

void kstream_close(HANDLE process, HANDLE h)
{
    STREAM_HANDLE* handle = (STREAM_HANDLE*)h;
    if (h == INVALID_HANDLE)
        return;
    CHECK_MAGIC(handle, MAGIC_STREAM_HANDLE);
    if (handle->stream == (STREAM*)INVALID_HANDLE)
    {
        error(ERROR_SYNC_OBJECT_DESTROYED);
        return;
    }
    if (handle->process != process)
    {
        error(ERROR_ACCESS_DENIED);
        return;
    }
    kfree(handle);
}

static void kstream_check_inform(STREAM* stream)
{
    unsigned int size;
    IPC ipc;
    size = kstream_get_size_internal(stream);
    //inform listener
    if (size && (stream->listener != INVALID_HANDLE))
    {
        ipc.process = stream->listener;
        ipc.cmd = HAL_CMD(stream->listener_hal, IPC_STREAM_WRITE);
        ipc.param1 = (unsigned int)stream->listener_param;
        ipc.param2 = (HANDLE)stream;
        ipc.param3 = size;
        stream->listener = INVALID_HANDLE;
        kipc_post(KERNEL_HANDLE, &ipc);
    }
}

void kstream_listen(HANDLE process, HANDLE s, unsigned int param, HAL hal)
{
    STREAM* stream = (STREAM*)s;
    CHECK_MAGIC(stream, MAGIC_STREAM);
    if (stream->listener != INVALID_HANDLE)
    {
        error(ERROR_ACCESS_DENIED);
        return;
    }

    stream->listener = process;
    stream->listener_param = param;
    stream->listener_hal = hal;
    kstream_check_inform(stream);
}

void kstream_stop_listen(HANDLE process, HANDLE s)
{
    STREAM* stream = (STREAM*)s;
    CHECK_MAGIC(stream, MAGIC_STREAM);
    if (stream->listener != process)
    {
        error(ERROR_ACCESS_DENIED);
        return;
    }
    stream->listener = INVALID_HANDLE;
}

unsigned int kstream_write_no_block_internal(STREAM_HANDLE *handle, char* buf, unsigned int size_max)
{
    register STREAM_HANDLE* reader;
    unsigned int to_write = size_max;
    disable_interrupts();
    //write directly to output
    while (to_write && (reader = handle->stream->read_waiters) != NULL)
    {
        //all can go directly
        if (reader->size <= to_write)
        {
            memcpy(reader->buf, buf, reader->size);
            to_write -= reader->size;
            buf += reader->size;
            dlist_remove_head((DLIST**)&handle->stream->read_waiters);
            kprocess_wakeup(reader->process);
            reader->mode = STREAM_MODE_IDLE;
        }
        else
        //part can go directly
        {
            memcpy(reader->buf, buf, to_write);
            reader->buf += to_write;
            reader->size -= to_write;
            buf += to_write;
            to_write = 0;
        }
    }
    //write rest to stream
    for(; to_write && !rb_is_full(&handle->stream->rb); --to_write)
        handle->stream->data[rb_put(&handle->stream->rb)] = *buf++;
    enable_interrupts();
    return size_max - to_write;
}

unsigned int kstream_write_no_block(HANDLE h, char* buf, unsigned int size_max)
{
    STREAM_HANDLE* handle = (STREAM_HANDLE*)h;
    CHECK_MAGIC(handle, MAGIC_STREAM_HANDLE);
    unsigned int res = kstream_write_no_block_internal(handle, buf, size_max);
    kstream_check_inform(handle->stream);
    return res;
}

void kstream_write(HANDLE process, HANDLE h, char* buf, unsigned int size)
{
    STREAM_HANDLE* handle = (STREAM_HANDLE*)h;
    CHECK_MAGIC(handle, MAGIC_STREAM_HANDLE);
    handle->size = size - kstream_write_no_block_internal(handle, buf, size);
    //still more? wait
    if (handle->size)
    {
        handle->buf = buf + (size - handle->size);
        handle->mode = STREAM_MODE_WRITE;
        kprocess_sleep(process, NULL, PROCESS_SYNC_STREAM, h);
        disable_interrupts();
        dlist_add_tail((DLIST**)&handle->stream->write_waiters, (DLIST*)handle);
        enable_interrupts();
    }
    kstream_check_inform(handle->stream);
}

unsigned int kstream_read_no_block(HANDLE h, char* buf, unsigned int size_max)
{
    register STREAM_HANDLE* writer;
    register unsigned int to_read, to_push;
    STREAM_HANDLE* handle = (STREAM_HANDLE*)h;
    CHECK_MAGIC(handle, MAGIC_STREAM_HANDLE);
    //read from stream
    disable_interrupts();
    for(to_read = size_max; to_read && !rb_is_empty(&handle->stream->rb); --to_read)
        *buf++ = handle->stream->data[rb_get(&handle->stream->rb)];
    //read directly from input
    while (to_read && (writer = handle->stream->write_waiters) != NULL)
    {
        //all can go directly
        if (writer->size <= to_read)
        {
            memcpy(buf, writer->buf, writer->size);
            to_read -= writer->size;
            buf += writer->size;
            //wakeup writer
            dlist_remove_head((DLIST**)&handle->stream->write_waiters);
            kprocess_wakeup(writer->process);
            writer->mode = STREAM_MODE_IDLE;
        }
        else
        //part can go directly
        {
            memcpy(buf, writer->buf, to_read);
            writer->size -= to_read;
            writer->buf += to_read;
            buf += to_read;
            to_read = 0;
        }
    }
    //push data to stream internally after read
    if (to_read < size_max)
    {
        to_push = rb_free(&handle->stream->rb);
        while ((writer = handle->stream->write_waiters) != NULL && to_push)
        {
            for(; to_push && writer->size; --writer->size, --to_push)
                handle->stream->data[rb_put(&handle->stream->rb)] = *writer->buf++;
            //writed all from waiter? Wake him up.
            if (!writer->size)
            {
                dlist_remove_head((DLIST**)&handle->stream->write_waiters);
                kprocess_wakeup(writer->process);
                writer->mode = STREAM_MODE_IDLE;
            }
        }
    }
    enable_interrupts();
    return size_max - to_read;
}

void kstream_read(HANDLE process, HANDLE h, char* buf, unsigned int size)
{
    STREAM_HANDLE* handle = (STREAM_HANDLE*)h;
    handle->size = size - kstream_read_no_block(h, buf, size);
    //still need more? Wait.
    if (handle->size)
    {
        handle->buf = buf + (size - handle->size);
        handle->mode = STREAM_MODE_READ;
        kprocess_sleep(process, NULL, PROCESS_SYNC_STREAM, h);
        disable_interrupts();
        dlist_add_tail((DLIST**)&handle->stream->read_waiters, (DLIST*)handle);
        enable_interrupts();
    }
}

void kstream_flush(HANDLE s)
{
    STREAM* stream = (STREAM*)s;
    STREAM_HANDLE* handle;
    CHECK_MAGIC(stream, MAGIC_STREAM);
    stream->listener = INVALID_HANDLE;
    //flush stream
    disable_interrupts();
    rb_clear(&stream->rb);
    //flush waiters
    while ((handle = stream->write_waiters) != NULL)
    {
        handle->mode = STREAM_MODE_IDLE;
        dlist_remove_head((DLIST**)&stream->write_waiters);
        kprocess_wakeup(handle->process);
    }
    enable_interrupts();
}

static void kstream_destroy_handle(STREAM* stream, DLIST** head)
{
    STREAM_HANDLE* handle;
    while (stream->write_waiters)
    {
        handle = (STREAM_HANDLE*)(*head);
        handle->stream = (STREAM*)INVALID_HANDLE;
        disable_interrupts();
        dlist_remove_head(head);
        kprocess_wakeup(handle->process);
        enable_interrupts();
        kprocess_error(handle->process, ERROR_SYNC_OBJECT_DESTROYED);
    }
}

void kstream_destroy(HANDLE s)
{
    STREAM* stream = (STREAM*)s;
    if (s == INVALID_HANDLE)
        return;
    CHECK_MAGIC(stream, MAGIC_STREAM);
    CLEAR_MAGIC(stream);
    kstream_destroy_handle(stream, (DLIST**)&stream->write_waiters);
    kstream_destroy_handle(stream, (DLIST**)&stream->read_waiters);
    kfree(stream->data);
    kfree(stream);
}
