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
#include "../userspace/time.h"
#include <string.h>
#include "sys_config.h"

#define FILE_ENTRIES_IN_SECTOR                              (FAT_SECTOR_SIZE / sizeof(FAT_FILE_ENTRY))
#define SECTORS_IN_IO                                       (VFS_IO_SIZE / FAT_SECTOR_SIZE)
#define FAT_ENTRIES_IN_SECTOR                               (FAT_SECTOR_SIZE / 2)

#define FAT_DATE_DAY_POS                                    0
#define FAT_DATE_DAY_MASK                                   (0x1f << 0)
#define FAT_DATE_MON_POS                                    5
#define FAT_DATE_MON_MASK                                   (0xf << 5)
#define FAT_DATE_YEAR_POS                                   9
#define FAT_DATE_YEAR_MASK                                  (0x7f << 9)
#define FAT_DATE_EPOCH_YEAR                                 1980

#define FAT_TIME_HALF_SECONDS_POS                           0
#define FAT_TIME_HALF_SECONDS_MASK                          (0x1f << 0)
#define FAT_TIME_MINUTES_POS                                5
#define FAT_TIME_MINUTES_MASK                               (0x3f << 0)
#define FAT_TIME_HOURS_POS                                  11
#define FAT_TIME_HOURS_MASK                                 (0x1f << 0)

typedef struct {
    unsigned int cluster, index, cluster_num;
} FAT16_FIND_TYPE;

typedef struct {
    unsigned int first_cluster, current_cluster, pos, size;
    FAT16_FIND_TYPE ff;
} FAT16_FILE_HANDLE_TYPE;

typedef struct {
    IO* io;
    VFS_VOLUME_TYPE volume;
    unsigned long sectors_count, cluster_sectors, root_sectors, reserved_sectors, fat_sectors, cluster_size, clusters_count;
    unsigned long current_sector, current_size;
    SO finds;
    SO file_handles;
} FAT16_TYPE;


static bool fat16_read_sectors(FAT16_TYPE* fat16, unsigned long sector, unsigned size)
{
    //cache read
    if ((sector == fat16->current_sector) && (fat16->io->data_size == size))
        return true;
    if (!storage_read_sync(fat16->volume.hal, fat16->volume.process, fat16->volume.user, fat16->io, sector + fat16->volume.first_sector, size))
        return false;
    fat16->current_sector = sector;
    return true;
}

static bool fat16_write_sectors(FAT16_TYPE* fat16, unsigned long sector, unsigned size)
{
    fat16->io->data_size = size;
    return storage_write_sync(fat16->volume.hal, fat16->volume.process, fat16->volume.user, fat16->io, sector + fat16->volume.first_sector);
}

/*
static unsigned short fat16_time_to_fat_date(struct tm* ts)
{
    return ((ts->tm_year - FAT_DATE_EPOCH_YEAR) << FAT_DATE_YEAR_POS) | ((ts->tm_mon + 1) << FAT_DATE_MON_POS) | (ts->tm_mday << FAT_DATE_DAY_POS);
}

static unsigned short fat16_time_to_fat_time(struct tm* ts)
{
    return (ts->tm_hour << FAT_TIME_HOURS_POS) | (ts->tm_min << FAT_TIME_MINUTES_POS) | ((ts->tm_sec >> 1) << FAT_DATE_DAY_POS);
}
*/
static unsigned long fat16_cluster_to_sector(FAT16_TYPE* fat16, unsigned long cluster)
{
    return fat16->reserved_sectors + fat16->fat_sectors * 2 + fat16->root_sectors + (cluster - 2) * fat16->cluster_sectors;
}

static unsigned long fat16_get_fat_value(FAT16_TYPE* fat16, unsigned long cluster)
{
    uint16_t* fat;
    if (!fat16_read_sectors(fat16, fat16->reserved_sectors + (cluster / FAT_ENTRIES_IN_SECTOR), FAT_SECTOR_SIZE))
        return FAT_CLUSTER_RESERVED;
    fat = io_data(fat16->io);
    return fat[cluster % FAT_ENTRIES_IN_SECTOR];
}

static unsigned long fat16_get_fat_next(FAT16_TYPE* fat16, unsigned long cluster)
{
    unsigned long next;
    next = fat16_get_fat_value(fat16, cluster);
    if (next >= FAT_CLUSTER_RESERVED || next < 2)
    {
#if (VFS_DEBUG_ERRORS)
        printf("FAT16 warning: FAT corrupted\n");
#endif //VFS_DEBUG_ERRORS
        error(ERROR_CORRUPTED);
    }
    return next;
}

static unsigned long fat16_get_fat_chain(FAT16_TYPE* fat16, unsigned long first_cluster, unsigned long i)
{
    unsigned long res;
    for(res = first_cluster; i; --i)
    {
        res = fat16_get_fat_next(fat16, res);
        if (res >= FAT_CLUSTER_RESERVED)
            break;
    }
    return res;
}

static bool fat16_set_fat_value(FAT16_TYPE* fat16, unsigned long cluster, unsigned long value)
{
    uint16_t* fat;
    if (!fat16_read_sectors(fat16, fat16->reserved_sectors + (cluster / FAT_ENTRIES_IN_SECTOR), FAT_SECTOR_SIZE))
        return false;
    fat = io_data(fat16->io);
    fat[cluster % FAT_ENTRIES_IN_SECTOR] = value;
    //first copy
    if (!fat16_write_sectors(fat16, fat16->reserved_sectors + (cluster / FAT_ENTRIES_IN_SECTOR), FAT_SECTOR_SIZE))
        return false;
    //second copy
    return fat16_write_sectors(fat16, fat16->reserved_sectors + fat16->fat_sectors + (cluster / FAT_ENTRIES_IN_SECTOR), FAT_SECTOR_SIZE);
}

static unsigned long fat16_find_free_cluster(FAT16_TYPE* fat16, unsigned long cluster)
{
    unsigned long next_free;
    //after current till last
    for (next_free = cluster + 1; next_free < fat16->clusters_count;  ++next_free)
        if (fat16_get_fat_value(fat16, next_free) < FAT_CLUSTER_RESERVED)
            return next_free;
    //from first till current -1
    for (next_free = 2; next_free < cluster;  ++next_free)
        if (fat16_get_fat_value(fat16, next_free) < FAT_CLUSTER_RESERVED)
            return next_free;
    return FAT_CLUSTER_RESERVED;
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
    so_create(&fat16->file_handles, sizeof(FAT16_FILE_HANDLE_TYPE), 1);
}

static bool fat16_parse_boot(FAT16_TYPE* fat16)
{
    FAT_BOOT_SECTOR_BPB_TYPE* bpb;
    uint8_t* boot;
    if (!fat16_read_sectors(fat16, 0, FAT_SECTOR_SIZE))
        return false;
    boot = io_data(fat16->io);
    if (*((uint16_t*)(boot + MBR_MAGIC_OFFSET)) != MBR_MAGIC)
    {
        error(ERROR_NOT_SUPPORTED);
#if (VFS_DEBUG_ERRORS)
        printf("FAT16: Invalid boot sector signature\n");
#endif //VFS_DEBUG_ERRORS
        return false;
    }
    bpb = (void*)(boot + sizeof(FAT_BOOT_SECTOR_HEADER_TYPE));
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
    if (fat16->sectors_count > fat16->volume.sectors_count || fat16->sectors_count == 0 || fat16->cluster_sectors == 0)
    {
        error(ERROR_CORRUPTED);
#if (VFS_DEBUG_ERRORS)
        printf("FAT16: Boot sector corrupted\n");
#endif //VFS_DEBUG_ERRORS
        return false;
    }
    fat16->clusters_count = (fat16->sectors_count - (fat16->reserved_sectors + fat16->fat_sectors * 2 + fat16->root_sectors)) / fat16->cluster_sectors + 2;
    fat16->cluster_size = fat16->cluster_sectors * FAT_SECTOR_SIZE;

#if (VFS_DEBUG_INFO)
    printf("FAT16 info:\n");
    printf("cluster size: %d\n", fat16->cluster_sectors * FAT_SECTOR_SIZE);
    printf("total sectors: %d\n", fat16->sectors_count);
    printf("serial No: %08X\n", bpb->serial);
#endif //VFS_DEBUG_INFO

    return true;
}

static FAT_FILE_ENTRY* fat16_read_file_entry(FAT16_TYPE* fat16, FAT16_FIND_TYPE* ff)
{
    unsigned int sector_num, cluster_num, sector, idx_cur;
    FAT_FILE_ENTRY* entries;
    if (ff->cluster == VFS_ROOT)
    {
        if (ff->index >= fat16->root_sectors * FILE_ENTRIES_IN_SECTOR)
            return NULL;
        sector_num = ff->index / FILE_ENTRIES_IN_SECTOR;
        idx_cur = ff->index - sector_num * FILE_ENTRIES_IN_SECTOR;
        sector = fat16->reserved_sectors + fat16->fat_sectors * 2 + sector_num;
    }
    else
    {
        cluster_num = ff->index / (FILE_ENTRIES_IN_SECTOR * fat16->cluster_sectors);
        if (cluster_num != ff->cluster_num)
        {
            ff->cluster_num = cluster_num;
            ff->cluster = fat16_get_fat_next(fat16, ff->cluster);
            if (ff->cluster >= FAT_CLUSTER_RESERVED)
                return NULL;
        }
        sector_num = (ff->index - FILE_ENTRIES_IN_SECTOR * fat16->cluster_sectors * cluster_num) / FILE_ENTRIES_IN_SECTOR;
        sector = fat16_cluster_to_sector(fat16, ff->cluster) + sector_num;
        idx_cur = ff->index - (cluster_num * fat16->cluster_sectors + sector_num) * FILE_ENTRIES_IN_SECTOR;
    }
    if (!fat16_read_sectors(fat16, sector, FAT_SECTOR_SIZE))
        return NULL;
    entries = io_data(fat16->io);
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

static bool fat16_get_file_name(FAT16_TYPE* fat16, char* name, FAT16_FIND_TYPE* ff, unsigned int mask, unsigned int ignore_mask)
{
    FAT_FILE_ENTRY* entry;
    FAT_LFN_ENTRY* lfn;
    uint16_t lfn_chunk[14];
    uint8_t lfn_chunk_latin1[14];
    unsigned int size, lfn_chunk_size;

    uint8_t lfn_seq = 0x00;
    uint8_t lfn_crc = 0x00;
    lfn_chunk[13] = 0x0000;
    lfn_chunk_latin1[13] = '\x0';
    size = 0;
    for (;;)
    {
        entry = fat16_read_file_entry(fat16, ff);
        if (entry == NULL)
            return false;
        if ((uint8_t)(entry->name[0]) == FAT_FILE_ENTRY_EMPTY)
            return false;
        if ((uint8_t)(entry->name[0]) == FAT_FILE_ENTRY_MAGIC_ERASED)
        {
            ++ff->index;
            continue;
        }
        //LFN chunk process
        if ((entry->attr & FAT_FILE_ATTR_LFN) == FAT_FILE_ATTR_LFN)
        {
            ++ff->index;
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
                memmove(name + lfn_chunk_size, name, size + 1);
                memcpy(name, lfn_chunk_latin1, lfn_chunk_size);
            }
            else
                memcpy(name, lfn_chunk_latin1, lfn_chunk_size + 1);
            size += lfn_chunk_size;
            if (--lfn_seq == 0)
                lfn_seq = 0xff;
            continue;
        }
        if (((entry->attr & (mask & FAT_FILE_ATTR_REAL_MASK)) != (mask & FAT_FILE_ATTR_REAL_MASK)) ||
            ((mask & FAT_FILE_ATTR_DOT_OR_DOT_DOT) && entry->name[0] != '.') ||
            ((ignore_mask & FAT_FILE_ATTR_DOT_OR_DOT_DOT) && entry->name[0] == '.') ||
            (entry->attr & (ignore_mask & FAT_FILE_ATTR_REAL_MASK)))
        {
            ++ff->index;
            lfn_seq = 0x00;
            size = 0;
            continue;
        }
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

        size = fat16_strspcpy(name, entry->name, VFS_MAX_FILE_PATH + 1, 8, entry->sys_attr & FAT_FILE_SYS_ATTR_NAME_LOWER_CASE);
        if (entry->ext[0] != ' ')
        {
            name[size++] = '.';
            fat16_strspcpy(name + size, entry->ext, VFS_MAX_FILE_PATH + 1 - size, 3, entry->sys_attr & FAT_FILE_SYS_ATTR_EXT_LOWER_CASE);
        }
        if (name[0] == FAT_FILE_ENTRY_STUFF_MAGIC)
            name[0] = FAT_FILE_ENTRY_MAGIC_ERASED;
        return true;

    }
}

static unsigned int fat16_decode_attr(unsigned int fat_attr)
{
    unsigned int res = 0;
    if (fat_attr & FAT_FILE_ATTR_SUBFOLDER)
        res |= VFS_ATTR_FOLDER;
    if (fat_attr & FAT_FILE_ATTR_HIDDEN)
        res |= VFS_ATTR_HIDDEN;
    if (fat_attr & FAT_FILE_ATTR_READ_ONLY)
        res |= VFS_ATTR_READ_ONLY;
    return res;
}

static bool fat16_find_by_name(FAT16_TYPE* fat16, FAT16_FIND_TYPE* ff, const char* name, unsigned int mask, unsigned int ignore_mask)
{
    char name_cur[VFS_MAX_FILE_PATH + 1];
    ff->cluster_num = 0;
    ff->index = 0;
    for (;;)
    {
        if (!fat16_get_file_name(fat16, name_cur, ff, mask, ignore_mask))
            return false;
        if (strcmp(name_cur, name) == 0)
            return true;
        ++ff->index;
    }
}

static bool fat16_get_by_name(FAT16_TYPE* fat16, unsigned int* folder, const char* name, unsigned int mask, unsigned int ignore_mask)
{
    FAT16_FIND_TYPE ff;
    FAT_FILE_ENTRY* entry;
    ff.cluster = *folder;
    if (!fat16_find_by_name(fat16, &ff, name, mask, ignore_mask))
    {
        error(ERROR_NOT_FOUND);
        return false;
    }
    entry = fat16_read_file_entry(fat16, &ff);
    *folder = entry->first_cluster;
    return true;
}

static bool fat16_get_parent_folder(FAT16_TYPE* fat16, unsigned int *folder)
{
    if (*folder == VFS_ROOT)
        return true;
    return fat16_get_by_name(fat16, folder, VFS_PARENT_FOLDER,  FAT_FILE_ATTR_DOT_OR_DOT_DOT, FAT_FILE_ATTR_LABEL);
}

static bool fat16_get_by_path(FAT16_TYPE* fat16, char* path, unsigned int* folder)
{
    unsigned int i;
    char* path_cur;
    //request current path
    if (path[0] == '\x0')
        return true;
    if (path[0] == VFS_FOLDER_DELIMITER)
    {
        *folder = VFS_ROOT;
        path++;
    }
    while (path[0] != '\x0')
    {
        path_cur = path;
        for(i = 0; path[i] != '\x0' && path[i] != VFS_FOLDER_DELIMITER; ++i) {}
        //wrong path, double /
        if (i == 0)
        {
            error(ERROR_INVALID_PARAMS);
            return false;
        }
        if (path[i] == '\x0')
            path += i;
        else
        {
            path[i] = '\x0';
            path += i + 1;
        }
        if (strcmp(path_cur, VFS_CURRENT_FOLDER) == 0)
            continue;
        if (strcmp(path_cur, VFS_PARENT_FOLDER) == 0)
        {
            if (!fat16_get_parent_folder(fat16, folder))
                return false;
            continue;
        }
        if (!fat16_get_by_name(fat16, folder, path_cur, FAT_FILE_ATTR_SUBFOLDER, FAT_FILE_ATTR_LABEL))
            return false;
    }
    return true;
}

static char* fat16_extract_file_from_path(char* path)
{
    char* file;
    int i;
    for (i = strlen(path) - 1; i >= 0 && path[i] != VFS_FOLDER_DELIMITER; --i) {}
    file = path + i + 1;
    if (strcmp(file, VFS_CURRENT_FOLDER) == 0)
        return NULL;
    if (strcmp(file, VFS_PARENT_FOLDER) == 0)
        return NULL;
    if (i >= 0)
        path[i] = '\x0';
    return file;
}

static void fat16_close_file(FAT16_TYPE* fat16, HANDLE fh)
{
    so_free(&fat16->file_handles, fh);
}

static void fat16_unmount_internal(FAT16_TYPE* fat16)
{
    HANDLE handle;
    //1. free finds
    while((handle = so_first(&fat16->finds)) != INVALID_HANDLE)
        so_free(&fat16->finds, handle);
    //2. free file_handles
    while((handle = so_first(&fat16->file_handles)) != INVALID_HANDLE)
        fat16_close_file(fat16, handle);
    //3. unmount volume
    fat16->volume.process = INVALID_HANDLE;
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

static inline void fat16_find_first(FAT16_TYPE* fat16, unsigned int folder, HANDLE process)
{
    HANDLE fh;
    FAT16_FIND_TYPE* ff;

    if (fat16->volume.process == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }

    fh = so_allocate(&fat16->finds);
    if (fh == INVALID_HANDLE)
        return;
    ff = so_get(&fat16->finds, fh);
    ff->cluster = folder;
    ff->cluster_num = 0;
    ff->index = 0;
    ipc_post_inline(process, HAL_CMD(HAL_VFS, VFS_FIND_FIRST), folder, fh, 0);
    error(ERROR_SYNC);
}

static inline void fat16_find_next(FAT16_TYPE* fat16, IPC* ipc)
{
    VFS_FIND_TYPE* find;
    FAT16_FIND_TYPE* ff;
    FAT_FILE_ENTRY* entry;
    IO* io = (IO*)ipc->param2;

    find = io_data(io);
    ff = so_get(&fat16->finds, ipc->param1);
    if (ff == NULL)
        return;
    if (!fat16_get_file_name(fat16, find->name, ff, 0, FAT_FILE_ATTR_LABEL | FAT_FILE_ATTR_DOT_OR_DOT_DOT))
    {
        error(ERROR_NOT_FOUND);
        return;
    }
    entry = fat16_read_file_entry(fat16, ff);
    find->item = entry->first_cluster;
    find->size = entry->size;
    find->attr = fat16_decode_attr(entry->attr);
    ++ff->index;

    io->data_size = sizeof(VFS_FIND_TYPE) - VFS_MAX_FILE_PATH + strlen(find->name) + 1;
    io_complete_ex(ipc->process, HAL_IO_CMD(HAL_VFS, VFS_FIND_NEXT), ipc->param1, io, ipc->param1);
    error(ERROR_SYNC);
}

static void fat16_find_close(FAT16_TYPE* fat16, HANDLE fh)
{
    so_free(&fat16->finds, fh);
}

static inline void fat16_cd_up(FAT16_TYPE* fat16, unsigned int folder, HANDLE process)
{
    unsigned int parent = folder;

    if (fat16->volume.process == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }

    if (!fat16_get_parent_folder(fat16, &parent))
        return;
    ipc_post_inline(process, HAL_CMD(HAL_VFS, VFS_CD_UP), folder, parent, 0);
    error(ERROR_SYNC);
}

static inline void fat16_cd_path(FAT16_TYPE* fat16, unsigned int folder, IO* io, HANDLE process)
{
    unsigned int res;
    char* path = io_data(io);
    if (fat16->volume.process == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    if (strlen(path) > VFS_MAX_FILE_PATH)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    res = folder;
    if (!fat16_get_by_path(fat16, path, &res))
        return;
    *((unsigned int*)io_data(io)) = res;
    io->data_size = sizeof(unsigned int);
    io_complete(process, HAL_IO_CMD(HAL_VFS, VFS_CD_PATH), folder, io);
    error(ERROR_SYNC);
}

static inline void fat16_read_volume_label(FAT16_TYPE* fat16, IPC* ipc)
{
    char* name;
    FAT16_FIND_TYPE ff;
    IO* io = (IO*)ipc->param2;

    if (fat16->volume.process == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }

    name = io_data(io);
    ff.cluster = VFS_ROOT;
    ff.cluster_num = 0;
    ff.index = 0;
    if (!fat16_get_file_name(fat16, name, &ff, FAT_FILE_ATTR_LABEL, FAT_FILE_ATTR_DOT_OR_DOT_DOT))
    {
        error(ERROR_NOT_FOUND);
        return;
    }
    io->data_size = strlen(name) + 1;
    io_complete_ex(ipc->process, HAL_IO_CMD(HAL_VFS, VFS_READ_VOLUME_LABEL), ipc->param1, io, ipc->param1);
    error(ERROR_SYNC);
}

static inline void fat16_open_file(FAT16_TYPE* fat16, unsigned int folder, IO* io, HANDLE process)
{
    VFS_OPEN_TYPE* ot;
    FAT16_FIND_TYPE ff;
    HANDLE fh;
    FAT16_FILE_HANDLE_TYPE* h;
    char* file;
    FAT_FILE_ENTRY* entry;
    ff.cluster = folder;

    if (fat16->volume.process == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    ot = io_data(io);
    if (strlen(ot->name) > VFS_MAX_FILE_PATH || (ot->mode & (VFS_MODE_READ | VFS_MODE_WRITE)) == 0)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    file = fat16_extract_file_from_path(ot->name);
    if (file == NULL)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (!fat16_get_by_path(fat16, ot->name, &ff.cluster))
        return;
    if (!fat16_find_by_name(fat16, &ff, file, 0, FAT_FILE_ATTR_LABEL | FAT_FILE_ATTR_SUBFOLDER))
    {
        //TODO: if mode write, create file
        error(ERROR_NOT_FOUND);
        return;
    }
    fh = so_allocate(&fat16->file_handles);
    if (fh == INVALID_HANDLE)
        return;
    h = so_get(&fat16->file_handles, fh);
    h->ff.cluster = ff.cluster;
    h->ff.cluster_num = ff.cluster_num;
    h->ff.index = ff.index;
    h->pos = 0;
    entry = fat16_read_file_entry(fat16, &ff);
    h->first_cluster = h->current_cluster = entry->first_cluster;
    h->size = entry->size;

    *((HANDLE*)io_data(io)) = fh;
    io->data_size = sizeof(HANDLE);
    io_complete(process, HAL_IO_CMD(HAL_VFS, IPC_OPEN), folder, io);
    error(ERROR_SYNC);
}

static inline void fat16_seek_file(FAT16_TYPE* fat16, HANDLE fh, unsigned int pos)
{
    FAT16_FILE_HANDLE_TYPE* h;
    unsigned int cluster_num;
    h = so_get(&fat16->file_handles, fh);
    if (h == NULL)
        return;
    if (pos > h->size)
    {
        h->pos = h->size;
        error(ERROR_EOF);
        return;
    }
    h->pos = pos;
    if (h->pos == h->size)
        cluster_num = (h->size - 1) / fat16->cluster_size;
    else
        cluster_num = h->pos / fat16->cluster_size;
    h->current_cluster = fat16_get_fat_chain(fat16, h->first_cluster, cluster_num);
    if (h->current_cluster >= FAT_CLUSTER_RESERVED)
    {
        h->current_cluster = h->first_cluster;
        h->pos = 0;
    }
}

static inline void fat16_read_file(FAT16_TYPE* fat16, HANDLE fh, IO* io, unsigned int size, HANDLE process)
{
    FAT16_FILE_HANDLE_TYPE* h;
    unsigned int cluster_offset, sector_offset, chunk, sector, sectors_count, next_cluster;
    h = so_get(&fat16->file_handles, fh);
    if (h == NULL)
        return;
    if (h->pos >= h->size)
    {
        error(ERROR_EOF);
        return;
    }
    if (h->size - h->pos < size)
        size = h->size - h->pos;
    io->data_size = 0;
    while(size)
    {
        cluster_offset = h->pos % fat16->cluster_size;
        sector_offset = cluster_offset % FAT_SECTOR_SIZE;
        sector = cluster_offset / FAT_SECTOR_SIZE;
        sectors_count = (size + sector_offset + FAT_SECTOR_SIZE - 1) / FAT_SECTOR_SIZE;
        if (sectors_count + sector > fat16->cluster_sectors)
            sectors_count = fat16->cluster_sectors - sector;
        chunk = sectors_count * FAT_SECTOR_SIZE - sector_offset;
        if (chunk > size)
            chunk = size;

        if (!fat16_read_sectors(fat16, fat16_cluster_to_sector(fat16, h->current_cluster) + sector, sectors_count * FAT_SECTOR_SIZE))
            return;
        io_data_append(io, (uint8_t*)io_data(fat16->io) + sector_offset, chunk);
        size -= chunk;
        h->pos += chunk;
        if ((h->pos < h->size) && ((h->pos % fat16->cluster_size) == 0))
        {
            next_cluster = fat16_get_fat_next(fat16, h->current_cluster);
            if (next_cluster >= FAT_CLUSTER_RESERVED)
            {
                h->pos = h->size;
                return;
            }
            else
                h->current_cluster = next_cluster;
        }
    }
    io_complete(process, HAL_IO_CMD(HAL_VFS, IPC_READ), fh, io);
    error(ERROR_SYNC);
}

static inline void fat16_write_file(FAT16_TYPE* fat16, HANDLE fh, IO* io, HANDLE process)
{
    FAT16_FILE_HANDLE_TYPE* h;
    uint8_t* data;
    unsigned int cluster_offset, sector_offset, chunk, sector, sectors_count, next_cluster, size;
    h = so_get(&fat16->file_handles, fh);
    if (h == NULL)
        return;
    data = io_data(io);
    for (size = io->data_size; size; size -= chunk)
    {
        cluster_offset = h->pos % fat16->cluster_size;
        sector_offset = cluster_offset % FAT_SECTOR_SIZE;
        sector = cluster_offset / FAT_SECTOR_SIZE;
        sectors_count = (size + sector_offset + FAT_SECTOR_SIZE - 1) / FAT_SECTOR_SIZE;
        if (sectors_count + sector > fat16->cluster_sectors)
            sectors_count = fat16->cluster_sectors - sector;
        chunk = sectors_count * FAT_SECTOR_SIZE - sector_offset;
        if (chunk > size)
            chunk = size;

        //append cluster. Empty file already have one
        if ((h->pos == h->size) && (cluster_offset == 0) && h->size)
        {
            next_cluster = fat16_find_free_cluster(fat16, h->current_cluster);
            if (next_cluster >= FAT_CLUSTER_RESERVED)
            {
#if (VFS_DEBUG_ERRORS)
                printf("FAT16 warning: No free space\n");
#endif //VFS_DEBUG_ERRORS
                break;
            }
            if (!fat16_set_fat_value(fat16, h->current_cluster, next_cluster))
                break;
            h->current_cluster = next_cluster;
        }

        //readout first, no align
        if (sector_offset || (chunk % FAT_SECTOR_SIZE))
        {
            if (!fat16_read_sectors(fat16, fat16_cluster_to_sector(fat16, h->current_cluster) + sector, sectors_count * FAT_SECTOR_SIZE))
                break;
        }

        //overwrite data
        memcpy((uint8_t*)io_data(fat16->io) + sector_offset, data, chunk);
        data += chunk;

        //writeback
        if (!fat16_write_sectors(fat16, fat16_cluster_to_sector(fat16, h->current_cluster) + sector, sectors_count * FAT_SECTOR_SIZE))
            break;

        h->pos += chunk;
        //append to end of file
        if (h->pos > h->size)
            h->size = h->pos;

        if ((h->pos < h->size) && ((h->pos % fat16->cluster_size) == 0))
        {
            next_cluster = fat16_get_fat_next(fat16, h->current_cluster);
            if (next_cluster >= FAT_CLUSTER_RESERVED)
            {
                h->pos = h->size;
                break;
            }
            else
                h->current_cluster = next_cluster;
        }
    }
    //TODO: update file date/time access, size
    io_complete_ex(process, HAL_IO_CMD(HAL_VFS, IPC_READ), fh, io, io->data_size - size);
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
        fat16_find_first(fat16, ipc->param1, ipc->process);
        break;
    case VFS_FIND_NEXT:
        fat16_find_next(fat16, ipc);
        break;
    case VFS_FIND_CLOSE:
        fat16_find_close(fat16, (HANDLE)ipc->param1);
        break;
    case VFS_CD_UP:
        fat16_cd_up(fat16, ipc->param1, ipc->process);
        break;
    case VFS_CD_PATH:
        fat16_cd_path(fat16, ipc->param1, (IO*)ipc->param2, ipc->process);
        break;
    case VFS_READ_VOLUME_LABEL:
        fat16_read_volume_label(fat16, ipc);
        break;
    case IPC_OPEN:
        fat16_open_file(fat16, ipc->param1, (IO*)ipc->param2, ipc->process);
        break;
    case IPC_SEEK:
        fat16_seek_file(fat16, ipc->param1, ipc->param2);
        break;
    case IPC_READ:
        fat16_read_file(fat16, ipc->param1, (IO*)ipc->param2, ipc->param3, ipc->process);
        break;
    case IPC_WRITE:
        fat16_write_file(fat16, ipc->param1, (IO*)ipc->param2, ipc->process);
        break;
    case IPC_CLOSE:
        fat16_close_file(fat16, ipc->param1);
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
