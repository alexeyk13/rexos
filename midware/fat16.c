/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "fat16.h"
#include "../userspace/ipc.h"
#include "../userspace/io.h"
#include "../userspace/error.h"
#include "../userspace/process.h"
#include "../userspace/sys.h"
#include "../userspace/stdio.h"
#include "../userspace/vfs.h"
#include "../userspace/storage.h"
#include "../userspace/disk.h"
#include <string.h>
#include "sys_config.h"

typedef struct {
    IO* io;
    VFS_VOLUME_TYPE volume;
} FAT16_TYPE;

static bool fat16_read(FAT16_TYPE* fat16, uint32_t sector, unsigned size)
{
    if (!storage_read_sync(fat16->volume.hal, fat16->volume.process, fat16->volume.user, fat16->io, sector + fat16->volume.first_sector, size))
    {
        error(ERROR_IO_FAIL);
        return false;
    }
    return true;
}

static inline void fat16_init(FAT16_TYPE* fat16)
{
    fat16->io = io_create(VFS_IO_SIZE);
    fat16->volume.process = INVALID_HANDLE;
}

static bool fat16_mount(FAT16_TYPE* fat16)
{
    if (!fat16_read(fat16, 0, FAT_SECTOR_SIZE))
        return false;
    if (*((uint16_t*)((uint8_t*)io_data(fat16->io) + MBR_MAGIC_OFFSET)) != MBR_MAGIC)
    {
        error(ERROR_NOT_SUPPORTED);
#if (VFS_DEBUG)
        printf("FAT16: Invalid boot sector signature\n");
#endif //VFS_DEBUG
        return false;
    }

    dump(io_data(fat16->io), FAT_SECTOR_SIZE);
    printf("vfs mount ok\n");
    return true;
}

static inline void fat16_open(FAT16_TYPE* fat16, IO* io)
{
    if (fat16->volume.process != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    if (io->data_size < sizeof(VFS_VOLUME_TYPE))
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    memcpy(&fat16->volume, io_data(io), sizeof(VFS_VOLUME_TYPE));
    fat16_mount(fat16);
}

static inline void fat16_close(FAT16_TYPE* fat16)
{
    if (fat16->volume.process == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    //TODO:
}

static inline void fat16_request(FAT16_TYPE* fat16, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        fat16_open(fat16, (IO*)ipc->param2);
        break;
    case IPC_CLOSE:
        fat16_close(fat16);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

void fat16()
{
    IPC ipc;
    FAT16_TYPE fat16;

#if (VFS_DEBUG)
    open_stdout();
#endif
    fat16_init(&fat16);

    for (;;)
    {
        ipc_read(&ipc);
        fat16_request(&fat16, &ipc);
        ipc_write(&ipc);
    }
}
