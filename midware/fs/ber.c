/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "ber.h"
#include "vfss_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/error.h"
#include "sys_config.h"
#include <string.h>

#define BER_MAGIC                           0x426b7189

#define BER_BLOCK_UNUSED                    0xffff
#define BER_STAT_BLOCK_USED                 (1 << 31)
#define BER_STAT_BLOCK_STUFFED              (1 << 30)
#define BER_STAT_BLOCK_CRC                  (1 << 29)
#define BER_STAT_VALUE(raw)                 ((raw) & 0x1fffffff)
#define BER_STAT_BAD_BLOCK                  0xffffffff

#define BER_MAGIC_FLASH_UNINITIALIZED       0xff

#pragma pack(push, 1)

typedef struct {
    uint32_t magic;
    uint32_t revision;
    uint16_t block_sectors;
    uint16_t fs_blocks;
    uint32_t total_blocks;
    uint32_t crc_count;
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

static uint16_t ber_find_best(VFSS_TYPE* vfss)
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
    return best_pblock;
}

static void ber_use_block(VFSS_TYPE* vfss, uint16_t pblock)
{
    vfss->ber.stat_list[pblock] = (BER_STAT_VALUE(vfss->ber.stat_list[pblock]) + 1) |
                                  (vfss->ber.stat_list[pblock] & BER_STAT_BLOCK_CRC) |
                                  BER_STAT_BLOCK_USED;
}

static void ber_free_block(VFSS_TYPE* vfss, uint16_t pblock)
{
    if (pblock != BER_BLOCK_UNUSED)
        vfss->ber.stat_list[pblock] &= ~(BER_STAT_BLOCK_USED | BER_STAT_BLOCK_STUFFED);
}

static void ber_prepare_superblock(VFSS_TYPE* vfss, uint16_t pblock)
{
    uint32_t old_stat_superblock, old_stat_pblock;
    BER_HEADER_TYPE* hdr;

    old_stat_superblock = vfss->ber.stat_list[vfss->ber.superblock];
    ber_free_block(vfss, vfss->ber.superblock);
    old_stat_pblock = vfss->ber.stat_list[pblock];
    ber_use_block(vfss, pblock);

    hdr = io_data(vfss->ber.io);
    hdr->magic = BER_MAGIC;
    hdr->revision = vfss->ber.ber_revision + 1;
    hdr->block_sectors = vfss->ber.volume.block_sectors;
    hdr->total_blocks = vfss->ber.total_blocks;
    hdr->fs_blocks = vfss->ber.volume.fs_blocks;
    hdr->crc_count = vfss->ber.crc_count;

    memcpy((uint8_t*)io_data(vfss->ber.io) + sizeof(BER_HEADER_TYPE),
           vfss->ber.remap_list, vfss->ber.volume.fs_blocks * sizeof(uint16_t));
    memcpy((uint8_t*)io_data(vfss->ber.io) + sizeof(BER_HEADER_TYPE) + vfss->ber.volume.fs_blocks * sizeof(uint16_t),
           vfss->ber.stat_list, vfss->ber.total_blocks * sizeof(uint32_t));
    //update cache later, only after successfull write
    vfss->ber.stat_list[vfss->ber.superblock] = old_stat_superblock;
    vfss->ber.stat_list[pblock] = old_stat_pblock;
}

static uint16_t ber_write_pblock(VFSS_TYPE* vfss, bool is_super)
{
    unsigned int retry;
    uint16_t pblock;

    vfss->ber.io->data_size = vfss->ber.block_size;
    for (;;)
    {
        pblock = ber_find_best(vfss);
        if (pblock == BER_BLOCK_UNUSED)
            return BER_BLOCK_UNUSED;
        for (retry = 0; retry < 3; ++retry)
        {
            //we need to update superblock statistics on each write - successfull or not
            if (is_super)
                ber_prepare_superblock(vfss, pblock);
            if (storage_write_sync(vfss->volume.hal, vfss->volume.process, vfss->volume.user, vfss->ber.io, vfss->volume.first_sector +
                                      pblock * vfss->ber.volume.block_sectors))
                return pblock;
            error(ERROR_OK);
#if (VFS_BER_DEBUG_ERRORS)
            printf("BER: crc at block %#x\n", pblock);
#endif //VFS_BER_DEBUG_ERRORS
            vfss->ber.stat_list[pblock] |= BER_STAT_BLOCK_CRC;
            ++vfss->ber.crc_count;
        }
        vfss->ber.stat_list[pblock] = BER_STAT_BAD_BLOCK;
#if (VFS_BER_DEBUG_ERRORS)
        printf("BER: marking block %#x as bad\n", pblock);
#endif //VFS_BER_DEBUG_ERRORS
    }
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
    bool stuffed;
    uint16_t pblock;

    //extremely rare, but possible. Stuff header
    stuffed = *((uint32_t*)io_data(vfss->ber.io)) == BER_MAGIC;
    if (stuffed)
        *((uint32_t*)io_data(vfss->ber.io)) = 0x00000000;

    pblock = ber_write_pblock(vfss, false);
    if (pblock == BER_BLOCK_UNUSED)
        return false;

    //update superblock cache only after successfull write
    ber_free_block(vfss, vfss->ber.remap_list[lblock]);
    ber_use_block(vfss, pblock);
    vfss->ber.remap_list[lblock] = pblock;
    if (stuffed)
        vfss->ber.stat_list[pblock] |= BER_STAT_BLOCK_STUFFED;
    return true;
}

static inline bool ber_update_superblock(VFSS_TYPE* vfss)
{
    uint16_t pblock = ber_write_pblock(vfss, true);
    if (pblock == BER_BLOCK_UNUSED)
        return false;

    //update cache only after successfull write
    ++vfss->ber.ber_revision;
    ber_free_block(vfss, vfss->ber.superblock);
    ber_use_block(vfss, pblock);
    vfss->ber.superblock = pblock;
    return true;
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
    free(vfss->ber.stat_list);
    vfss->ber.remap_list = NULL;
    vfss->ber.stat_list = NULL;
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
    vfss->ber.crc_count = hdr->crc_count;
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
        {
            if (!storage_erase_sync(vfss->volume.hal, vfss->volume.process, vfss->volume.user, vfss->io,
                                    vfss->volume.first_sector + i * format->block_sectors, 1))
            {
#if (VFS_BER_DEBUG_ERRORS)
                printf("BER: can't format superblock. Replace flash.\n");
#endif //VFS_BER_DEBUG_ERRORS
                return;
            }
        }
    }

    vfss_resize_buf(vfss, vfss->ber.block_size);
    memset(vfss_get_buf(vfss), 0xff, vfss->ber.block_size);
    vfss->ber.volume.block_sectors = format->block_sectors;
    vfss->ber.volume.fs_blocks = format->fs_blocks;
    vfss->ber.io = io_create(vfss->ber.block_size + sizeof(STORAGE_STACK));
    vfss->ber.remap_list = malloc(vfss->ber.volume.fs_blocks * sizeof(uint16_t));
    vfss->ber.stat_list = malloc(vfss->ber.total_blocks * sizeof(uint32_t));
    vfss->ber.ber_revision = 0;
    vfss->ber.crc_count = 0;
    vfss->ber.superblock = 0;
    if (vfss->ber.io == NULL || vfss->ber.remap_list == NULL || vfss->ber.stat_list == NULL)
    {
        ber_close_internal(vfss);
        return;
    }

    //remap list - all empty blocks
    for (i = 0; i < vfss->ber.volume.fs_blocks; ++i)
        vfss->ber.remap_list[i] = BER_BLOCK_UNUSED;

    //stat list - unused, no CRC, clear
    for (i = 1; i < vfss->ber.total_blocks; ++i)
        vfss->ber.stat_list[i] = 0;

    ber_update_superblock(vfss);
    ber_close_internal(vfss);
}

static inline void ber_stat(VFSS_TYPE* vfss, IO* io)
{
    unsigned int i;
    VFS_BER_STAT_TYPE* stat = io_data(io);

    if (!vfss->ber.active)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    stat->crc_errors_count = vfss->ber.crc_count;
    stat->bad_blocks = stat->crc_blocks = 0;
    for (i = 0; i < vfss->ber.total_blocks; ++i)
    {
        if (vfss->ber.stat_list[i] == BER_STAT_BAD_BLOCK)
            ++stat->bad_blocks;
        else if (vfss->ber.stat_list[i] & BER_STAT_BLOCK_CRC)
            ++stat->crc_blocks;
    }
    io->data_size = sizeof(VFS_BER_STAT_TYPE);
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
    case VFS_STAT:
        ber_stat(vfss, (IO*)ipc->param2);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}
