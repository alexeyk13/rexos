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
    unsigned int first_cluster, current_cluster, cluster_num, pos;
} FAT16_FILE_INFO;

typedef struct {
    FAT16_FILE_INFO fi, data;
    unsigned int size, mode;
} FAT16_FILE_HANDLE_TYPE;

typedef struct {
    IO* io;
    VFS_VOLUME_TYPE volume;
    unsigned long sectors_count, cluster_sectors, root_count, root_sectors, reserved_sectors, fat_sectors, cluster_size, clusters_count;
    unsigned long current_sector, current_size;
    SO finds;
    SO file_handles;
} FAT16_TYPE;

typedef enum {
    FAT16_UPPER_CASE,
    FAT16_LOWER_CASE,
    FAT16_MIXED_CASE,
    FAT16_UNDEFINED_CASE
} FAT16_CASE_TYPE;

static const uint8_t __FAT16_BOOT[FAT_SECTOR_SIZE] = {
                                                            0xEB, 0x3C, 0x90, 0x4D, 0x53, 0x44, 0x4F, 0x53, 0x35, 0x2E, 0x30, 0x00, 0x02, 0x00, 0x01, 0x00,
                                                            0x02, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0x3F, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x5F, 0x58, 0xC1, 0x90, 0x4E, 0x4F, 0x20, 0x4E, 0x41,
                                                            0x4D, 0x45, 0x20, 0x20, 0x20, 0x20, 0x46, 0x41, 0x54, 0x31, 0x36, 0x20, 0x20, 0x20, 0x33, 0xC9,
                                                            0x8E, 0xD1, 0xBC, 0xF0, 0x7B, 0x8E, 0xD9, 0xB8, 0x00, 0x20, 0x8E, 0xC0, 0xFC, 0xBD, 0x00, 0x7C,
                                                            0x38, 0x4E, 0x24, 0x7D, 0x24, 0x8B, 0xC1, 0x99, 0xE8, 0x3C, 0x01, 0x72, 0x1C, 0x83, 0xEB, 0x3A,
                                                            0x66, 0xA1, 0x1C, 0x7C, 0x26, 0x66, 0x3B, 0x07, 0x26, 0x8A, 0x57, 0xFC, 0x75, 0x06, 0x80, 0xCA,
                                                            0x02, 0x88, 0x56, 0x02, 0x80, 0xC3, 0x10, 0x73, 0xEB, 0x33, 0xC9, 0x8A, 0x46, 0x10, 0x98, 0xF7,
                                                            0x66, 0x16, 0x03, 0x46, 0x1C, 0x13, 0x56, 0x1E, 0x03, 0x46, 0x0E, 0x13, 0xD1, 0x8B, 0x76, 0x11,
                                                            0x60, 0x89, 0x46, 0xFC, 0x89, 0x56, 0xFE, 0xB8, 0x20, 0x00, 0xF7, 0xE6, 0x8B, 0x5E, 0x0B, 0x03,
                                                            0xC3, 0x48, 0xF7, 0xF3, 0x01, 0x46, 0xFC, 0x11, 0x4E, 0xFE, 0x61, 0xBF, 0x00, 0x00, 0xE8, 0xE6,
                                                            0x00, 0x72, 0x39, 0x26, 0x38, 0x2D, 0x74, 0x17, 0x60, 0xB1, 0x0B, 0xBE, 0xA1, 0x7D, 0xF3, 0xA6,
                                                            0x61, 0x74, 0x32, 0x4E, 0x74, 0x09, 0x83, 0xC7, 0x20, 0x3B, 0xFB, 0x72, 0xE6, 0xEB, 0xDC, 0xA0,
                                                            0xFB, 0x7D, 0xB4, 0x7D, 0x8B, 0xF0, 0xAC, 0x98, 0x40, 0x74, 0x0C, 0x48, 0x74, 0x13, 0xB4, 0x0E,
                                                            0xBB, 0x07, 0x00, 0xCD, 0x10, 0xEB, 0xEF, 0xA0, 0xFD, 0x7D, 0xEB, 0xE6, 0xA0, 0xFC, 0x7D, 0xEB,
                                                            0xE1, 0xCD, 0x16, 0xCD, 0x19, 0x26, 0x8B, 0x55, 0x1A, 0x52, 0xB0, 0x01, 0xBB, 0x00, 0x00, 0xE8,
                                                            0x3B, 0x00, 0x72, 0xE8, 0x5B, 0x8A, 0x56, 0x24, 0xBE, 0x0B, 0x7C, 0x8B, 0xFC, 0xC7, 0x46, 0xF0,
                                                            0x3D, 0x7D, 0xC7, 0x46, 0xF4, 0x29, 0x7D, 0x8C, 0xD9, 0x89, 0x4E, 0xF2, 0x89, 0x4E, 0xF6, 0xC6,
                                                            0x06, 0x96, 0x7D, 0xCB, 0xEA, 0x03, 0x00, 0x00, 0x20, 0x0F, 0xB6, 0xC8, 0x66, 0x8B, 0x46, 0xF8,
                                                            0x66, 0x03, 0x46, 0x1C, 0x66, 0x8B, 0xD0, 0x66, 0xC1, 0xEA, 0x10, 0xEB, 0x5E, 0x0F, 0xB6, 0xC8,
                                                            0x4A, 0x4A, 0x8A, 0x46, 0x0D, 0x32, 0xE4, 0xF7, 0xE2, 0x03, 0x46, 0xFC, 0x13, 0x56, 0xFE, 0xEB,
                                                            0x4A, 0x52, 0x50, 0x06, 0x53, 0x6A, 0x01, 0x6A, 0x10, 0x91, 0x8B, 0x46, 0x18, 0x96, 0x92, 0x33,
                                                            0xD2, 0xF7, 0xF6, 0x91, 0xF7, 0xF6, 0x42, 0x87, 0xCA, 0xF7, 0x76, 0x1A, 0x8A, 0xF2, 0x8A, 0xE8,
                                                            0xC0, 0xCC, 0x02, 0x0A, 0xCC, 0xB8, 0x01, 0x02, 0x80, 0x7E, 0x02, 0x0E, 0x75, 0x04, 0xB4, 0x42,
                                                            0x8B, 0xF4, 0x8A, 0x56, 0x24, 0xCD, 0x13, 0x61, 0x61, 0x72, 0x0B, 0x40, 0x75, 0x01, 0x42, 0x03,
                                                            0x5E, 0x0B, 0x49, 0x75, 0x06, 0xF8, 0xC3, 0x41, 0xBB, 0x00, 0x00, 0x60, 0x66, 0x6A, 0x00, 0xEB,
                                                            0xB0, 0x4E, 0x54, 0x4C, 0x44, 0x52, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x0D, 0x0A, 0x52, 0x65,
                                                            0x6D, 0x6F, 0x76, 0x65, 0x20, 0x64, 0x69, 0x73, 0x6B, 0x73, 0x20, 0x6F, 0x72, 0x20, 0x6F, 0x74,
                                                            0x68, 0x65, 0x72, 0x20, 0x6D, 0x65, 0x64, 0x69, 0x61, 0x2E, 0xFF, 0x0D, 0x0A, 0x44, 0x69, 0x73,
                                                            0x6B, 0x20, 0x65, 0x72, 0x72, 0x6F, 0x72, 0xFF, 0x0D, 0x0A, 0x50, 0x72, 0x65, 0x73, 0x73, 0x20,
                                                            0x61, 0x6E, 0x79, 0x20, 0x6B, 0x65, 0x79, 0x20, 0x74, 0x6F, 0x20, 0x72, 0x65, 0x73, 0x74, 0x61,
                                                            0x72, 0x74, 0x0D, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAC, 0xCB, 0xD8, 0x55, 0xAA};

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
    fat16->current_sector = sector;
    return storage_write_sync(fat16->volume.hal, fat16->volume.process, fat16->volume.user, fat16->io, sector + fat16->volume.first_sector);
}

static bool fat16_zero_sectors(FAT16_TYPE* fat16, unsigned long sector, unsigned count)
{
    unsigned long size, chunk;
    memset(io_data(fat16->io), 0, VFS_IO_SIZE);
    for (size = count * FAT_SECTOR_SIZE; size; size -= fat16->io->data_size)
    {
        chunk = VFS_IO_SIZE;
        if (chunk > size)
            chunk = size;
        if (!fat16_write_sectors(fat16, sector, chunk))
            return false;
        sector += fat16->io->data_size / FAT_SECTOR_SIZE;
    }
    return true;
}

static void fat16_now(struct tm* ts)
{
    TIME t;
    SYSTIME uptime;
    get_uptime(&uptime);
    t.day = VFS_BASE_DATE + uptime.sec / SEC_IN_DAY;
    t.ms = (uptime.sec % SEC_IN_DAY) * 1000 + uptime.usec / 1000;
    gmtime(&t, ts);
}

static unsigned short fat16_fat_date_now()
{
    struct tm ts;
    fat16_now(&ts);
    return ((ts.tm_year - FAT_DATE_EPOCH_YEAR) << FAT_DATE_YEAR_POS) | ((ts.tm_mon + 1) << FAT_DATE_MON_POS) | (ts.tm_mday << FAT_DATE_DAY_POS);
}

static unsigned short fat16_fat_time_now()
{
    struct tm ts;
    fat16_now(&ts);
    return (ts.tm_hour << FAT_TIME_HOURS_POS) | (ts.tm_min << FAT_TIME_MINUTES_POS) | ((ts.tm_sec >> 1) << FAT_DATE_DAY_POS);
}

static unsigned short fat16_ztime_now()
{
    SYSTIME uptime;
    get_uptime(&uptime);
    return uptime.usec / 10000;
}

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
        if (fat16_get_fat_value(fat16, next_free) == FAT_CLUSTER_FREE)
            return next_free;
    //from first till current -1
    for (next_free = 2; next_free < cluster;  ++next_free)
        if (fat16_get_fat_value(fat16, next_free) == FAT_CLUSTER_FREE)
            return next_free;
#if (VFS_DEBUG_ERRORS)
    printf("FAT16 warning: No free space\n");
#endif //VFS_DEBUG_ERRORS
    return FAT_CLUSTER_RESERVED;
}

static unsigned long fat16_occupy_first_cluster(FAT16_TYPE* fat16)
{
    unsigned long cluster = fat16_find_free_cluster(fat16, 2);
    if (cluster >= FAT_CLUSTER_RESERVED)
        return FAT_CLUSTER_RESERVED;
    if (!fat16_set_fat_value(fat16, cluster, FAT_CLUSTER_LAST))
        return FAT_CLUSTER_RESERVED;
    return cluster;
}

static unsigned long fat16_occupy_next_cluster(FAT16_TYPE* fat16, unsigned long current_cluster)
{
    unsigned long cluster = fat16_find_free_cluster(fat16, current_cluster);
    if (cluster >= FAT_CLUSTER_RESERVED)
        return FAT_CLUSTER_RESERVED;
    if (!fat16_set_fat_value(fat16, current_cluster, cluster))
        return FAT_CLUSTER_RESERVED;
    if (!fat16_set_fat_value(fat16, cluster, FAT_CLUSTER_LAST))
        return FAT_CLUSTER_RESERVED;
    return cluster;
}

static bool fat16_release_chain(FAT16_TYPE* fat16, unsigned long start_cluster)
{
    unsigned long cluster, next_cluster;
    for (cluster = start_cluster; cluster < FAT_CLUSTER_RESERVED; cluster = next_cluster)
    {
        next_cluster = fat16_get_fat_value(fat16, cluster);
        if (!fat16_set_fat_value(fat16, cluster, FAT_CLUSTER_FREE))
            return false;
    }
    return true;
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
    so_create(&fat16->finds, sizeof(FAT16_FILE_INFO), 1);
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
#if (VFS_DEBUG_ERRORS)
        printf("FAT16: Invalid boot sector signature\n");
#endif //VFS_DEBUG_ERRORS
        error(ERROR_NOT_SUPPORTED);
        return false;
    }
    bpb = (void*)(boot + sizeof(FAT_BOOT_SECTOR_HEADER_TYPE));
    if ((bpb->sector_size != FAT_SECTOR_SIZE) || (bpb->fat_count != 2) || (bpb->cluster_sectors * bpb->sector_size > VFS_IO_SIZE) ||
        (bpb->ext_signature != FAT_BPB_EXT_SIGNATURE) || bpb->reserved_sectors == 0 || bpb->fat_sectors == 0)
    {
        error(ERROR_NOT_SUPPORTED);
#if (VFS_DEBUG_ERRORS)
        printf("FAT16: Unsupported BPB\n");
#endif //VFS_DEBUG_ERRORS
        return false;
    }
    fat16->cluster_sectors = bpb->cluster_sectors;
    fat16->sectors_count = bpb->sectors_short;
    fat16->root_count = bpb->root_count;
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

static void fat16_fi_create(FAT16_FILE_INFO* fi, unsigned int first_cluster)
{
    fi->first_cluster = fi->current_cluster = first_cluster;
    fi->cluster_num = fi->pos = 0;
}

static void fat16_fi_reset(FAT16_FILE_INFO* fi)
{
    fi->current_cluster = fi->first_cluster;
    fi->cluster_num = 0;
}

static bool fat16_fi_get_cluster_num(FAT16_TYPE* fat16, FAT16_FILE_INFO* fi, unsigned int cluster_num)
{
    for(fat16_fi_reset(fi); fi->current_cluster < FAT_CLUSTER_RESERVED && fi->cluster_num < cluster_num; ++fi->cluster_num)
        fi->current_cluster = fat16_get_fat_next(fat16, fi->current_cluster);
    return fi->current_cluster < FAT_CLUSTER_RESERVED;
}

static FAT_FILE_ENTRY* fat16_read_file_entry(FAT16_TYPE* fat16, FAT16_FILE_INFO* fi)
{
    unsigned int sector_num, cluster_num, sector, pos_cur;
    FAT_FILE_ENTRY* entries;
    if (fi->first_cluster == VFS_ROOT)
    {
        if (fi->pos >= fat16->root_count)
            return NULL;
        sector_num = fi->pos / FILE_ENTRIES_IN_SECTOR;
        pos_cur = fi->pos - sector_num * FILE_ENTRIES_IN_SECTOR;
        sector = fat16->reserved_sectors + fat16->fat_sectors * 2 + sector_num;
    }
    else
    {
        cluster_num = fi->pos / (FILE_ENTRIES_IN_SECTOR * fat16->cluster_sectors);
        if (cluster_num != fi->cluster_num)
        {
            //most often case - iterate search
            if (cluster_num == fi->cluster_num + 1)
            {
                fi->current_cluster = fat16_get_fat_next(fat16, fi->current_cluster);
                fi->cluster_num = cluster_num;
            }
            else
                fat16_fi_get_cluster_num(fat16, fi, cluster_num);
            if (fi->current_cluster >= FAT_CLUSTER_RESERVED)
                return NULL;
        }
        sector_num = (fi->pos - FILE_ENTRIES_IN_SECTOR * fat16->cluster_sectors * fi->cluster_num) / FILE_ENTRIES_IN_SECTOR;
        sector = fat16_cluster_to_sector(fat16, fi->current_cluster) + sector_num;
        pos_cur = fi->pos - (fi->cluster_num * fat16->cluster_sectors + sector_num) * FILE_ENTRIES_IN_SECTOR;
    }
    if (!fat16_read_sectors(fat16, sector, FAT_SECTOR_SIZE))
        return NULL;
    entries = io_data(fat16->io);
    return entries + pos_cur;
}

static FAT_FILE_ENTRY* fat16_init_file_entry(FAT16_TYPE* fat16, FAT16_FILE_INFO* fi)
{
    FAT_FILE_ENTRY* entry;
    entry = fat16_read_file_entry(fat16, fi);
    if (entry == NULL)
        return NULL;
    memset(entry->name, ' ', 11);
    entry->attr = 0;
    entry->sys_attr = 0;
    entry->security = 0;
    entry->first_cluster = 0;
    entry->size = 0;
    entry->crt_ztime = fat16_ztime_now();
    entry->crt_date = entry->mod_date = entry->acc_date = fat16_fat_date_now();
    entry->crt_time = entry->mod_time = fat16_fat_time_now();
    return entry;
}

//must be readed first
static bool fat16_write_file_entry(FAT16_TYPE* fat16)
{
    return fat16_write_sectors(fat16, fat16->current_sector, FAT_SECTOR_SIZE);
}

static bool fat16_allocate_file_entries(FAT16_TYPE* fat16, FAT16_FILE_INFO* fi, unsigned int count)
{
    unsigned int i;
    unsigned long next_cluster;
    FAT_FILE_ENTRY* entry;
    fat16_fi_reset(fi);
    for(fi->pos = 0, i = 0; i < count; ++fi->pos)
    {
        if ((entry = fat16_read_file_entry(fat16, fi)) == NULL)
            return false;
        //last one
        if (entry->name[0] == FAT_FILE_ENTRY_EMPTY)
            break;
        if (entry->name[0] == FAT_FILE_ENTRY_MAGIC_ERASED)
            ++i;
        else
            i = 0;
    }
    fi->pos -= i;
    if (i == count)
        return true;
    //no hole? append.
    if (fi->first_cluster == VFS_ROOT)
    {
        if (fi->pos + count >= fat16->root_count)
        {
#if (VFS_DEBUG_ERRORS)
            printf("FAT16: Warning: Too many root entries\n");
#endif //VFS_DEBUG_ERRORS
            error(ERROR_NOT_FOUND);
            return false;
        }
    }
    else
    {
        i = (fat16->cluster_size - ((fi->pos * sizeof(FAT_FILE_ENTRY)) % fat16->cluster_size)) / sizeof(FAT_FILE_ENTRY);
        //need more cluster?
        if (i < count)
        {
            if ((next_cluster = fat16_occupy_next_cluster(fat16, fi->current_cluster)) >= FAT_CLUSTER_RESERVED)
                return false;
            if (!fat16_zero_sectors(fat16, fat16_cluster_to_sector(fat16, next_cluster), fat16->cluster_sectors))
                return false;
        }
    }
    return true;
}

static uint8_t fat16_lfn_checksum(const char* name83)
{
   unsigned int i;
   uint8_t crc = 0;

   for (i = 11; i; i--)
      crc = ((crc & 1) << 7) + (crc >> 1) + (uint8_t)(*name83++);
   return crc;
}

static unsigned int fat16_lfn_read_chunk(FAT_LFN_ENTRY* lfn, char* lfn_chunk_latin1)
{
    uint16_t lfn_chunk[FAT_LFN_CHUNK_SIZE];
    memcpy(lfn_chunk + 0, lfn->name1, 5 * 2);
    memcpy(lfn_chunk + 5, lfn->name2, 6 * 2);
    memcpy(lfn_chunk + 11, lfn->name3, 2 * 2);
    return utf16_to_latin1(lfn_chunk, lfn_chunk_latin1, FAT_LFN_CHUNK_SIZE);
}

static bool fat16_lfn_write_chunk(FAT16_TYPE* fat16, FAT16_FILE_INFO* fi, uint8_t checksum, uint8_t seq, const char* name)
{
    uint16_t lfn_chunk[FAT_LFN_CHUNK_SIZE];
    FAT_LFN_ENTRY* lfn;
    unsigned int lfn_chunk_size;
    lfn = (FAT_LFN_ENTRY*)fat16_read_file_entry(fat16, fi);
    if (lfn == NULL)
        return false;
    memset(lfn, 0xFF, sizeof(FAT_LFN_ENTRY));
    lfn->seq = seq;
    lfn->attr = FAT_FILE_ATTR_LFN;
    lfn->type = 0x00;
    lfn->checksum = checksum;
    lfn->first_cluster = 0x0000;

    lfn_chunk_size = latin1_to_utf16(name, lfn_chunk, FAT_LFN_CHUNK_SIZE);
    //add tailing zero
    if (lfn_chunk_size < FAT_LFN_CHUNK_SIZE)
        ++lfn_chunk_size;
    memcpy(lfn->name1, lfn_chunk + 0, ((lfn_chunk_size >= 5 ? 5 : lfn_chunk_size) - 0) * 2);
    if (lfn_chunk_size > 5)
        memcpy(lfn->name2, lfn_chunk + 5, ((lfn_chunk_size >= 11 ? 11 : lfn_chunk_size) - 5) * 2);
    if (lfn_chunk_size > 11)
        memcpy(lfn->name3, lfn_chunk + 11, (lfn_chunk_size - 11) * 2);
    return fat16_write_file_entry(fat16);
}

static bool fat16_get_file_name(FAT16_TYPE* fat16, char* name, FAT16_FILE_INFO* fi, unsigned int mask, unsigned int ignore_mask)
{
    FAT_FILE_ENTRY* entry;
    FAT_LFN_ENTRY* lfn;
    char lfn_chunk_latin1[FAT_LFN_CHUNK_SIZE];
    unsigned int size, lfn_chunk_size;

    uint8_t lfn_seq = 0x00;
    uint8_t lfn_crc = 0x00;
    size = 0;
    for (;;)
    {
        entry = fat16_read_file_entry(fat16, fi);
        if (entry == NULL)
            return false;
        if ((uint8_t)(entry->name[0]) == FAT_FILE_ENTRY_EMPTY)
            return false;
        if ((uint8_t)(entry->name[0]) == FAT_FILE_ENTRY_MAGIC_ERASED)
        {
            ++fi->pos;
            continue;
        }
        //LFN chunk process
        if ((entry->attr & FAT_FILE_ATTR_LFN) == FAT_FILE_ATTR_LFN)
        {
            ++fi->pos;
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
            lfn_chunk_size = fat16_lfn_read_chunk(lfn, lfn_chunk_latin1);
            memmove(name + lfn_chunk_size, name, size);
            memcpy(name, lfn_chunk_latin1, lfn_chunk_size);
            size += lfn_chunk_size;
            name[size] = '\x0';
            if (--lfn_seq == 0)
                lfn_seq = 0xff;
            continue;
        }
        if (((entry->attr & (mask & FAT_FILE_ATTR_REAL_MASK)) != (mask & FAT_FILE_ATTR_REAL_MASK)) ||
            ((mask & FAT_FILE_ATTR_DOT_OR_DOT_DOT) && entry->name[0] != '.') ||
            ((ignore_mask & FAT_FILE_ATTR_DOT_OR_DOT_DOT) && entry->name[0] == '.') ||
            (entry->attr & (ignore_mask & FAT_FILE_ATTR_REAL_MASK)))
        {
            ++fi->pos;
            lfn_seq = 0x00;
            size = 0;
            continue;
        }
        if (lfn_seq == 0xff)
        {
            if (lfn_crc == fat16_lfn_checksum(entry->name))
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

static bool fat16_find_by_name(FAT16_TYPE* fat16, FAT16_FILE_INFO* fi, const char* name, unsigned int mask, unsigned int ignore_mask)
{
    char name_cur[VFS_MAX_FILE_PATH + 1];
    fat16_fi_reset(fi);
    for (fi->pos = 0; fat16_get_file_name(fat16, name_cur, fi, mask, ignore_mask); ++fi->pos)
    {
        if (strcmp(name_cur, name) == 0)
            return true;
    }
    return false;
}

static bool fat16_find_by_name83(FAT16_TYPE* fat16, FAT16_FILE_INFO* fi, const char* name83)
{
    FAT_FILE_ENTRY* entry;
    fat16_fi_reset(fi);
    for (fi->pos = 0; (entry = fat16_read_file_entry(fat16, fi)) != NULL; ++fi->pos)
    {
         if ((uint8_t)(entry->name[0]) == FAT_FILE_ENTRY_EMPTY)
             return false;
         //skip erased and LFN chunks
         if ((uint8_t)(entry->name[0]) == FAT_FILE_ENTRY_MAGIC_ERASED)
             continue;
         if ((entry->attr & FAT_FILE_ATTR_LFN) == FAT_FILE_ATTR_LFN)
             continue;
         if (memcmp(entry->name, name83, 11) == 0)
             return true;
    }
    return false;
}

static bool fat16_get_by_name(FAT16_TYPE* fat16, FAT16_FILE_INFO* fi, const char* name, unsigned int mask, unsigned int ignore_mask)
{
    FAT_FILE_ENTRY* entry;
    if (!fat16_find_by_name(fat16, fi, name, mask, ignore_mask))
    {
        error(ERROR_NOT_FOUND);
        return false;
    }
    entry = fat16_read_file_entry(fat16, fi);
    fat16_fi_create(fi, entry->first_cluster);
    return true;
}

static bool fat16_get_parent_folder(FAT16_TYPE* fat16, FAT16_FILE_INFO* fi)
{
    if (fi->first_cluster == VFS_ROOT)
        return true;
    return fat16_get_by_name(fat16, fi, VFS_PARENT_FOLDER,  FAT_FILE_ATTR_DOT_OR_DOT_DOT, FAT_FILE_ATTR_LABEL);
}

static bool fat16_get_path(FAT16_TYPE* fat16, FAT16_FILE_INFO* fi, char* path)
{
    unsigned int i;
    char* path_cur;
    //NULL path is current path
    if (path == NULL)
        return true;
    //root only path
    if (path[0] == '\x0')
    {
        fat16_fi_create(fi, VFS_ROOT);
        return true;
    }
    //starting from root
    if (path[0] == VFS_FOLDER_DELIMITER)
    {
        fat16_fi_create(fi, VFS_ROOT);
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
            if (!fat16_get_parent_folder(fat16, fi))
                return false;
            continue;
        }
        if (!fat16_get_by_name(fat16, fi, path_cur, FAT_FILE_ATTR_SUBFOLDER, FAT_FILE_ATTR_LABEL))
            return false;
    }
    return true;
}

static char* fat16_extract_file_from_path(char* raw, char** path)
{
    char* file;
    int i;
    for (i = strlen(raw) - 1; i >= 0 && raw[i] != VFS_FOLDER_DELIMITER; --i) {}
    file = raw + i + 1;
    *path = raw;
    if (strcmp(file, VFS_CURRENT_FOLDER) == 0)
        return NULL;
    if (strcmp(file, VFS_PARENT_FOLDER) == 0)
        return NULL;
    if (i >= 0)
        raw[i] = '\x0';
    else
        *path = NULL;
    return file;
}

static char* fat16_extract_ext_from_name(char* name_ext, char** name)
{
    unsigned int len;
    int i;
    len = strlen(name_ext);
    *name = name_ext;
    if (len == 0)
        return NULL;
    //file. has no extension
    if (name_ext[len - 1] == '.')
        return NULL;
    //file of .abc is name without extension
    for (i = len - 1; i > 0; --i)
    {
        //dot found
        if (name_ext[i] == '.')
        {
            name_ext[i] = '\x0';
            return name_ext + i + 1;
        }

        //no extension
        if (len - 1 - i >= 3)
            break;
    }
    return NULL;
}

static bool fat16_is_folder_empty(FAT16_TYPE* fat16, FAT16_FILE_INFO* fi)
{
    FAT16_FILE_INFO folder_fi;
    FAT_FILE_ENTRY* entry;
    entry = fat16_read_file_entry(fat16, fi);
    if (entry == NULL)
        return false;
    fat16_fi_create(&folder_fi, entry->first_cluster);
    //can't erase root folder
    if (folder_fi.first_cluster == VFS_ROOT)
        return false;
    //skip . and ..
    for (folder_fi.pos = 2;; ++folder_fi.pos)
    {
        entry = fat16_read_file_entry(fat16, &folder_fi);
        if (entry == NULL)
            return false;
        //skip LFN chunks, volume labels,
        if ((entry->name[0] == FAT_FILE_ENTRY_MAGIC_ERASED) || (entry->attr == FAT_FILE_ATTR_LFN) || (entry->attr == FAT_FILE_ATTR_LABEL))
            continue;
        if (entry->name[0] == FAT_FILE_ENTRY_EMPTY)
            return true;
        //in all other cases it's file or folder
        return false;
    }
}

static char fat16_char_upper(char c)
{
    if (c >= 'a' && c <= 'z')
        return c - 0x20;
    return c;
}

static bool fat16_83_existed_char(char c)
{
    if (c == '.' || c == ' ')
        return false;
    return true;
}

static FAT16_CASE_TYPE fat16_get_case(char* str)
{
    FAT16_CASE_TYPE c = FAT16_UNDEFINED_CASE;
    for (; str[0]; str++)
    {
        if (!fat16_83_existed_char(str[0]))
            return FAT16_MIXED_CASE;
        if (str[0] >= 'A' && str[0] <= 'Z')
        {
            if (c == FAT16_LOWER_CASE)
                return FAT16_MIXED_CASE;
            c = FAT16_UPPER_CASE;
        }
        if (str[0] >= 'a' && str[0] <= 'z')
        {
            if (c == FAT16_UPPER_CASE)
                return FAT16_MIXED_CASE;
            c = FAT16_LOWER_CASE;
        }
    }
    if (c == FAT16_UNDEFINED_CASE)
        c = FAT16_UPPER_CASE;
    return c;
}

static bool fat16_create_file_entry(FAT16_TYPE* fat16, FAT16_FILE_INFO* fi, char* name_ext, uint8_t attr)
{
    char* ext;
    char* name;
    char name83[11];
    unsigned int len, i, j, first_cluster;
    uint8_t name83_checksum;
    FAT16_CASE_TYPE name_case, ext_case;
    FAT_FILE_ENTRY* entry;
    //make sure file(folder) not exists
    if (fat16_find_by_name(fat16, fi, name_ext, 0, FAT_FILE_ATTR_LABEL))
    {
        error(ERROR_FILE_PATH_ALREADY_EXISTS);
        return false;
    }
    len = strlen(name_ext);
    ext = fat16_extract_ext_from_name(name_ext, &name);
    if (name == NULL)
    {
        error(ERROR_INVALID_PARAMS);
        return false;
    }
    for (i = 0; i < 11; ++i)
        name83[i] = ' ';
    if (strlen(name) > 8)
        name_case = FAT16_MIXED_CASE;
    else
        name_case = fat16_get_case(name);
    if (ext)
    {
        ext_case = fat16_get_case(ext);
        for (i = 0, j = 8; i < 3 && ext[i]; ++i)
            if (fat16_83_existed_char(ext[i]))
                name83[j++] = fat16_char_upper(ext[i]);
    }
    else
        ext_case = FAT16_UPPER_CASE;
    first_cluster = fat16_occupy_first_cluster(fat16);
    if (first_cluster >= FAT_CLUSTER_RESERVED)
        return false;
    do {
        //need lfn?
        if (name_case == FAT16_MIXED_CASE || ext_case == FAT16_MIXED_CASE)
        {
            name_case = ext_case = FAT16_MIXED_CASE;
            for (i = 0, j = 0; name[i] && j < 6; ++i)
                if (fat16_83_existed_char(name[i]))
                    name83[j++] = fat16_char_upper(name[i]);
            name83[j++] = '~';
            //make sure short name is not used
            for (i = 1; i <= 9; ++i)
            {
                name83[j] = '0' + i;
                if (!fat16_find_by_name83(fat16, fi, name83))
                    break;
            }
            if (i > 9)
            {
                error(ERROR_FILE_PATH_ALREADY_EXISTS);
                break;
            }
            //join name and ext
            if (ext)
                name[strlen(name)] = '.';
            j = (len + FAT_LFN_CHUNK_SIZE - 1) / FAT_LFN_CHUNK_SIZE;
            if (!fat16_allocate_file_entries(fat16, fi, 1 + j))
                return false;
            //write LFN chunks
            name83_checksum = fat16_lfn_checksum(name83);
            for (i = j; i > 0; --i)
            {
                if (!fat16_lfn_write_chunk(fat16, fi, name83_checksum, i | (i == j ? FAT_LFN_SEQ_LAST : 0), name + FAT_LFN_CHUNK_SIZE * (i - 1)))
                    break;
                ++fi->pos;
            }
            if (i)
                break;
        }
        else
        {
            for (i = 0; i < 8 && name[i]; ++i)
                name83[i] = fat16_char_upper(name[i]);
            if (!fat16_allocate_file_entries(fat16, fi, 1))
                break;
        }
        //write short entry
        entry = fat16_init_file_entry(fat16, fi);
        if (entry == NULL)
            break;
        memcpy(entry->name, name83, 11);
        entry->attr = attr;
        if (name_case == FAT16_LOWER_CASE)
            entry->sys_attr |= FAT_FILE_SYS_ATTR_NAME_LOWER_CASE;
        if (ext_case == FAT16_LOWER_CASE)
            entry->sys_attr |= FAT_FILE_SYS_ATTR_EXT_LOWER_CASE;
        entry->first_cluster = first_cluster;
        if (!fat16_write_file_entry(fat16))
            break;
        return true;
    } while (false);

    //error occured, free occupied cluster
    fat16_release_chain(fat16, first_cluster);
    return false;
}

static bool fat16_remove_file_entry(FAT16_TYPE* fat16, FAT16_FILE_INFO* fi)
{
    FAT_FILE_ENTRY* entry;
    FAT_LFN_ENTRY* lfn;
    uint8_t name83_checksum, seq;
    //first remove short entry
    entry = fat16_read_file_entry(fat16, fi);
    if (entry == NULL)
        return false;
    name83_checksum = fat16_lfn_checksum(entry->name);
    entry->name[0] = FAT_FILE_ENTRY_MAGIC_ERASED;
    if (!fat16_write_file_entry(fat16))
        return false;
    //remove lfn chunks
    if (fi->pos == 0)
        return true;
    for(--fi->pos;;--fi->pos)
    {
        lfn = (FAT_LFN_ENTRY*)fat16_read_file_entry(fat16, fi);
        if (lfn == NULL)
            return false;
        if ((lfn->attr != FAT_FILE_ATTR_LFN) || (lfn->checksum != name83_checksum))
            return true;
        seq = lfn->seq;
        lfn->attr = 0;
        lfn->seq = FAT_FILE_ENTRY_MAGIC_ERASED;
        if (!fat16_write_file_entry(fat16))
            return false;
        if ((fi->pos == 0) || (seq & FAT_LFN_SEQ_LAST))
            return true;
    }
}

static void fat16_close_file(FAT16_TYPE* fat16, HANDLE h)
{
    FAT16_FILE_HANDLE_TYPE* f;
    FAT_FILE_ENTRY* entry;
    f = so_get(&fat16->file_handles, h);
    if (f)
    {
        entry = fat16_read_file_entry(fat16, &f->fi);
        if (entry)
        {
            entry->acc_date = fat16_fat_date_now();
            if (f->mode & VFS_MODE_WRITE)
            {
                entry->mod_date = entry->acc_date;
                entry->mod_time = fat16_fat_time_now();
            }
            fat16_write_file_entry(fat16);
        }
    }
    so_free(&fat16->file_handles, h);
}

static void fat16_unmount_internal(FAT16_TYPE* fat16)
{
    fat16->volume.process = INVALID_HANDLE;
}

static void fat16_mount_internal(FAT16_TYPE* fat16, VFS_VOLUME_TYPE* volume)
{
    memcpy(&fat16->volume, volume, sizeof(VFS_VOLUME_TYPE));
    fat16->current_sector = 0xffffffff;
    fat16->current_size = 0;
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
    fat16_mount_internal(fat16, io_data(io));
    if (!fat16_parse_boot(fat16))
        fat16_unmount_internal(fat16);
}

static void fat16_unmount(FAT16_TYPE* fat16)
{
    HANDLE handle;
    if (fat16->volume.process == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    //1. free finds
    while((handle = so_first(&fat16->finds)) != INVALID_HANDLE)
        so_free(&fat16->finds, handle);
    //2. free file_handles
    while((handle = so_first(&fat16->file_handles)) != INVALID_HANDLE)
        fat16_close_file(fat16, handle);
    fat16_unmount_internal(fat16);
}

static inline void fat16_find_first(FAT16_TYPE* fat16, unsigned int folder, HANDLE process)
{
    HANDLE h;
    FAT16_FILE_INFO* fi;

    if (fat16->volume.process == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }

    h = so_allocate(&fat16->finds);
    if (h == INVALID_HANDLE)
        return;
    fi = so_get(&fat16->finds, h);
    fat16_fi_create(fi, folder);
    ipc_post_inline(process, HAL_CMD(HAL_VFS, VFS_FIND_FIRST), folder, h, 0);
    error(ERROR_SYNC);
}

static inline void fat16_find_next(FAT16_TYPE* fat16, IPC* ipc)
{
    VFS_FIND_TYPE* find;
    FAT16_FILE_INFO* fi;
    FAT_FILE_ENTRY* entry;
    IO* io = (IO*)ipc->param2;

    find = io_data(io);
    fi = so_get(&fat16->finds, ipc->param1);
    if (fi == NULL)
        return;
    if (!fat16_get_file_name(fat16, find->name, fi, 0, FAT_FILE_ATTR_LABEL | FAT_FILE_ATTR_DOT_OR_DOT_DOT))
    {
        error(ERROR_NOT_FOUND);
        return;
    }
    entry = fat16_read_file_entry(fat16, fi);
    find->item = entry->first_cluster;
    find->size = entry->size;
    find->attr = fat16_decode_attr(entry->attr);
    ++fi->pos;

    io->data_size = sizeof(VFS_FIND_TYPE) - VFS_MAX_FILE_PATH + strlen(find->name) + 1;
    io_complete_ex(ipc->process, HAL_IO_CMD(HAL_VFS, VFS_FIND_NEXT), ipc->param1, io, ipc->param1);
    error(ERROR_SYNC);
}

static void fat16_find_close(FAT16_TYPE* fat16, HANDLE h)
{
    so_free(&fat16->finds, h);
}

static inline void fat16_cd_up(FAT16_TYPE* fat16, unsigned int folder, HANDLE process)
{
    FAT16_FILE_INFO fi;
    if (fat16->volume.process == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }

    fat16_fi_create(&fi, folder);
    if (!fat16_get_parent_folder(fat16, &fi))
        return;
    ipc_post_inline(process, HAL_CMD(HAL_VFS, VFS_CD_UP), folder, fi.first_cluster, 0);
    error(ERROR_SYNC);
}

static inline void fat16_cd_path(FAT16_TYPE* fat16, unsigned int folder, IO* io, HANDLE process)
{
    FAT16_FILE_INFO fi;
    fat16_fi_create(&fi, folder);
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
    if (!fat16_get_path(fat16, &fi, path))
        return;
    *((unsigned int*)io_data(io)) = fi.first_cluster;
    io->data_size = sizeof(unsigned int);
    io_complete(process, HAL_IO_CMD(HAL_VFS, VFS_CD_PATH), folder, io);
    error(ERROR_SYNC);
}

static inline void fat16_read_volume_label(FAT16_TYPE* fat16, IPC* ipc)
{
    char* name;
    FAT16_FILE_INFO fi;
    IO* io = (IO*)ipc->param2;

    if (fat16->volume.process == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }

    name = io_data(io);
    fat16_fi_create(&fi, VFS_ROOT);
    if (!fat16_get_file_name(fat16, name, &fi, FAT_FILE_ATTR_LABEL, FAT_FILE_ATTR_DOT_OR_DOT_DOT))
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
    FAT16_FILE_INFO fi;
    HANDLE h;
    FAT16_FILE_HANDLE_TYPE* f;
    char* file;
    char* path;
    FAT_FILE_ENTRY* entry;

#if (VFS_MAX_HANDLES)
    if (so_count(&fat16->file_handles) >= VFS_MAX_HANDLES)
    {
        error(ERROR_TOO_MANY_HANDLES);
        return;
    }
#endif //VFS_MAX_HANDLES
    if (fat16->volume.process == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }

    fat16_fi_create(&fi, folder);
    ot = io_data(io);
    if (strlen(ot->name) > VFS_MAX_FILE_PATH || (ot->mode & (VFS_MODE_READ | VFS_MODE_WRITE)) == 0)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    file = fat16_extract_file_from_path(ot->name, &path);
    if (file == NULL)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (path)
        if (!fat16_get_path(fat16, &fi, path))
            return;
    if (!fat16_find_by_name(fat16, &fi, file, 0, FAT_FILE_ATTR_LABEL | FAT_FILE_ATTR_SUBFOLDER))
    {
        if (ot->mode & VFS_MODE_WRITE)
        {
            //try to create file
            if (!fat16_create_file_entry(fat16, &fi, file, FAT_FILE_ATTR_ARCHIVE))
                return;
        }
        else
        {
            error(ERROR_NOT_FOUND);
            return;
        }
    }
    h = so_allocate(&fat16->file_handles);
    if (h == INVALID_HANDLE)
        return;
    f = so_get(&fat16->file_handles, h);
    memcpy(&f->fi, &fi, sizeof(FAT16_FILE_INFO));
    entry = fat16_read_file_entry(fat16, &fi);
    fat16_fi_create(&f->data, entry->first_cluster);
    f->size = entry->size;
    f->mode = ot->mode;

    *((HANDLE*)io_data(io)) = h;
    io->data_size = sizeof(HANDLE);
    io_complete(process, HAL_IO_CMD(HAL_VFS, IPC_OPEN), folder, io);
    error(ERROR_SYNC);
}

static inline void fat16_seek_file(FAT16_TYPE* fat16, HANDLE h, unsigned int pos)
{
    FAT16_FILE_HANDLE_TYPE* f;
    unsigned int cluster_num;
    f = so_get(&fat16->file_handles, h);
    if (f == NULL)
        return;
    if (pos > f->size)
        f->data.pos = f->size;
    else
        f->data.pos = pos;
    if ((f->data.pos == f->size) && f->size)
        cluster_num = (f->size - 1) / fat16->cluster_size;
    else
        cluster_num = f->data.pos / fat16->cluster_size;
    if (!fat16_fi_get_cluster_num(fat16, &f->data, cluster_num))
    {
        fat16_fi_reset(&f->data);
        f->data.pos = 0;
    }
}

static inline void fat16_read_file(FAT16_TYPE* fat16, HANDLE h, IO* io, unsigned int size, HANDLE process)
{
    FAT16_FILE_HANDLE_TYPE* f;
    unsigned int cluster_offset, sector_offset, chunk, sector, sectors_count, next_cluster;
    f = so_get(&fat16->file_handles, h);
    if (f == NULL)
        return;
    if ((f->mode & VFS_MODE_READ) == 0)
    {
        error(ERROR_ACCESS_DENIED);
        return;
    }
    if (f->data.pos >= f->size)
    {
        error(ERROR_EOF);
        return;
    }
    if (f->size - f->data.pos < size)
        size = f->size - f->data.pos;
    io->data_size = 0;
    while(size)
    {
        cluster_offset = f->data.pos % fat16->cluster_size;
        sector_offset = cluster_offset % FAT_SECTOR_SIZE;
        sector = cluster_offset / FAT_SECTOR_SIZE;
        sectors_count = (size + sector_offset + FAT_SECTOR_SIZE - 1) / FAT_SECTOR_SIZE;
        if (sectors_count + sector > fat16->cluster_sectors)
            sectors_count = fat16->cluster_sectors - sector;
        chunk = sectors_count * FAT_SECTOR_SIZE - sector_offset;
        if (chunk > size)
            chunk = size;

        if (!fat16_read_sectors(fat16, fat16_cluster_to_sector(fat16, f->data.current_cluster) + sector, sectors_count * FAT_SECTOR_SIZE))
            return;
        io_data_append(io, (uint8_t*)io_data(fat16->io) + sector_offset, chunk);
        size -= chunk;
        f->data.pos += chunk;
        if ((f->data.pos < f->size) && ((f->data.pos % fat16->cluster_size) == 0))
        {
            next_cluster = fat16_get_fat_next(fat16, f->data.current_cluster);
            if (next_cluster >= FAT_CLUSTER_RESERVED)
            {
                fat16_fi_reset(&f->data);
                f->data.pos = 0;
                return;
            }
            else
                f->data.current_cluster = next_cluster;
        }
    }
    io_complete(process, HAL_IO_CMD(HAL_VFS, IPC_READ), h, io);
    error(ERROR_SYNC);
}

static inline void fat16_write_file(FAT16_TYPE* fat16, HANDLE h, IO* io, HANDLE process)
{
    FAT16_FILE_HANDLE_TYPE* f;
    FAT_FILE_ENTRY* entry;
    uint8_t* data;
    unsigned int cluster_offset, sector_offset, chunk, sector, sectors_count, next_cluster, size;
    f = so_get(&fat16->file_handles, h);
    if (f == NULL)
        return;
    if ((f->mode & VFS_MODE_WRITE) == 0)
    {
        error(ERROR_ACCESS_DENIED);
        return;
    }
    data = io_data(io);
    for (size = io->data_size; size; size -= chunk)
    {
        cluster_offset = f->data.pos % fat16->cluster_size;
        sector_offset = cluster_offset % FAT_SECTOR_SIZE;
        sector = cluster_offset / FAT_SECTOR_SIZE;
        sectors_count = (size + sector_offset + FAT_SECTOR_SIZE - 1) / FAT_SECTOR_SIZE;
        if (sectors_count + sector > fat16->cluster_sectors)
            sectors_count = fat16->cluster_sectors - sector;
        chunk = sectors_count * FAT_SECTOR_SIZE - sector_offset;
        if (chunk > size)
            chunk = size;

        //append cluster. Empty file already have one
        if ((f->data.pos == f->size) && (cluster_offset == 0) && f->size)
        {
            next_cluster = fat16_occupy_next_cluster(fat16, f->data.current_cluster);
            if (next_cluster >= FAT_CLUSTER_RESERVED)
                break;
            f->data.current_cluster = next_cluster;
        }

        //readout first, no align
        if (sector_offset || (chunk % FAT_SECTOR_SIZE))
        {
            if (!fat16_read_sectors(fat16, fat16_cluster_to_sector(fat16, f->data.current_cluster) + sector, sectors_count * FAT_SECTOR_SIZE))
                break;
        }

        //overwrite data
        memcpy((uint8_t*)io_data(fat16->io) + sector_offset, data, chunk);
        data += chunk;

        //writeback
        if (!fat16_write_sectors(fat16, fat16_cluster_to_sector(fat16, f->data.current_cluster) + sector, sectors_count * FAT_SECTOR_SIZE))
            break;

        f->data.pos += chunk;
        //append to end of file
        if (f->data.pos > f->size)
            f->size = f->data.pos;

        if ((f->data.pos < f->size) && ((f->data.pos % fat16->cluster_size) == 0))
        {
            next_cluster = fat16_get_fat_next(fat16, f->data.current_cluster);
            if (next_cluster >= FAT_CLUSTER_RESERVED)
            {
                fat16_fi_reset(&f->data);
                f->data.pos = 0;
                break;
            }
            else
                f->data.current_cluster = next_cluster;
        }
    }
    //update file attributes
    entry = fat16_read_file_entry(fat16, &f->fi);
    if (entry)
    {
        entry->mod_date = fat16_fat_date_now();
        entry->mod_time = fat16_fat_time_now();
        entry->size = f->size;
        fat16_write_file_entry(fat16);
    }
    io_complete_ex(process, HAL_IO_CMD(HAL_VFS, IPC_WRITE),  h, io, io->data_size - size);
    error(ERROR_SYNC);
}

static inline void fat16_remove(FAT16_TYPE* fat16, unsigned int folder, IO* io, HANDLE process)
{
    FAT16_FILE_INFO fi;
    FAT_FILE_ENTRY* entry;
    char* file;
    char* path;
    char* file_path;
    unsigned long first_cluster;

    if (fat16->volume.process == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    fat16_fi_create(&fi, folder);
    file_path = io_data(io);

    if (strlen(file_path) > VFS_MAX_FILE_PATH)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    file = fat16_extract_file_from_path(file_path, &path);
    if (file == NULL)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (path)
        if (!fat16_get_path(fat16, &fi, path))
            return;
    if (!fat16_find_by_name(fat16, &fi, file, 0, FAT_FILE_ATTR_LABEL))
    {
        error(ERROR_NOT_FOUND);
        return;
    }
    entry = fat16_read_file_entry(fat16, &fi);
    if (entry == NULL)
        return;
    first_cluster = entry->first_cluster;
    if (entry->attr & FAT_FILE_ATTR_SUBFOLDER)
    {
        if (!fat16_is_folder_empty(fat16, &fi))
        {
            error(ERROR_FOLDER_NOT_EMPTY);
            return;
        }
    }
    if (!fat16_release_chain(fat16, first_cluster))
        return;
    if (!fat16_remove_file_entry(fat16, &fi))
        return;

    io->data_size = 0;
    io_complete(process, HAL_IO_CMD(HAL_VFS, VFS_REMOVE), folder, io);
    error(ERROR_SYNC);
}

static inline void fat16_mk_folder(FAT16_TYPE* fat16, unsigned int folder, IO* io, HANDLE process)
{
    FAT16_FILE_INFO fi;
    FAT16_FILE_INFO folder_fi;
    FAT_FILE_ENTRY* entry;
    unsigned int first_cluster;
    char* file;
    char* path;
    char* file_path;

    if (fat16->volume.process == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    fat16_fi_create(&fi, folder);
    file_path = io_data(io);

    if (strlen(file_path) > VFS_MAX_FILE_PATH)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    file = fat16_extract_file_from_path(file_path, &path);
    if (file == NULL)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (path)
        if (!fat16_get_path(fat16, &fi, path))
            return;
    if (!fat16_create_file_entry(fat16, &fi, file, FAT_FILE_ATTR_SUBFOLDER))
        return;

    entry = fat16_read_file_entry(fat16, &fi);
    if (entry == NULL)
        return;
    first_cluster = entry->first_cluster;

    //zero items cluster
    if (!fat16_zero_sectors(fat16, fat16_cluster_to_sector(fat16, first_cluster), fat16->cluster_sectors))
        return;

    //cd folder
    fat16_fi_create(&folder_fi, first_cluster);
    //mkdir .
    entry = fat16_init_file_entry(fat16, &folder_fi);
    entry->attr = FAT_FILE_ATTR_SUBFOLDER;
    entry->first_cluster = first_cluster;
    entry->name[0] = '.';
    if (!fat16_write_file_entry(fat16))
        return;

    //mkdir ..
    folder_fi.pos = 1;
    entry = fat16_init_file_entry(fat16, &folder_fi);
    entry->attr = FAT_FILE_ATTR_SUBFOLDER;
    entry->first_cluster = fi.first_cluster;
    entry->name[0] = entry->name[1] = '.';
    if (!fat16_write_file_entry(fat16))
        return;

    io->data_size = 0;
    io_complete(process, HAL_IO_CMD(HAL_VFS, VFS_MK_FOLDER), folder, io);
    error(ERROR_SYNC);
}

static inline void fat16_format(FAT16_TYPE* fat16, IO* io, HANDLE process)
{
    unsigned int root_sectors, fat_sectors, clusters_count, i;
    FAT_BOOT_SECTOR_BPB_TYPE* bpb;
    uint8_t* boot;
    uint16_t* fat;
    FAT16_FILE_INFO fi;
    FAT_FILE_ENTRY* entry;
    VFS_FAT_FORMAT_TYPE* format = io_data(io);

    if (fat16->volume.process != INVALID_HANDLE)
    {
        error(ERROR_ACCESS_DENIED);
        return;
    }
    io->data_size = 0;

    root_sectors = (format->root_entries * sizeof(FAT_FILE_ENTRY) + FAT_SECTOR_SIZE - 1) / FAT_SECTOR_SIZE;
    clusters_count = ((format->volume.sectors_count - 1 - root_sectors) + format->cluster_sectors - 1) / format->cluster_sectors;
    fat_sectors = ((clusters_count + 2) * 2 + FAT_SECTOR_SIZE - 1) / FAT_SECTOR_SIZE;

    fat16_mount_internal(fat16, &format->volume);
    do {
        //generate boot sector
        boot = io_data(fat16->io);
        memcpy(boot, __FAT16_BOOT, FAT_SECTOR_SIZE);
        bpb = (void*)(boot + sizeof(FAT_BOOT_SECTOR_HEADER_TYPE));
        fat16->cluster_sectors = bpb->cluster_sectors = format->cluster_sectors;
        fat16->root_count = bpb->root_count = format->root_entries;;
        fat16->root_sectors = (bpb->root_count * sizeof(FAT_FILE_ENTRY) + FAT_SECTOR_SIZE - 1) / FAT_SECTOR_SIZE;
        fat16->fat_sectors = bpb->fat_sectors = fat_sectors;
        fat16->reserved_sectors = 1;
        bpb->hidden = format->volume.first_sector;
        bpb->sectors = format->volume.sectors_count;
        bpb->serial = format->serial;
        if (!fat16_write_sectors(fat16, 0, FAT_SECTOR_SIZE))
            break;

        //zero fat, root
        if (!fat16_zero_sectors(fat16, 1, fat_sectors * 2 + root_sectors))
            break;

        //init fat (sector is now zero)
        fat = io_data(fat16->io);
        fat[0] = FAT_CLUSTER_RESERVED;
        fat[1] = FAT_CLUSTER_LAST;
        if (!fat16_write_sectors(fat16, fat16->reserved_sectors, FAT_SECTOR_SIZE))
            break;
        if (!fat16_write_sectors(fat16, fat16->reserved_sectors + fat16->fat_sectors, FAT_SECTOR_SIZE))
            break;

        //write label in root fs
        fat16_fi_create(&fi, VFS_ROOT);
        entry = fat16_init_file_entry(fat16, &fi);
        if (entry == NULL)
            break;
        for (i = 0; i < 8 && format->label[i]; ++i)
            entry->name[i] = fat16_char_upper(format->label[i]);
        entry->attr = FAT_FILE_ATTR_LABEL;
        if (fat16_write_file_entry(fat16))
            break;

        io_complete(process, HAL_IO_CMD(HAL_VFS, VFS_FORMAT), 0, io);
        error(ERROR_SYNC);
    } while(false);
    fat16_unmount_internal(fat16);
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
    case VFS_REMOVE:
        fat16_remove(fat16, ipc->param1, (IO*)ipc->param2, ipc->process);
        break;
    case VFS_MK_FOLDER:
        fat16_mk_folder(fat16, ipc->param1, (IO*)ipc->param2, ipc->process);
        break;
    case VFS_FORMAT:
        fat16_format(fat16, (IO*)ipc->param2, ipc->process);
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
