/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "eep.h"
#include "sys_config.h"
#include "ipc.h"
#include "object.h"
#include <string.h>
#include "io.h"
#include "core/core.h"

#ifdef EXODRIVERS
bool eep_seek(unsigned int addr)
{
    return get_handle_exo(HAL_REQ(HAL_EEPROM, IPC_SEEK), 0, addr, 0) != INVALID_HANDLE;
}

int eep_read(unsigned int offset, void* buf, unsigned int size)
{
    int res;
    IO* io = io_create(size);
    res = io_read_sync_exo(HAL_IO_REQ(HAL_EEPROM, IPC_READ), offset, io, size);
    if (res > 0)
        memcpy(buf, io_data(io), res);
    io_destroy(io);
    return res;
}

int eep_write(unsigned int offset, const void* buf, unsigned int size)
{
    int res;
    IO* io = io_create(size);
    if (io == NULL)
        return (get_last_error());
    io_data_write(io, buf, size);
    res = io_write_sync_exo(HAL_IO_REQ(HAL_EEPROM, IPC_WRITE), 0, io);
    io_destroy(io);
    return res;
}
#else
bool eep_seek(unsigned int addr)
{
    return get_handle(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_EEPROM, IPC_SEEK), 0, addr, 0) != INVALID_HANDLE;
}

int eep_read(unsigned int offset, void* buf, unsigned int size)
{
    int res;
    IO* io = io_create(size);
    res = io_read_sync(object_get(SYS_OBJ_CORE), HAL_IO_REQ(HAL_EEPROM, IPC_READ), offset, io, size);
    if (res > 0)
        memcpy(buf, io_data(io), res);
    io_destroy(io);
    return res;
}

int eep_write(unsigned int offset, const void* buf, unsigned int size)
{
    int res;
    IO* io = io_create(size);
    if (io == NULL)
        return (get_last_error());
    io_data_write(io, buf, size);
    res = io_write_sync(object_get(SYS_OBJ_CORE), HAL_IO_REQ(HAL_EEPROM, IPC_WRITE), 0, io);
    io_destroy(io);
    return res;
}
#endif //EXODRIVERS
