/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STREAM_H
#define STREAM_H

#include "types.h"
#include "ipc.h"


/** \addtogroup stream stream
    stream is sort of \ref ipc, when you need to pass few bytes from one process to another

    \{
 */

/**
    \brief creates STREAM object.
    \param size: requested size in bytes
    \retval STREAM on success. On failure (out of memory), error will be raised
*/
HANDLE stream_create(unsigned int size);

/**
    \brief open STREAM handle
    \param stream: handle of created stream
    \retval STREAM HANDLE on success. On failure (out of memory), error will be raised
*/
HANDLE stream_open(HANDLE stream);

/**
    \brief close STREAM handle
    \param handle: handle of created stream
    \retval none
*/
void stream_close(HANDLE handle);

/**
    \brief get STREAM used size
    \param stream: created STREAM object
    \retval used size
*/
unsigned int stream_get_size(HANDLE stream);

/**
    \brief get STREAM free bytes count
    \param stream: created STREAM object
    \retval free size
*/
unsigned int stream_get_free(HANDLE stream);

/**
    \brief start listen to STREAM successfull writes.
    \param stream: created STREAM object
    \param hal: HAL group for parsing
    \retval none
*/
void stream_listen(HANDLE stream, unsigned int param, HAL hal);

/**
    \brief start listen to STREAM successfull writes. ISR version
    \param stream: created STREAM object
    \param hal: HAL group for parsing
    \retval none
*/
void stream_ilisten(HANDLE stream, unsigned int param, HAL hal);

/**
    \brief stop listen to STREAM successfull writes. Caller must be one, who created stream
    \param stream: created STREAM object
    \retval none
*/
void stream_stop_listen(HANDLE stream);

/**
    \brief write to STREAM handle without blocking
    \param handle: handle of created stream
    \param buf: pointer to data
    \param size: max size of data to write
    \retval number of bytes written
*/
unsigned int stream_write_no_block(HANDLE handle, const char* buf, unsigned int size);

/**
    \brief write to STREAM handle without blocking, ISR version
    \param handle: handle of created stream
    \param buf: pointer to data
    \param size: max size of data to write
    \retval number of bytes written
*/
unsigned int stream_iwrite_no_block(HANDLE handle, const char* buf, unsigned int size);

/**
    \brief write to STREAM handle
    \param handle: handle of created stream
    \param buf: pointer to data
    \param size: size of data to write
    \retval true on ok
*/
bool stream_write(HANDLE handle, const char* buf, unsigned int size);

/**
    \brief read from STREAM handle without blocking
    \param handle: handle of created stream
    \param buf: pointer to data
    \param size: max size of data to read
    \retval number of bytes readed
*/
unsigned int stream_read_no_block(HANDLE handle, const char* buf, unsigned int size);

/**
    \brief read from STREAM handle without blocking, ISR version
    \param handle: handle of created stream
    \param buf: pointer to data
    \param size: max size of data to read
    \retval number of bytes readed
*/
unsigned int stream_iread_no_block(HANDLE handle, const char* buf, unsigned int size);

/**
    \brief read from STREAM handle
    \param handle: handle of created stream
    \param buf: pointer to data
    \param size: size of data to read
    \retval true on ok
*/
bool stream_read(HANDLE handle, char* buf, unsigned int size);

/**
    \brief flush STREAM
    \param stream: created STREAM object
    \retval none
*/
void stream_flush(HANDLE stream);

/**
    \brief destroy STREAM object
    \param stream: created STREAM object
    \retval none
*/
void stream_destroy(HANDLE stream);

/** \} */ // end of strean group

#endif // STREAM_H
