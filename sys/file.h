/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef FILE_H
#define FILE_H

#include "sys.h"
#include "../userspace/direct.h"
#include "../userspace/block.h"

/** \addtogroup file file
    \{
 */


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
    block_send_ipc(block, process, &ipc);
}

/**
    \brief read from file
    \param process: process handle
    \param file: file handle
    \param block: file block
    \param size: size of transfer
    \retval true on success
*/

__STATIC_INLINE bool fread(HANDLE process, HANDLE file, HANDLE block, unsigned int size)
{
    IPC ipc;
    ipc.cmd = IPC_READ;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = block;
    ipc.param3 = size;
    block_send_ipc(block, process, &ipc);
    ipc_read_ms(&ipc, 0, process);
    return ipc.cmd == IPC_READ_COMPLETE;
}


/**
    \brief read from file (async) with no data
    \param process: process handle
    \param file: file handle
    \retval none. Check corresponding IPC.
*/

__STATIC_INLINE void fread_async_null(HANDLE process, HANDLE file)
{
    IPC ipc;
    ipc.cmd = IPC_READ;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = INVALID_HANDLE;
    ipc.param3 = 0;
    ipc_post(&ipc);
}

/**
    \brief read from file with no data
    \param process: process handle
    \param file: file handle
    \retval true on success
*/

__STATIC_INLINE bool fread_null(HANDLE process, HANDLE file)
{
    IPC ipc;
    ipc.cmd = IPC_READ;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = INVALID_HANDLE;
    ipc.param3 = 0;
    ipc_post(&ipc);
    ipc_read_ms(&ipc, 0, process);
    return ipc.cmd == IPC_READ_COMPLETE;
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
    block_send_ipc(block, process, &ipc);
}

/**
    \brief write to file
    \param process: process handle
    \param file: file handle
    \param block: file block
    \param size: size of transfer
    \retval true on success
*/

__STATIC_INLINE bool fwrite(HANDLE process, HANDLE file, HANDLE block, unsigned int size)
{
    IPC ipc;
    ipc.cmd = IPC_WRITE;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = block;
    ipc.param3 = size;
    block_send_ipc(block, process, &ipc);
    ipc_read_ms(&ipc, 0, process);
    return ipc.cmd == IPC_WRITE_COMPLETE;
}

/**
    \brief write to file (async) with no data
    \param process: process handle
    \param file: file handle
    \retval none. Check corresponding IPC.
*/

__STATIC_INLINE void fwrite_async_null(HANDLE process, HANDLE file)
{
    IPC ipc;
    ipc.cmd = IPC_WRITE;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = INVALID_HANDLE;
    ipc.param3 = 0;
    ipc_post(&ipc);
}

/**
    \brief write to file (async) with no data
    \param process: process handle
    \param file: file handle
    \retval true on success
*/

__STATIC_INLINE bool fwrite_null(HANDLE process, HANDLE file)
{
    IPC ipc;
    ipc.cmd = IPC_WRITE;
    ipc.process = process;
    ipc.param1 = file;
    ipc.param2 = INVALID_HANDLE;
    ipc.param3 = 0;
    ipc_post(&ipc);
    ipc_read_ms(&ipc, 0, process);
    return ipc.cmd == IPC_WRITE_COMPLETE;
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

/** \} */ // end of file group

#endif // FILE_H
