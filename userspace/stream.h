/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STREAM_H
#define STREAM_H

#include "types.h"
#include "process.h"
#include "svc.h"
#include "error.h"
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
__STATIC_INLINE HANDLE stream_create(unsigned int size)
{
    HANDLE stream = 0;
    svc_call(SVC_STREAM_CREATE, (unsigned int)&stream, size, 0);
    return stream;
}

/**
    \brief open STREAM handle
    \param stream: handle of created stream
    \retval STREAM HANDLE on success. On failure (out of memory), error will be raised
*/
__STATIC_INLINE HANDLE stream_open(HANDLE stream)
{
    HANDLE handle = 0;
    svc_call(SVC_STREAM_OPEN, (unsigned int)stream, (unsigned int)&handle, 0);
    return handle;
}

/**
    \brief close STREAM handle
    \param handle: handle of created stream
    \retval none
*/
__STATIC_INLINE void stream_close(HANDLE handle)
{
    svc_call(SVC_STREAM_CLOSE, (unsigned int)handle, 0, 0);
}

/**
    \brief get STREAM used size
    \param stream: created STREAM object
    \retval used size
*/
__STATIC_INLINE unsigned int stream_get_size(HANDLE stream)
{
    unsigned int size;
    svc_call(SVC_STREAM_GET_SIZE, (unsigned int)stream, (unsigned int)&size, 0);
    return size;
}

/**
    \brief get STREAM free bytes count
    \param stream: created STREAM object
    \retval free size
*/
__STATIC_INLINE unsigned int stream_get_free(HANDLE stream)
{
    unsigned int size;
    svc_call(SVC_STREAM_GET_FREE, (unsigned int)stream, (unsigned int)&size, 0);
    return size;
}

/**
    \brief start listen to STREAM successfull writes. Caller must be one, who created stream
    \param stream: created STREAM object
    \param hal: HAL group for parsing
    \retval true on ok
*/
__STATIC_INLINE bool stream_listen(HANDLE stream, unsigned int param, HAL hal)
{
    error(ERROR_OK);
    svc_call(SVC_STREAM_LISTEN, (unsigned int)stream, param, (unsigned int)hal);
    return get_last_error() == ERROR_OK;
}

/**
    \brief stop listen to STREAM successfull writes. Caller must be one, who created stream
    \param stream: created STREAM object
    \retval true on ok
*/
__STATIC_INLINE bool stream_stop_listen(HANDLE stream)
{
    error(ERROR_OK);
    svc_call(SVC_STREAM_STOP_LISTEN, (unsigned int)stream, 0, 0);
    return get_last_error() == ERROR_OK;
}

/**
    \brief write to STREAM handle
    \param handle: handle of created stream
    \param buf: pointer to data
    \param size: size of data to write
    \retval true on ok
*/
__STATIC_INLINE bool stream_write(HANDLE handle, const char* buf, unsigned int size)
{
    error(ERROR_OK);
    svc_call(SVC_STREAM_WRITE, (unsigned int)handle, (unsigned int)buf, (unsigned int)size);
    return get_last_error() == ERROR_OK;
}

/**
    \brief read from STREAM handle
    \param handle: handle of created stream
    \param buf: pointer to data
    \param size: size of data to read
    \retval true on ok
*/
__STATIC_INLINE bool stream_read(HANDLE handle, char* buf, unsigned int size)
{
    error(ERROR_OK);
    svc_call(SVC_STREAM_READ, (unsigned int)handle, (unsigned int)buf, (unsigned int)size);
    return get_last_error() == ERROR_OK;
}

/**
    \brief flush STREAM
    \param stream: created STREAM object
    \retval none
*/
__STATIC_INLINE void stream_flush(HANDLE stream)
{
    svc_call(SVC_STREAM_FLUSH, (unsigned int)stream, 0, 0);
}

/**
    \brief destroy STREAM object
    \param stream: created STREAM object
    \retval none
*/
__STATIC_INLINE void stream_destroy(HANDLE stream)
{
    svc_call(SVC_STREAM_DESTROY, (unsigned int)stream, 0, 0);
}

/** \} */ // end of strean group

#endif // STREAM_H
