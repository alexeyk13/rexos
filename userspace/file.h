/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef FILE_H
#define FILE_H

#include "sys.h"
#include "direct.h"
#include "block.h"

/** \addtogroup file file
    \{
 */

#define FILE_MODE_READ                                  (1 << 0)
#define FILE_MODE_WRITE                                 (1 << 1)
#define FILE_MODE_READ_WRITE                            (FILE_MODE_READ | FILE_MODE_WRITE)

/**
    \brief open file
    \param process: process handle
    \param file: file handle
    \param mode: file mode
    \retval handle on success, INVALID_HANDLE on error
*/

__STATIC_INLINE HANDLE fopen(HANDLE process, HANDLE file, unsigned int mode)
{
    IPC ipc;
    ipc.cmd = IPC_OPEN;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = mode;
    if (call(&ipc))
        return ipc.param1;
    return INVALID_HANDLE;
}

/**
    \brief open file with custom param
    \param process: process handle
    \param file: file handle
    \param mode: file mode
    \param param: custom extra param
    \retval handle on success, INVALID_HANDLE on error
*/

__STATIC_INLINE HANDLE fopen_p(HANDLE process, HANDLE file, unsigned int mode, void* param)
{
    IPC ipc;
    ipc.cmd = IPC_OPEN;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = mode;
    ipc.param3 = (unsigned int)param;
    if (call(&ipc))
        return ipc.param1;
    return INVALID_HANDLE;
}

/**
    \brief open file with custom initialization data
    \param process: process handle
    \param file: file handle
    \param mode: file mode
    \param addr: address of direct read
    \param size: size of direct read
    \retval handle on success, INVALID_HANDLE on error
*/

__STATIC_INLINE HANDLE fopen_ex(HANDLE process, HANDLE file, unsigned int mode, void* ptr, unsigned int size)
{
    IPC ipc;
    direct_enable_read(process, ptr, size);
    ipc.cmd = IPC_OPEN;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = mode;
    ipc.param3 = size;
    if (call(&ipc))
        return ipc.param1;
    return INVALID_HANDLE;
}

/**
    \brief close file
    \param process: process handle
    \param file: file handle
    \retval true on success
*/

__STATIC_INLINE bool fclose(HANDLE process, HANDLE file)
{
    IPC ipc;
    ipc.cmd = IPC_CLOSE;
    ipc.process = process;
    ipc.param1 = file;
    return call(&ipc);
}

/**
    \brief read from file (async)
    \param process: process handle
    \param file: file handle
    \param block: file block
    \param size: size of transfer
    \retval none. Check corresponding IPC.
*/

__STATIC_INLINE void fread_async(HANDLE process, HANDLE file, HANDLE block, unsigned int size)
{
    IPC ipc;
    ipc.cmd = IPC_READ;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = block;
    ipc.param3 = size;
    if (block != INVALID_HANDLE)
        block_send_ipc(block, process, &ipc);
    else
        ipc_post(&ipc);
}

#define fread_complete             fread_async

/**
    \brief read from file (async), isr version
    \param process: process handle
    \param file: file handle
    \param block: file block
    \param size: size of transfer
    \retval none. Check corresponding IPC.
*/

__STATIC_INLINE void firead_async(HANDLE process, HANDLE file, HANDLE block, unsigned int size)
{
    IPC ipc;
    ipc.cmd = IPC_READ;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = block;
    ipc.param3 = size;
    if (block != INVALID_HANDLE)
        block_isend_ipc(block, process, &ipc);
    else
        ipc_ipost(&ipc);
}

#define firead_complete             firead_async

/**
    \brief read from file
    \param process: process handle
    \param file: file handle
    \param block: file block
    \param size: size of transfer
    \retval size of bytes readed
*/

__STATIC_INLINE int fread(HANDLE process, HANDLE file, HANDLE block, unsigned int size)
{
    IPC ipc;
    ipc.cmd = IPC_READ;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = block;
    ipc.param3 = size;

    if (block != INVALID_HANDLE)
        block_send(block, process);
    if (!call(&ipc))
        error(ipc.param3);
    return ipc.param3;
}

/**
    \brief read direct from file (async)
    \param process: process handle
    \param file: file handle
    \param addr: pointer to data
    \param size: size of transfer
    \retval none. Check corresponding IPC.
*/

__STATIC_INLINE void fdread_async(HANDLE process, HANDLE file, void* addr, unsigned int size)
{
    IPC ipc;
    ipc.cmd = IPC_READ;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = INVALID_HANDLE;
    ipc.param3 = size;
    direct_enable_write(process, addr, size);
    ipc_post(&ipc);
}

/**
    \brief read from file
    \param process: process handle
    \param file: file handle
    \param addr: pointer to data
    \param size: size of transfer
    \retval size of bytes readed
*/

__STATIC_INLINE int fdread(HANDLE process, HANDLE file, void* addr, unsigned int size)
{
    IPC ipc;
    ipc.cmd = IPC_READ;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = INVALID_HANDLE;
    ipc.param3 = size;
    direct_enable_write(process, addr, size);
    if (!call(&ipc))
        error(ipc.param3);
    return ipc.param3;
}

/**
    \brief write to file (async)
    \param process: process handle
    \param file: file handle
    \param block: file block
    \param size: size of transfer
    \retval none. Check corresponding IPC.
*/

__STATIC_INLINE void fwrite_async(HANDLE process, HANDLE file, HANDLE block, unsigned int size)
{
    IPC ipc;
    ipc.cmd = IPC_WRITE;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = block;
    ipc.param3 = size;
    if (block != INVALID_HANDLE)
        block_send_ipc(block, process, &ipc);
    else
        ipc_post(&ipc);
}

#define fwrite_complete                fwrite_async

/**
    \brief write to file (async), isr version
    \param process: process handle
    \param file: file handle
    \param block: file block
    \param size: size of transfer
    \retval none. Check corresponding IPC.
*/

__STATIC_INLINE void fiwrite_async(HANDLE process, HANDLE file, HANDLE block, unsigned int size)
{
    IPC ipc;
    ipc.cmd = IPC_WRITE;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = block;
    ipc.param3 = size;
    if (block != INVALID_HANDLE)
        block_isend_ipc(block, process, &ipc);
    else
        ipc_ipost(&ipc);
}

#define fiwrite_complete                fiwrite_async


/**
    \brief write to file
    \param process: process handle
    \param file: file handle
    \param block: file block
    \param size: size of transfer
    \retval size of bytes written
*/

__STATIC_INLINE int fwrite(HANDLE process, HANDLE file, HANDLE block, unsigned int size)
{
    IPC ipc;
    ipc.cmd = IPC_WRITE;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = block;
    ipc.param3 = size;
    if (block != INVALID_HANDLE)
        block_send(block, process);
    if (!call(&ipc))
        error(ipc.param3);
    return ipc.param3;
}

/**
    \brief write direct to file (async)
    \param process: process handle
    \param file: file handle
    \param addr: pointer to data
    \param size: size of transfer
    \retval none. Check corresponding IPC.
*/

__STATIC_INLINE void fdwrite_async(HANDLE process, HANDLE file, void* addr, unsigned int size)
{
    IPC ipc;
    ipc.cmd = IPC_WRITE;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = INVALID_HANDLE;
    ipc.param3 = size;
    direct_enable_read(process, addr, size);
    ipc_post(&ipc);
}

/**
    \brief write to file
    \param process: process handle
    \param file: file handle
    \param addr: pointer to data
    \param size: size of transfer
    \retval size of bytes readed
*/

__STATIC_INLINE int fdwrite(HANDLE process, HANDLE file, void* addr, unsigned int size)
{
    IPC ipc;
    ipc.cmd = IPC_WRITE;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = INVALID_HANDLE;
    ipc.param3 = size;
    direct_enable_read(process, addr, size);
    if (!call(&ipc))
        error(ipc.param3);
    return ipc.param3;
}

/**
    \brief flush file
    \param process: process handle
    \param file: file handle
    \retval true on success
*/

__STATIC_INLINE bool fflush(HANDLE process, HANDLE file)
{
    IPC ipc;
    ipc.cmd = IPC_FLUSH;
    ipc.process = process;
    ipc.param1 = file;
    return call(&ipc);
}

/**
    \brief seek file
    \param process: process handle
    \param file: file handle
    \param pos to seek: file handle
    \retval true on success
*/

__STATIC_INLINE bool fseek(HANDLE process, HANDLE file, unsigned int pos)
{
    IPC ipc;
    ipc.cmd = IPC_SEEK;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = pos;
    return call(&ipc);
}

/**
    \brief cancel IO
    \param process: host process
    \param file: file handle
    \retval none
*/

__STATIC_INLINE void fcancel_io(HANDLE process, HANDLE file)
{
    IPC ipc;
    ipc.cmd = IPC_CANCEL_IO;
    ipc.process = process;
    ipc.param1 = file;
    ipc_post(&ipc);
}

/**
    \brief cancel direct IO, isr version
    \param process: host process
    \param file: file handle
    \retval none
*/

__STATIC_INLINE void ficancel_io(HANDLE process, HANDLE file)
{
    IPC ipc;
    ipc.cmd = IPC_CANCEL_IO;
    ipc.process = process;
    ipc.param1 = file;
    ipc_ipost(&ipc);
}

/** \} */ // end of file group

#endif // FILE_H
