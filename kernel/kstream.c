/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "kstream.h"
#include "kernel.h"
#include "kmalloc.h"
#include "kipc.h"
#include "../userspace/error.h"
#include <string.h>

//push data to stream internally after read
void kstream_push(STREAM* stream)
{
    STREAM_HANDLE* handle;
    int written = 0;
    while ((handle = stream->write_waiters) != NULL && !rb_is_full(&stream->rb))
    {
        for(; !rb_is_full(&stream->rb) && handle->size > 0; --handle->size)
        {
            stream->data[rb_put(&stream->rb)] = *handle->buf++;
            ++written;
        }
        //writed all from waiter? Wake him up.
        if (handle->size <= 0)
        {
            handle->mode = STREAM_MODE_IDLE;
            dlist_remove_head((DLIST**)&stream->write_waiters);
            kprocess_wakeup(handle->process);
        }
    }
}

void kstream_lock_release(STREAM_HANDLE* handle, PROCESS* process)
{
    CHECK_HANDLE(handle, sizeof(STREAM_HANDLE));
    CHECK_MAGIC(handle, MAGIC_STREAM_HANDLE);
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

void kstream_create(STREAM** stream, unsigned int size)
{
    PROCESS* process = kprocess_get_current();
    CHECK_ADDRESS(process, stream, sizeof(void*));
    *stream = kmalloc(sizeof(STREAM));
    if ((*stream = kmalloc(sizeof(STREAM))) == NULL)
    {
        kprocess_error(process, ERROR_OUT_OF_SYSTEM_MEMORY);
        return;
    }
    //allocate stream data
    if (((*stream)->data = paged_alloc(size)) == NULL)
    {
        kfree(*stream);
        (*stream) = NULL;
        kprocess_error(process, ERROR_OUT_OF_PAGED_MEMORY);
        return;
    }
    DO_MAGIC((*stream), MAGIC_STREAM);
    rb_init(&(*stream)->rb, size);
    (*stream)->listener = INVALID_HANDLE;
    (*stream)->write_waiters = (*stream)->read_waiters = NULL;
}

void kstream_open(STREAM* stream, STREAM_HANDLE** handle)
{
    CHECK_HANDLE(stream, sizeof(STREAM));
    CHECK_MAGIC(stream, MAGIC_STREAM);
    PROCESS* process = kprocess_get_current();
    CHECK_ADDRESS(process, handle, sizeof(void*));
    *handle = kmalloc(sizeof(STREAM_HANDLE));
    if (*handle != NULL)
    {
        DO_MAGIC((*handle), MAGIC_STREAM_HANDLE);
        (*handle)->process = process;
        (*handle)->stream = stream;
        (*handle)->mode = STREAM_MODE_IDLE;
    }
    else
        kprocess_error(process, ERROR_OUT_OF_SYSTEM_MEMORY);
}

void kstream_close(STREAM_HANDLE* handle)
{
    if ((HANDLE)handle == INVALID_HANDLE)
        return;
    CHECK_HANDLE(handle, sizeof(STREAM_HANDLE));
    CHECK_MAGIC(handle, MAGIC_STREAM_HANDLE);
    switch (handle->mode)
    {
    case STREAM_MODE_READ:
        dlist_remove((DLIST**)&(handle->stream->read_waiters), (DLIST*)handle);
        kprocess_wakeup(handle->process);
        kprocess_error(handle->process, ERROR_SYNC_OBJECT_DESTROYED);
        break;
    case STREAM_MODE_WRITE:
        dlist_remove((DLIST**)&(handle->stream->write_waiters), (DLIST*)handle);
        kprocess_wakeup(handle->process);
        kprocess_error(handle->process, ERROR_SYNC_OBJECT_DESTROYED);
        break;
    default:
        break;
    }
    kfree(handle);
}

void kstream_get_size(STREAM* stream, unsigned int *size)
{
    DLIST_ENUM de;
    STREAM_HANDLE* cur;
    CHECK_HANDLE(stream, sizeof(STREAM));
    CHECK_MAGIC(stream, MAGIC_STREAM);
    CHECK_ADDRESS(kprocess_get_current(), size, sizeof(unsigned int));
    if (rb_is_full(&stream->rb))
    {
        *size = stream->rb.size - 1;
        dlist_enum_start((DLIST**)&stream->write_waiters, &de);
        while (dlist_enum(&de, (DLIST**)&cur))
            *size += cur->size;
    }
    else
        *size = rb_size(&stream->rb);
}

void kstream_get_free(STREAM *stream, unsigned int* size)
{
    DLIST_ENUM de;
    STREAM_HANDLE* cur;
    CHECK_HANDLE(stream, sizeof(STREAM));
    CHECK_MAGIC(stream, MAGIC_STREAM);
    CHECK_ADDRESS(kprocess_get_current(), size, sizeof(unsigned int));
    if (rb_is_empty(&stream->rb))
    {
        *size = stream->rb.size - 1;
        dlist_enum_start((DLIST**)&stream->read_waiters, &de);
        while (dlist_enum(&de, (DLIST**)&cur))
            *size += cur->size;
    }
    else
        *size = rb_free(&stream->rb);
}

void kstream_listen(STREAM* stream, void* param)
{
    CHECK_HANDLE(stream, sizeof(STREAM));
    CHECK_MAGIC(stream, MAGIC_STREAM);
    PROCESS* process = kprocess_get_current();
    if (stream->listener == INVALID_HANDLE)
    {
        stream->listener = (HANDLE)process;
        stream->listener_param = param;
    }
    else
        kprocess_error(process, ERROR_ACCESS_DENIED);
}

void kstream_stop_listen(STREAM* stream)
{
    CHECK_HANDLE(stream, sizeof(STREAM));
    CHECK_MAGIC(stream, MAGIC_STREAM);
    PROCESS* process = kprocess_get_current();
    if (stream->listener == (HANDLE)process)
        stream->listener = INVALID_HANDLE;
    else
        kprocess_error(process, ERROR_ACCESS_DENIED);
}

void kstream_write(STREAM_HANDLE *handle, char* buf, unsigned int size)
{
    IPC ipc;
    CHECK_HANDLE(handle, sizeof(STREAM_HANDLE));
    ASSERT(handle->mode == STREAM_MODE_IDLE);
    TIME time;
    int written = 0;
    STREAM_HANDLE* reader;
    CHECK_MAGIC(handle, MAGIC_STREAM_HANDLE);
    PROCESS* process = kprocess_get_current();
    CHECK_ADDRESS_FLASH(process, buf, size);
    handle->size = size;
    //write directly to output
    while (handle->size && (reader = handle->stream->read_waiters) != NULL)
    {
        //all can go directly
        if (handle->size >= reader->size)
        {
            memcpy(reader->buf, buf, reader->size);
            handle->size -= reader->size;
            buf += reader->size;
            dlist_remove_head((DLIST**)&handle->stream->read_waiters);
            reader->mode = STREAM_MODE_IDLE;
            //wakeup reader
            kprocess_wakeup(reader->process);
        }
        else
        //part can go directly
        {
            memcpy(reader->buf, buf, handle->size);
            reader->buf += handle->size;
            reader->size -= handle->size;
            buf += handle->size;
            handle->size = 0;
        }
    }
    //write rest to stream
    for(; handle->size && !rb_is_full(&handle->stream->rb); --handle->size)
    {
        handle->stream->data[rb_put(&handle->stream->rb)] = *buf++;
        ++written;
    }
    //still more? wait
    if (handle->size)
    {
        handle->buf = buf;
        handle->mode = STREAM_MODE_WRITE;
        dlist_add_tail((DLIST**)&handle->stream->write_waiters, (DLIST*)handle);
        time.sec = time.usec = 0;
        kprocess_sleep(process, &time, PROCESS_SYNC_STREAM, handle);
    }
    //inform listener
    if ((handle->stream->listener != INVALID_HANDLE) && size)
    {
        ipc.process = handle->stream->listener;
        ipc.cmd = IPC_STREAM_WRITE;
        ipc.param1 = (HANDLE)handle->stream;
        ipc.param2 = size;
        ipc.param3 = (unsigned int)handle->stream->listener_param;
        handle->stream->listener = INVALID_HANDLE;
        kipc_post_process(&ipc, INVALID_HANDLE);
    }
}

void kstream_read(STREAM_HANDLE* handle, char* buf, unsigned int size)
{
    CHECK_HANDLE(handle, sizeof(STREAM_HANDLE));
    ASSERT(handle->mode == STREAM_MODE_IDLE);
    register STREAM_HANDLE* writer;
    TIME time;
    CHECK_MAGIC(handle, MAGIC_STREAM_HANDLE);
    PROCESS* process = kprocess_get_current();
    CHECK_ADDRESS(process, buf, size);
    handle->size = size;
    //read from stream
    for(; handle->size && !rb_is_empty(&handle->stream->rb); --handle->size)
        *buf++ = handle->stream->data[rb_get(&handle->stream->rb)];
    //read directly from input
    while (handle->size && (writer = handle->stream->write_waiters) != NULL)
    {
        //all can go directly
        if (handle->size >= writer->size)
        {
            memcpy(buf, writer->buf, writer->size);
            handle->size -= writer->size;
            buf += writer->size;
            //wakeup writer
            writer->mode = STREAM_MODE_IDLE;
            dlist_remove_head((DLIST**)&handle->stream->write_waiters);
            kprocess_wakeup(writer->process);
        }
        else
        //part can go directly
        {
            memcpy(buf, writer->buf, handle->size);
            writer->size -= handle->size;
            writer->buf += handle->size;
            buf += handle->size;
            handle->size = 0;
        }
    }
    if (handle->size)
    {
        printk("head: %d, tail: %d\n\r", handle->stream->rb.head, handle->stream->rb.tail);
    }
    //still need more? Wait.
    if (handle->size)
    {
        printk("out of data, size: %d, req: %d\n\r", handle->size, size);
        handle->buf = buf;
        handle->mode = STREAM_MODE_READ;
        dlist_add_tail((DLIST**)&handle->stream->read_waiters, (DLIST*)handle);
        time.sec = time.usec = 0;
        kprocess_sleep(process, &time, PROCESS_SYNC_STREAM, handle);
    }
    else
        kstream_push(handle->stream);
}

void kstream_flush(STREAM* stream)
{
    CHECK_HANDLE(stream, sizeof(STREAM));
    STREAM_HANDLE* handle;
    CHECK_MAGIC(stream, MAGIC_STREAM);
    //flush stream
    rb_clear(&stream->rb);
    stream->listener = INVALID_HANDLE;
    //flush waiters
    while ((handle = stream->write_waiters) != NULL)
    {
        handle->mode = STREAM_MODE_IDLE;
        dlist_remove_head((DLIST**)&stream->write_waiters);
        kprocess_wakeup(handle->process);
    }
}

void kstream_destroy(STREAM* stream)
{
    if ((HANDLE)stream == INVALID_HANDLE)
        return;
    STREAM_HANDLE* handle;
    CHECK_HANDLE(stream, sizeof(STREAM));
    CHECK_MAGIC(stream, MAGIC_STREAM);
    CLEAR_MAGIC(stream);
    while (stream->write_waiters)
    {
        handle = stream->write_waiters;
        CHECK_MAGIC(handle, MAGIC_STREAM_HANDLE);
        dlist_remove_head((DLIST**)&stream->write_waiters);
        kprocess_wakeup(handle->process);
        kprocess_error(handle->process, ERROR_SYNC_OBJECT_DESTROYED);
        kfree(handle);
    }
    while (stream->read_waiters)
    {
        handle = stream->read_waiters;
        CHECK_MAGIC(handle, MAGIC_STREAM_HANDLE);
        dlist_remove_head((DLIST**)&stream->read_waiters);
        kprocess_wakeup(handle->process);
        kprocess_error(handle->process, ERROR_SYNC_OBJECT_DESTROYED);
        kfree(handle);
    }
    paged_free(stream->data);
    kfree(stream);
}
