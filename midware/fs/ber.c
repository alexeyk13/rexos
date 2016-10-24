/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "ber.h"
#include "vfss_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/error.h"
#include "sys_config.h"
#include <string.h>

#define BER_MAGIC                           0xf247f2a0

#define BER_BLOCK_UNUSED                    0xffff
#define BER_STAT_BLOCK_USED                 (1 << 31)
#define BER_STAT_BLOCK_STUFFED              (1 << 30)
#define BER_STAT_VALUE(raw)                 ((raw) & 0x3fffffff)

#define BER_MAGIC_FLASH_UNINITIALIZED       0xff

#pragma pack(push, 1)

typedef struct {
    uint32_t magic;
    uint32_t revision;
    uint16_t block_sectors;
    uint16_t total_blocks;
    uint16_t fs_blocks;
} BER_HEADER_TYPE;

#pragma pack(pop)

/*
        BER superblock format:
        BER header
        <blocks remap list>
        <blocks stat list>
 */


unsigned int ber_get_volume_sectors(VFSS_TYPE *vfss)
{
    return vfss->ber.volume.block_sectors * vfss->ber.volume.fs_blocks;
}

bool ber_read_sectors(VFSS_TYPE* vfss, unsigned long sector, unsigned size)
{
    unsigned int i, size_sectors, lblock, pblock, pblock_offset, sectors_to_read;
    uint8_t* buf = vfss_get_buf(vfss);
    size_sectors = size / FAT_SECTOR_SIZE;
    for (i = 0; i < size_sectors; i += sectors_to_read)
    {
        lblock = (sector + i) / vfss->ber.volume.block_sectors;
        pblock = vfss->ber.remap_list[lblock];
        pblock_offset = (sector + i) % vfss->ber.volume.block_sectors;
        sectors_to_read = vfss->ber.volume.block_sectors - pblock_offset;
        if (sectors_to_read > size_sectors - i)
            sectors_to_read = size_sectors - i;
        if (pblock == BER_BLOCK_UNUSED)
            memset(buf + i * FAT_SECTOR_SIZE, BER_MAGIC_FLASH_UNINITIALIZED, sectors_to_read * FAT_SECTOR_SIZE);
        else
        {
            if (!storage_read_sync(vfss->volume.hal, vfss->volume.process, vfss->volume.user, vfss->ber.io,
                                   vfss->volume.first_sector + pblock * vfss->ber.volume.block_sectors + pblock_offset,
                                   sectors_to_read * FAT_SECTOR_SIZE))
                return false;
            //unstuff
            if (vfss->ber.stat_list[pblock] & BER_STAT_BLOCK_STUFFED)
                *((uint32_t*)io_data(vfss->ber.io)) = BER_MAGIC;
            memcpy(buf + i * FAT_SECTOR_SIZE, io_data(vfss->ber.io), sectors_to_read * FAT_SECTOR_SIZE);
        }
    }
    return true;
}

static uint16_t ber_find_best(VFSS_TYPE* vfss, uint16_t current_pblock)
{
    uint16_t best_pblock, i;
    uint32_t best_pblock_stat;
    best_pblock = BER_BLOCK_UNUSED;
    best_pblock_stat = 0xffffffff;
    for (i = 0; i < vfss->ber.total_blocks; ++i)
    {
        if (vfss->ber.stat_list[i] & BER_STAT_BLOCK_USED)
            continue;
        if (BER_STAT_VALUE(vfss->ber.stat_list[i]) < best_pblock_stat)
        {
            best_pblock = i;
            best_pblock_stat = BER_STAT_VALUE(vfss->ber.stat_list[i]);
        }
    }
    if (best_pblock == BER_BLOCK_UNUSED)
    {
#if (VFS_BER_DEBUG_ERRORS)
        printf("BER: no free blocks\n");
#endif //VFS_BER_DEBUG_ERRORS
        error(ERROR_FULL);
    }
    if (current_pblock != BER_BLOCK_UNUSED)
        vfss->ber.stat_list[current_pblock] &= ~(BER_STAT_BLOCK_USED | BER_STAT_BLOCK_STUFFED);
    vfss->ber.stat_list[best_pblock] = (vfss->ber.stat_list[best_pblock] + 1) | BER_STAT_BLOCK_USED;
    return best_pblock;
}

static inline bool ber_read_lblock(VFSS_TYPE* vfss, uint16_t lblock)
{
    if (vfss->ber.remap_list[lblock] == BER_BLOCK_UNUSED)
    {
        memset(io_data(vfss->ber.io), BER_MAGIC_FLASH_UNINITIALIZED, vfss->ber.block_size);
        return true;
    }
    return storage_read_sync(vfss->volume.hal, vfss->volume.process, vfss->volume.user, vfss->ber.io, vfss->volume.first_sector +
                             vfss->ber.remap_list[lblock] * vfss->ber.volume.block_sectors, vfss->ber.block_size);
}

static inline bool ber_write_lblock(VFSS_TYPE* vfss, uint16_t lblock)
{
    uint16_t best_pblock = ber_find_best(vfss, vfss->ber.remap_list[lblock]);
    if (best_pblock == BER_BLOCK_UNUSED)
        return false;
    vfss->ber.remap_list[lblock] = best_pblock;
    //extremely rare, but possible. Stuff header
    if (*((uint32_t*)io_data(vfss->ber.io)) == BER_MAGIC)
    {
        *((uint32_t*)io_data(vfss->ber.io)) = 0x00000000;
        vfss->ber.stat_list[best_pblock] |= BER_STAT_BLOCK_STUFFED;
    }
    vfss->ber.io->data_size = vfss->ber.block_size;
    return storage_write_sync(vfss->volume.hal, vfss->volume.process, vfss->volume.user, vfss->ber.io, vfss->volume.first_sector +
                              best_pblock * vfss->ber.volume.block_sectors);
}

static inline bool ber_update_superblock(VFSS_TYPE* vfss)
{
    BER_HEADER_TYPE* hdr;
    uint16_t best_pblock = ber_find_best(vfss, vfss->ber.superblock);
    if (best_pblock == BER_BLOCK_UNUSED)
        return false;
    vfss->ber.superblock = best_pblock;
    ++vfss->ber.ber_revision;
    hdr = io_data(vfss->ber.io);
    hdr->magic = BER_MAGIC;
    hdr->revision = vfss->ber.ber_revision;
    hdr->block_sectors = vfss->ber.volume.block_sectors;
    hdr->total_blocks = vfss->ber.total_blocks;
    hdr->fs_blocks = vfss->ber.volume.fs_blocks;

    memcpy((uint8_t*)io_data(vfss->ber.io) + sizeof(BER_HEADER_TYPE),
           vfss->ber.remap_list, vfss->ber.volume.fs_blocks * sizeof(uint16_t));
    memcpy((uint8_t*)io_data(vfss->ber.io) + sizeof(BER_HEADER_TYPE) + vfss->ber.volume.fs_blocks * sizeof(uint16_t),
           vfss->ber.stat_list, vfss->ber.total_blocks * sizeof(uint32_t));

    vfss->ber.io->data_size = vfss->ber.block_size;
    return storage_write_sync(vfss->volume.hal, vfss->volume.process, vfss->volume.user, vfss->ber.io, vfss->volume.first_sector +
                              best_pblock * vfss->ber.volume.block_sectors);
}

bool ber_write_sectors(VFSS_TYPE* vfss, unsigned long sector, unsigned size)
{
    unsigned int i, size_sectors, lblock, lblock_offset, sectors_to_write;
    uint8_t* buf = vfss_get_buf(vfss);
    size_sectors = size / FAT_SECTOR_SIZE;
    for (i = 0; i < size_sectors; i += sectors_to_write)
    {
        lblock = (sector + i) / vfss->ber.volume.block_sectors;
        lblock_offset = (sector + i) % vfss->ber.volume.block_sectors;
        sectors_to_write = vfss->ber.volume.block_sectors - lblock_offset;
        if (sectors_to_write > size_sectors - i)
            sectors_to_write = size_sectors - i;
        //readback first
        if (lblock_offset || (sectors_to_write < vfss->ber.volume.block_sectors))
        {
            if (!ber_read_lblock(vfss, lblock))
                return false;
        }
        memcpy(io_data(vfss->ber.io) + lblock_offset * FAT_SECTOR_SIZE, buf + i * FAT_SECTOR_SIZE, sectors_to_write * FAT_SECTOR_SIZE);
        if (!ber_write_lblock(vfss, lblock))
            return false;
    }
    if (!ber_update_superblock(vfss))
        return false;
    return true;
}

void ber_init(VFSS_TYPE *vfss)
{
    vfss->ber.active = false;
    vfss->ber.io = NULL;
    vfss->ber.remap_list = NULL;
    vfss->ber.stat_list = NULL;
}

static void ber_close_internal(VFSS_TYPE* vfss)
{
    free(vfss->ber.remap_list);
    vfss->ber.remap_list = NULL;
    vfss->ber.stat_list = NULL;
    free(vfss->ber.stat_list);
    io_destroy(vfss->ber.io);
    vfss->ber.io = NULL;
}

static inline void ber_open(VFSS_TYPE* vfss, unsigned int block_sectors)
{
    unsigned int i, revision;
    BER_HEADER_TYPE* hdr;
    if (vfss->ber.active)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    vfss->ber.total_blocks = vfss->volume.sectors_count / block_sectors;
    vfss->ber.block_size = block_sectors * FAT_SECTOR_SIZE;
    //try to find latest superblock
    revision = 0;
    for (i = 0; i < vfss->ber.total_blocks; ++i)
    {
        if (!storage_read_sync(vfss->volume.hal, vfss->volume.process, vfss->volume.user, vfss->io, vfss->volume.first_sector + i * block_sectors, FAT_SECTOR_SIZE))
            return;
        hdr = vfss_get_buf(vfss);
        if ((hdr->magic == BER_MAGIC) && (hdr->revision > revision))
        {
            revision = hdr->revision;
            vfss->ber.superblock = i;
        }
    }
    if (revision == 0)
    {
#if (VFS_BER_DEBUG_ERRORS)
        printf("BER: no superblock found\n");
#endif //VFS_BER_DEBUG_ERRORS
        error(ERROR_CORRUPTED);
        return;
    }
    vfss_resize_buf(vfss, vfss->ber.block_size);
    if (!storage_read_sync(vfss->volume.hal, vfss->volume.process, vfss->volume.user, vfss->io,
                           vfss->volume.first_sector + vfss->ber.superblock * block_sectors, vfss->ber.block_size))
        return;
    hdr = vfss_get_buf(vfss);
    if (hdr->block_sectors != block_sectors)
    {
#if (VFS_BER_DEBUG_ERRORS)
        printf("BER: superblock block size doesn't match requested\n");
#endif //VFS_BER_DEBUG_ERRORS
        error(ERROR_CORRUPTED);
        return;
    }
    vfss->ber.volume.block_sectors = hdr->block_sectors;
    vfss->ber.volume.fs_blocks = hdr->fs_blocks;
    vfss->ber.total_blocks = hdr->total_blocks;
    vfss->ber.io = io_create(vfss->ber.block_size + sizeof(STORAGE_STACK));
    vfss->ber.remap_list = malloc(hdr->fs_blocks * sizeof(uint16_t));
    vfss->ber.stat_list = malloc(hdr->total_blocks * sizeof(uint32_t));
    vfss->ber.ber_revision = hdr->revision;
    if (vfss->ber.io == NULL || vfss->ber.remap_list == NULL || vfss->ber.stat_list == NULL)
    {
        ber_close_internal(vfss);
        return;
    }
    memcpy(vfss->ber.remap_list, (uint8_t*)vfss_get_buf(vfss) + sizeof(BER_HEADER_TYPE), hdr->fs_blocks * sizeof(uint16_t));
    memcpy(vfss->ber.stat_list, (uint8_t*)vfss_get_buf(vfss) + sizeof(BER_HEADER_TYPE) + hdr->fs_blocks * sizeof(uint16_t), hdr->total_blocks * sizeof(uint32_t));

#if (VFS_BER_DEBUG_INFO)
    printf("BER: mounted, FS size: %dKB\n", vfss->ber.volume.fs_blocks * vfss->ber.volume.block_sectors / 2);
#endif //VFS_BER_DEBUG_INFO
    vfss->ber.active = true;
}

static inline void ber_close(VFSS_TYPE* vfss)
{
    if (!vfss->ber.active)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    ber_close_internal(vfss);
    vfss->ber.active = false;
}

static inline void ber_format(VFSS_TYPE* vfss, IO* io)
{
    unsigned int i;
    uint16_t* remap_list;
    uint32_t* stat_list;
    BER_HEADER_TYPE* hdr;
    VFS_BER_FORMAT_TYPE* format = io_data(io);

    if (vfss->ber.active)
    {
        error(ERROR_INVALID_MODE);
        return;
    }
    //make sure fits in one superblock
    vfss->ber.total_blocks = vfss->volume.sectors_count / format->block_sectors;
    vfss->ber.block_size = format->block_sectors * FAT_SECTOR_SIZE;
    if (sizeof(BER_HEADER_TYPE) +  format->fs_blocks * sizeof(uint16_t) + vfss->ber.total_blocks * sizeof(uint32_t) > vfss->ber.block_size)
    {
#if (VFS_BER_DEBUG_ERRORS)
        printf("BER: data doesn't fits in superblock!\n");
#endif //VFS_BER_DEBUG_ERRORS
        error(ERROR_INVALID_PARAMS);
        return;
    }

    //erase all superblocks
    for (i = 0; i < vfss->ber.total_blocks; ++i)
    {
        if (!storage_read_sync(vfss->volume.hal, vfss->volume.process, vfss->volume.user, vfss->io,
                               vfss->volume.first_sector + i * format->block_sectors, FAT_SECTOR_SIZE))
            return;
        hdr = vfss_get_buf(vfss);
        if ((hdr->magic == BER_MAGIC))
            if (!storage_erase_sync(vfss->volume.hal, vfss->volume.process, vfss->volume.user, vfss->io,
                                    vfss->volume.first_sector + i * format->block_sectors, FAT_SECTOR_SIZE))
                return;
    }

    //make superblock
    //header
    vfss_resize_buf(vfss, vfss->ber.block_size);
    hdr = vfss_get_buf(vfss);
    hdr->magic = BER_MAGIC;
    hdr->revision = 1;
    hdr->block_sectors = format->block_sectors;
    hdr->total_blocks = vfss->ber.total_blocks;
    hdr->fs_blocks = format->fs_blocks;

    //remap list - all empty blocks
    remap_list = (uint16_t*)((uint8_t*)vfss_get_buf(vfss) + sizeof(BER_HEADER_TYPE));
    for (i = 0; i < format->fs_blocks; ++i)
        remap_list[i] = BER_BLOCK_UNUSED;

    //stat list
    stat_list = (uint32_t*)((uint8_t*)vfss_get_buf(vfss) + sizeof(BER_HEADER_TYPE) + format->fs_blocks * sizeof(uint16_t));
    //first reserve for superblock
    stat_list[0] = 1 | BER_STAT_BLOCK_USED;
    for (i = 1; i < vfss->ber.total_blocks; ++i)
        stat_list[i] = 0;

    //save superblock
    vfss->io->data_size = vfss->ber.block_size;
    storage_write_sync(vfss->volume.hal, vfss->volume.process, vfss->volume.user, vfss->io, vfss->volume.first_sector);
}

void ber_request(VFSS_TYPE *vfss, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        ber_open(vfss, ipc->param2);
        break;
    case IPC_CLOSE:
        ber_close(vfss);
        break;
    case VFS_FORMAT:
        ber_format(vfss, (IO*)ipc->param2);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}
