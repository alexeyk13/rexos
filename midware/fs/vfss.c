/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "vfss.h"
#include "vfss_private.h"
#include "fat16.h"
#include "ber.h"
#include "../../userspace/stdio.h"
#include "../../userspace/sys.h"
#include "../../userspace/ipc.h"
#include "sys_config.h"
#include <string.h>

void* vfss_get_buf(VFSS_TYPE* vfss)
{
    return io_data(vfss->io);
}

unsigned int vfss_get_buf_size(VFSS_TYPE* vfss)
{
    return vfss->io_size;
}

void vfss_resize_buf(VFSS_TYPE* vfss, unsigned int size)
{
    if (size > vfss->io_size)
    {
        io_destroy(vfss->io);
        vfss->io = io_create(size + sizeof(STORAGE_STACK));
        vfss->io_size = size;
    }
}

unsigned int vfss_get_volume_offset(VFSS_TYPE* vfss)
{
#if (VFS_BER)
    if (vfss->volume.sector_mode == SECTOR_MODE_BER)
        return 0;
#endif //VFS_BER
    return vfss->volume.first_sector;
}

unsigned int vfss_get_volume_sectors(VFSS_TYPE* vfss)
{
#if (VFS_BER)
    if (vfss->volume.sector_mode == SECTOR_MODE_BER)
        return ber_get_volume_sectors(vfss);
#endif //VFS_BER
    return vfss->volume.sectors_count;
}

void* vfss_read_sectors(VFSS_TYPE* vfss, unsigned long sector, unsigned size)
{
    bool res;
    //cache read
    if ((sector == vfss->current_sector) && (vfss->io->data_size == size))
        return io_data(vfss->io);
#if (VFS_BER)
    if (vfss->volume.sector_mode == SECTOR_MODE_BER)
        res = ber_read_sectors(vfss, sector, size);
    else
#endif //VFS_BER
        res = storage_read_sync(vfss->volume.hal, vfss->volume.process, vfss->volume.user, vfss->io, sector + vfss->volume.first_sector, size);
    if (!res)
        return NULL;
    vfss->current_sector = sector;
    return io_data(vfss->io);
}

bool vfss_write_sectors(VFSS_TYPE* vfss, unsigned long sector, unsigned size)
{
    bool res;
#if (VFS_BER)
    if (vfss->volume.sector_mode == SECTOR_MODE_BER)
        res = ber_write_sectors(vfss, sector, size);
    else
#endif //VFS_BER
    {
        vfss->io->data_size = size;
        res = storage_write_sync(vfss->volume.hal, vfss->volume.process, vfss->volume.user, vfss->io, sector + vfss->volume.first_sector);
    }
    if (res)
        vfss->current_sector = sector;
    return res;
}

bool vfss_zero_sectors(VFSS_TYPE* vfss, unsigned long sector, unsigned count)
{
    unsigned long i, io_sectors, sectors_to_zero;
    memset(io_data(vfss->io), 0, vfss->io_size);
    io_sectors = vfss->io_size / FAT_SECTOR_SIZE;
    for (i = 0; i < count; i += sectors_to_zero)
    {
        sectors_to_zero = count - i;
        if (sectors_to_zero > io_sectors)
            sectors_to_zero = io_sectors;
        if (!vfss_write_sectors(vfss, sector + i, sectors_to_zero * FAT_SECTOR_SIZE))
            return false;
    }
    return true;
}


static inline void vfss_open_volume(VFSS_TYPE* vfss, IO* io)
{
    if (vfss->volume.process != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    if (io->data_size < sizeof(VFS_VOLUME_TYPE))
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    memcpy(&vfss->volume, io_data(io), sizeof(VFS_VOLUME_TYPE));
    vfss->current_sector = 0xffffffff;
    vfss->current_size = 0;
}

static inline void vfss_close_volume(VFSS_TYPE* vfss)
{
    vfss->volume.process = INVALID_HANDLE;
}

static inline void vfss_init(VFSS_TYPE* vfss)
{
    vfss->volume.process = INVALID_HANDLE;
    vfss->io = io_create(FAT_SECTOR_SIZE + sizeof(STORAGE_STACK));
    vfss->io_size = FAT_SECTOR_SIZE;
#if (VFS_BER)
    ber_init(vfss);
#endif //VFS_BER
#if (VFS_SFS)
    sfs_init(vfss);
#else
    fat16_init(vfss);
#endif  // VFS_SFS
}

void vfss_request(VFSS_TYPE *vfss, IPC* ipc)
{
    if ((HAL_ITEM(ipc->cmd) == IPC_OPEN) && (ipc->param1 == VFS_VOLUME_HANDLE))
    {
        vfss_open_volume(vfss, (IO*)ipc->param2);
        return;
    }
    if (vfss->volume.process == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    //vfss request
    if ((HAL_ITEM(ipc->cmd) == IPC_CLOSE) && (ipc->param1 == VFS_VOLUME_HANDLE))
    {
        vfss_close_volume(vfss);
        return;
    }
#if (VFS_BER)
    if ((vfss->volume.sector_mode == SECTOR_MODE_BER) && (ipc->param1 == VFS_BER_HANDLE))
    {
        ber_request(vfss, ipc);
        return;
    }
#endif //VFS_BER
    //forward to fs
#if (VFS_SFS)
    sfs_request(vfss, ipc);
#else
    fat16_request(vfss, ipc);
#endif  // VFS_SFS
}

void vfss()
{
    IPC ipc;
    VFSS_TYPE vfss;

#if (VFS_DEBUG_INFO) || (VFS_DEBUG_ERRORS)
    open_stdout();
#endif //(VFS_DEBUG_INFO) || (VFS_DEBUG_ERRORS)

    vfss_init(&vfss);

    for (;;)
    {
        ipc_read(&ipc);
        vfss_request(&vfss, &ipc);
        ipc_write(&ipc);
    }
}

