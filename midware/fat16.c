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
#include "../userspace/so.h"
#include "../userspace/utf.h"
#include <string.h>
#include "sys_config.h"

#define FILE_ENTRIES_IN_SECTOR                              (FAT_SECTOR_SIZE / sizeof(FAT_FILE_ENTRY))
#define FILE_ENTRIES_IN_IO                                  (VFS_IO_SIZE / sizeof(FAT_FILE_ENTRY))
#define SECTORS_IN_IO                                       (VFS_IO_SIZE / FAT_SECTOR_SIZE)

typedef struct {
    unsigned int cluster, index;
} FAT16_FIND_TYPE;

typedef struct {
    IO* io;
    VFS_VOLUME_TYPE volume;
    unsigned long sectors_count, cluster_sectors, root_sectors, reserved_sectors, fat_sectors;
    unsigned long current_sector, current_size;
    SO finds;
} FAT16_TYPE;


static void* fat16_read(FAT16_TYPE* fat16, uint32_t sector, unsigned size)
{
    //cache read
    if (sector >= fat16->current_sector && (fat16->current_sector - sector) * FAT_SECTOR_SIZE + size <= fat16->current_size)
        return (uint8_t*)io_data(fat16->io) + (fat16->current_sector - sector) * FAT_SECTOR_SIZE;
    if (!storage_read_sync(fat16->volume.hal, fat16->volume.process, fat16->volume.user, fat16->io, sector + fat16->volume.first_sector, size))
    {
        error(ERROR_IO_FAIL);
        return NULL;
    }
    return io_data(fat16->io);
}

static unsigned int fat16_strspcpy(char* dst, char* src, unsigned int dst_size, unsigned int src_size, bool lowercase)
{
    unsigned int i, size;
    for (size = src_size; size; --size)
        if (src[size - 1] != ' ')
            break;
    if (size >= dst_size)
        size = dst_size - 1;
    memcpy(dst, src, size);
    dst[size] = '\x0';
    if (lowercase)
    {
        for (i = 0; i < size; ++i)
            if (dst[i] >= 'A' && dst[i] <= 'Z')
                dst[i] += 0x20;
    }
    return size;
}

static inline void fat16_init(FAT16_TYPE* fat16)
{
    fat16->io = io_create(VFS_IO_SIZE + sizeof(STORAGE_STACK));
    fat16->volume.process = INVALID_HANDLE;
    so_create(&fat16->finds, sizeof(FAT16_FIND_TYPE), 1);
}

static bool fat16_parse_boot(FAT16_TYPE* fat16)
{
    FAT_BOOT_SECTOR_BPB_TYPE* bpb;
    void* boot;
    if ((boot = fat16_read(fat16, 0, FAT_SECTOR_SIZE)) == NULL)
        return false;
    if (*((uint16_t*)((uint8_t*)boot + MBR_MAGIC_OFFSET)) != MBR_MAGIC)
    {
        error(ERROR_NOT_SUPPORTED);
#if (VFS_DEBUG_ERRORS)
        printf("FAT16: Invalid boot sector signature\n");
#endif //VFS_DEBUG_ERRORS
        return false;
    }
    bpb = (void*)((uint8_t*)boot + sizeof(FAT_BOOT_SECTOR_HEADER_TYPE));
    if ((bpb->sector_size != FAT_SECTOR_SIZE) || (bpb->fat_count != 2) || (bpb->cluster_size * bpb->sector_size > VFS_IO_SIZE) ||
        (bpb->ext_signature != FAT_BPB_EXT_SIGNATURE) || bpb->reserved_sectors == 0 || bpb->fat_sectors == 0)
    {
        error(ERROR_NOT_SUPPORTED);
#if (VFS_DEBUG_ERRORS)
        printf("FAT16: Unsupported BPB\n");
#endif //VFS_DEBUG_ERRORS
        return false;
    }
    fat16->cluster_sectors = bpb->cluster_size;
    fat16->sectors_count = bpb->sectors_short;
    fat16->root_sectors = (bpb->root_count * sizeof(FAT_FILE_ENTRY) + FAT_SECTOR_SIZE - 1) / FAT_SECTOR_SIZE;
    fat16->reserved_sectors = bpb->reserved_sectors;
    fat16->fat_sectors = bpb->fat_sectors;
    if (fat16->sectors_count == 0)
        fat16->sectors_count = bpb->sectors;
    if (fat16->sectors_count > fat16->volume.sectors_count || fat16->sectors_count == 0)
    {
        error(ERROR_CORRUPTED);
#if (VFS_DEBUG_ERRORS)
        printf("FAT16: Boot sector corrupted\n");
#endif //VFS_DEBUG_ERRORS
        return false;
    }

#if (VFS_DEBUG_INFO)
    printf("FAT16 info:\n");
    printf("cluster size: %d\n", fat16->cluster_sectors * FAT_SECTOR_SIZE);
    printf("total sectors: %d\n", fat16->sectors_count);
    printf("serial No: %08X\n", bpb->serial);
#endif //VFS_DEBUG_INFO
    return true;
}

static FAT_FILE_ENTRY* fat16_read_file_entry_internal(FAT16_TYPE* fat16, unsigned int head_cluster, unsigned int idx)
{
    unsigned int cluster, sector, idx_cur, size;
    FAT_FILE_ENTRY* entries;
    if (head_cluster == VFS_ROOT)
    {
        if (idx >= fat16->root_sectors * FILE_ENTRIES_IN_SECTOR)
            return NULL;
        //actually here not cluster, just block
        cluster = idx / FILE_ENTRIES_IN_IO;
        idx_cur = idx - cluster * FILE_ENTRIES_IN_IO;
        sector = fat16->reserved_sectors + fat16->fat_sectors * 2 + cluster * SECTORS_IN_IO;
        size = VFS_IO_SIZE;
    }
    else
    {
        //TODO: decode cluster
        //TODO: cluster read
        return NULL;
    }
    entries = fat16_read(fat16, sector, size);
    if (entries == NULL)
        return NULL;
    return entries + idx_cur;
}

static uint8_t fat16_lfn_checksum(const uint8_t* name_ext)
{
   unsigned int i;
   uint8_t crc = 0;

   for (i = 11; i; i--)
      crc = ((crc & 1) << 7) + (crc >> 1) + *name_ext++;
   return crc;
}

static bool fat16_read_file_entry(FAT16_TYPE* fat16, VFS_DATA_TYPE* find, unsigned int head_cluster, unsigned int* idx, unsigned int ignore_mask)
{
    FAT_FILE_ENTRY* entry;
    FAT_LFN_ENTRY* lfn;
    uint16_t lfn_chunk[14];
    uint8_t lfn_chunk_latin1[14];
    unsigned int size, lfn_chunk_size;

    uint8_t lfn_seq = 0x00;
    uint8_t lfn_crc = 0x00;
    find->attr = 0;
    lfn_chunk[13] = 0x0000;
    lfn_chunk_latin1[13] = '\x0';
    size = 0;
    for (;;)
    {
        entry = fat16_read_file_entry_internal(fat16, head_cluster, *idx);
        if (entry == NULL)
            return false;
        if ((uint8_t)(entry->name[0]) == FAT_FILE_ENTRY_EMPTY)
            return false;
        if ((uint8_t)(entry->name[0]) == FAT_FILE_ENTRY_MAGIC_ERASED)
        {
            ++(*idx);
            continue;
        }
        //LFN chunk process
        if ((entry->attr & FAT_FILE_ATTR_LFN) == FAT_FILE_ATTR_LFN)
        {
            ++(*idx);
            lfn = (FAT_LFN_ENTRY*)entry;
            //last chunk is first
            if (((lfn->seq & FAT_LFN_SEQ_LAST) && lfn_seq) || (((lfn->seq & FAT_LFN_SEQ_LAST) == 0) && ((lfn->seq & FAT_LFN_SEQ_MASK) != lfn_seq)))
            {
#if (VFS_DEBUG_ERRORS)
                printf("FAT16 warning: LFN wrong sequence\n");
#endif //VFS_DEBUG_ERRORS
                lfn_seq = 0x00;
                size = 0;
                continue;
            }
            if (lfn->seq & FAT_LFN_SEQ_LAST)
            {
                lfn_seq = lfn->seq & FAT_LFN_SEQ_MASK;
                lfn_crc = lfn->checksum;
            }
            else if (lfn->checksum != lfn_crc)
            {
#if (VFS_DEBUG_ERRORS)
                printf("FAT16 warning: LFN checksum not match\n");
#endif //VFS_DEBUG_ERRORS
                lfn_seq = 0x00;
                size = 0;
                continue;
            }
            memcpy(lfn_chunk + 0, lfn->name1, 5 * 2);
            memcpy(lfn_chunk + 5, lfn->name2, 6 * 2);
            memcpy(lfn_chunk + 11, lfn->name3, 2 * 2);
            lfn_chunk_size = utf16_to_latin1(lfn_chunk, lfn_chunk_latin1);
            if (size)
            {
                memmove(find->name + lfn_chunk_size, find->name, size + 1);
                memcpy(find->name, lfn_chunk_latin1, lfn_chunk_size);
            }
            else
                memcpy(find->name, lfn_chunk_latin1, lfn_chunk_size + 1);
            size += lfn_chunk_size;
            if (--lfn_seq == 0)
                lfn_seq = 0xff;
            continue;
        }
        if (entry->attr & ignore_mask)
        {
            ++(*idx);
            continue;
        }
        find->item_handle = entry->first_cluster;
        find->size = entry->size;
        if (entry->attr & FAT_FILE_ATTR_SUBFOLDER)
            find->attr |= VFS_ATTR_FOLDER;
        if (entry->attr & FAT_FILE_ATTR_HIDDEN)
            find->attr |= VFS_ATTR_HIDDEN;
        if (entry->attr & FAT_FILE_ATTR_READ_ONLY)
            find->attr |= VFS_ATTR_READ_ONLY;
        if (lfn_seq == 0xff)
        {
            if (lfn_crc == fat16_lfn_checksum((uint8_t*)entry->name))
                return true;
#if (VFS_DEBUG_ERRORS)
            else
                printf("FAT16 warning: LFN checksum not match\n");
#endif //VFS_DEBUG_ERRORS
        }
#if (VFS_DEBUG_ERRORS)
        else if (lfn_seq)
            printf("FAT16 warning: LFN sequence incomplete\n");
#endif //VFS_DEBUG_ERRORS

        size = fat16_strspcpy(find->name, entry->name, VFS_MAX_FILE_PATH + 1, 8, entry->sys_attr & FAT_FILE_SYS_ATTR_NAME_LOWER_CASE);
        if (entry->ext[0] != ' ')
        {
            find->name[size++] = '.';
            fat16_strspcpy(find->name + size, entry->ext, VFS_MAX_FILE_PATH + 1 - size, 3, entry->sys_attr & FAT_FILE_SYS_ATTR_EXT_LOWER_CASE);
        }
        if (find->name[0] == FAT_FILE_ENTRY_STUFF_MAGIC)
            find->name[0] = FAT_FILE_ENTRY_MAGIC_ERASED;
        return true;

    }
}

static void fat16_unmount_internal(FAT16_TYPE* fat16)
{
    //TODO:
}

static inline void fat16_mount(FAT16_TYPE* fat16, IO* io)
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
    fat16->current_sector = 0xffffffff;
    fat16->current_size = 0;
    if (!fat16_parse_boot(fat16))
        fat16_unmount_internal(fat16);
}

static void fat16_unmount(FAT16_TYPE* fat16)
{
    if (fat16->volume.process == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    fat16_unmount_internal(fat16);
}

static inline void fat16_find_first(FAT16_TYPE* fat16, IPC* ipc)
{
    VFS_DATA_TYPE* find;
    HANDLE fh;
    FAT16_FIND_TYPE* find_handle;
    unsigned int idx;
    IO* io = (IO*)ipc->param2;
    find = io_data(io);

    idx = 0;
    if (!fat16_read_file_entry(fat16, find, ipc->param1, &idx, FAT_FILE_ATTR_LABEL))
    {
        error(ERROR_NOT_FOUND);
        return;
    }

    fh = so_allocate(&fat16->finds);
    if (fh == INVALID_HANDLE)
        return;
    find_handle = so_get(&fat16->finds, fh);
    find_handle->cluster = ipc->param1;
    find_handle->index = idx;

    io->data_size = sizeof(VFS_DATA_TYPE) - VFS_MAX_FILE_PATH + strlen(find->name) + 1;
    io_complete_ex(ipc->process, HAL_IO_CMD(HAL_VFS, VFS_FIND_FIRST), ipc->param1, io, fh);
    error(ERROR_SYNC);
}

static inline void fat16_find_next(FAT16_TYPE* fat16, IPC* ipc)
{
    VFS_DATA_TYPE* find;
    FAT16_FIND_TYPE* find_handle;
    IO* io = (IO*)ipc->param2;
    find = io_data(io);

    find_handle = so_get(&fat16->finds, ipc->param1);
    if (find_handle == NULL)
        return;
    ++find_handle->index;
    if (!fat16_read_file_entry(fat16, find, find_handle->cluster, &find_handle->index, FAT_FILE_ATTR_LABEL))
    {
        error(ERROR_NOT_FOUND);
        return;
    }

    io->data_size = sizeof(VFS_DATA_TYPE) - VFS_MAX_FILE_PATH + strlen(find->name) + 1;
    io_complete_ex(ipc->process, HAL_IO_CMD(HAL_VFS, VFS_FIND_NEXT), ipc->param1, io, ipc->param1);
    error(ERROR_SYNC);
}

static inline void fat16_find_close(FAT16_TYPE* fat16, HANDLE fh)
{
    so_free(&fat16->finds, fh);
}

static inline void fat16_read_volume_label(FAT16_TYPE* fat16, IPC* ipc)
{
    VFS_DATA_TYPE* find;
    IO* io = (IO*)ipc->param2;
    find = io_data(io);
    unsigned int idx;

    idx = 0;
    if (!fat16_read_file_entry(fat16, find, VFS_ROOT, &idx,
                               FAT_FILE_ATTR_READ_ONLY | FAT_FILE_ATTR_HIDDEN | FAT_FILE_ATTR_SYSTEM | FAT_FILE_ATTR_SUBFOLDER))
    {
        error(ERROR_NOT_FOUND);
        return;
    }

    io->data_size = sizeof(VFS_DATA_TYPE) - VFS_MAX_FILE_PATH + strlen(find->name) + 1;
    io_complete_ex(ipc->process, HAL_IO_CMD(HAL_VFS, VFS_READ_VOLUME_LABEL), ipc->param1, io, ipc->param1);
    error(ERROR_SYNC);
}

static inline void fat16_request(FAT16_TYPE* fat16, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case VFS_MOUNT:
        fat16_mount(fat16, (IO*)ipc->param2);
        break;
    case VFS_UNMOUNT:
        fat16_unmount(fat16);
        break;
    case VFS_FIND_FIRST:
        fat16_find_first(fat16, ipc);
        break;
    case VFS_FIND_NEXT:
        fat16_find_next(fat16, ipc);
        break;
    case VFS_FIND_CLOSE:
        fat16_find_close(fat16, (HANDLE)ipc->param1);
        break;
    case VFS_READ_VOLUME_LABEL:
        fat16_read_volume_label(fat16, ipc);
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

#if (VFS_DEBUG_INFO) || (VFS_DEBUG_ERRORS)
    open_stdout();
#endif //(VFS_DEBUG_INFO) || (VFS_DEBUG_ERRORS)
    fat16_init(&fat16);

    for (;;)
    {
        ipc_read(&ipc);
        fat16_request(&fat16, &ipc);
        ipc_write(&ipc);
    }
}
