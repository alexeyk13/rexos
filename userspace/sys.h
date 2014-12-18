/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SYS_H
#define SYS_H

#include "ipc.h"
#include "stream.h"
#include "object.h"
#include "types.h"
#include "sys_config.h"

typedef enum {
    IPC_SET_STDIO = IPC_SYSTEM + 1,                             //!< Will be called for objects, created before global stdout STREAM is set
    IPC_GET_INFO,
    IPC_READ,
    IPC_READ_COMPLETE,
    IPC_WRITE,
    IPC_WRITE_COMPLETE,
    IPC_FLUSH,
    IPC_SEEK,
    IPC_OPEN,
    IPC_CLOSE,
    IPC_GET_RX_STREAM,
    IPC_GET_TX_STREAM

}SYS_IPCS;

typedef enum {
    HAL_GPIO = 0,
    HAL_POWER,
    HAL_TIMER,
    HAL_RTC,
    HAL_WDT,
    HAL_UART,
    HAL_USB,
    HAL_USBD,
    HAL_ADC,
    HAL_DAC,
    HAL_I2C,
    HAL_LCD
} HAL;

#define HAL_HANDLE(group, item)                             ((group) << 16 | (item))
#define HAL_ITEM(handle)                                    ((handle) & 0xffff)
#define HAL_GROUP(handle)                                   ((handle) >> 16)

#define HAL_IPC(hal)                                        (IPC_USER + ((hal) << 16))
#define HAL_IPC_GROUP(ipc)                                  (((ipc) - IPC_USER) >> 16)

/** \addtogroup sys sys
    \{
 */

__STATIC_INLINE bool open_stdout()
{
    HANDLE stream = object_get(SYS_OBJ_STDOUT);
    if (stream != INVALID_HANDLE)
        __HEAP->stdout = stream_open(stream);
    return __HEAP->stdout != INVALID_HANDLE;
}

/**
    \brief open stdid
    \retval true on success
*/

__STATIC_INLINE bool open_stdin()
{
    HANDLE stream = object_get(SYS_OBJ_STDIN);
    if (stream != INVALID_HANDLE)
        __HEAP->stdin = stream_open(stream);
    return __HEAP->stdin != INVALID_HANDLE;
}

/**
    \brief close stdout
    \retval none
*/

__STATIC_INLINE void close_stdout()
{
    if (__HEAP->stdout != INVALID_HANDLE)
        stream_close(__HEAP->stdout);
}

/**
    \brief close stdid
    \retval none
*/

__STATIC_INLINE void close_stdin()
{
    if (__HEAP->stdin != INVALID_HANDLE)
        stream_close(__HEAP->stdin);
}

/** \} */ // end of sys group

#endif // SYS_H
