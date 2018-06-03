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
#define BER_TRANSACTION_INCREMENT           64

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
    uint32_t crc;
    uint32_t revision;
    uint16_t block_sectors;
    uint16_t fs_blocks;
    uint32_t total_blocks;
    uint32_t crc_count;
} BER_HEADER_TYPE;
#pragma pack(pop)

typedef struct {
    uint16_t lblock;
    uint16_t pblock;
} BER_TRANS_ENTRY;

/*
        BER superblock format:
        BER header
        <blocks remap list>
        <blocks stat list>
 */
static void ber_rollback_trans(VFSS_TYPE* vfss);
static inline void ber_trans_clear_buffer(VFSS_TYPE *vfss)
{
    if(vfss->ber.trans_buffer)
        array_destroy(&vfss->ber.trans_buffer);
}

static inline bool ber_trans_is_pblock_lock(VFSS_TYPE *vfss, uint16_t pblock)
{
    int i;
    BER_TRANS_ENTRY* entry;
    if(vfss->ber.trans_buffer == NULL)
        return false;
    for (i = 0; i < array_size(vfss->ber.trans_buffer); i++)
    {
        entry = (BER_TRANS_ENTRY*)array_at(vfss->ber.trans_buffer, i);
        if (entry == NULL)
            return false;
        if (entry->pblock == pblock)
            return true;
    }
    return false;
}

static inline bool ber_trans_add_block(VFSS_TYPE *vfss, uint16_t lblock)
{
    int i;
    BER_TRANS_ENTRY* entry;
    if(vfss->ber.trans_buffer == NULL)
        return true;
    for (i = 0; i < array_size(vfss->ber.trans_buffer); i++)
    {
        entry = (BER_TRANS_ENTRY*)array_at(vfss->ber.trans_buffer, i);
        if (entry == NULL)
            continue;
        if (entry->lblock == lblock)
            return true;
    }
    entry = (BER_TRANS_ENTRY*)array_append(&vfss->ber.trans_buffer);
    if (entry == NULL)
        return false;
    entry->lblock = lblock;
    entry->pblock = vfss->ber.remap_list[lblock];
    return true;
}

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
        if (lblock >= vfss->ber.volume.fs_blocks)
        {
            error(ERROR_OUT_OF_RANGE);
            return false;
        }
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
        if(ber_trans_is_pblock_lock(vfss, i))
            continue;
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
        ber_rollback_trans(vfss);
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

static uint32_t ber_superblock_crc(VFSS_TYPE* vfss, IO* io)
{
    uint16_t len = sizeof(BER_HEADER_TYPE) - 8 + vfss->ber.volume.fs_blocks * sizeof(uint16_t) + vfss->ber.total_blocks * sizeof(uint32_t);
    if(len > io->size)
        return 0;
    len /= sizeof(uint16_t);
    uint32_t crc  = 0;
    uint16_t* ptr = io_data(io);
    ptr +=  8 / sizeof(uint16_t);
    do
    {
        crc += *ptr++;
    }while(--len);
    return crc;
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
    hdr->crc = ber_superblock_crc(vfss, vfss->ber.io);
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
#if (VFS_BER_DEBUG_INFO)
            printf("BER: write pblock %#x  version %#x super %#x\n", pblock, vfss->ber.ber_revision, is_super);
#endif // VFS_BER_DEBUG_INFO

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
#if (VFS_BER_DEBUG_INFO)
    printf("BER: write lblock %#x  ", lblock);
#endif // VFS_BER_DEBUG_INFO

    if(!ber_trans_add_block(vfss, lblock))
    {
        ber_rollback_trans(vfss);
        return false;
    }

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
        if (lblock >= vfss->ber.volume.fs_blocks)
        {
            error(ERROR_OUT_OF_RANGE);
            return false;
        }
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
    if(vfss->ber.trans_buffer == NULL)
    {
        if (!ber_update_superblock(vfss))
            return false;
    }
    return true;
}

void ber_init(VFSS_TYPE *vfss)
{
    vfss->ber.active = false;
    vfss->ber.io = NULL;
    vfss->ber.remap_list = NULL;
    vfss->ber.stat_list = NULL;
    vfss->ber.trans_buffer = NULL;
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
    vfss_resize_buf(vfss, vfss->ber.block_size);
    //try to find latest superblock
    revision = 0;
    for (i = 0; i < vfss->ber.total_blocks; ++i)
    {
        if (!storage_read_sync(vfss->volume.hal, vfss->volume.process, vfss->volume.user, vfss->io, vfss->volume.first_sector + i * block_sectors, vfss->ber.block_size))
            return;
        hdr = vfss_get_buf(vfss);
        if ((hdr->magic == BER_MAGIC) && (hdr->revision > revision))
        {
            vfss->ber.total_blocks = hdr->total_blocks;
            vfss->ber.volume.fs_blocks = hdr->fs_blocks;
            if(hdr->crc == ber_superblock_crc(vfss, vfss->io))
            {
                revision = hdr->revision;
                vfss->ber.superblock = i;
            }
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

static void ber_close(VFSS_TYPE* vfss)
{
    ber_close_internal(vfss);
    ber_trans_clear_buffer(vfss);
    vfss->ber.active = false;
}

static inline void ber_read(VFSS_TYPE* vfss, IO* io, unsigned int size)
{
    unsigned int sector, chunk_size;
    STORAGE_STACK* stack = io_stack(io);
    io_pop(io, sizeof(STORAGE_STACK));
    if (size % FAT_SECTOR_SIZE)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    for (io->data_size = 0, sector = stack->sector; io->data_size < size; sector += chunk_size / FAT_SECTOR_SIZE)
    {
        chunk_size = (size - io->data_size);
        if (chunk_size > vfss->ber.block_size)
            chunk_size = vfss->ber.block_size;
        if (!ber_read_sectors(vfss, sector, chunk_size))
            return;
        io_data_append(io, vfss_get_buf(vfss), chunk_size);
    }
}

static inline void ber_write(VFSS_TYPE* vfss, IO* io)
{
    unsigned int sector, chunk_size, offset;
    STORAGE_STACK* stack = io_stack(io);
    io_pop(io, sizeof(STORAGE_STACK));
    if (io->data_size % FAT_SECTOR_SIZE)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    for (offset = 0, sector = stack->sector; offset < io->data_size; sector += chunk_size / FAT_SECTOR_SIZE)
    {
        chunk_size = (io->data_size - offset);
        if (chunk_size > vfss->ber.block_size)
            chunk_size = vfss->ber.block_size;
        memcpy(vfss_get_buf(vfss), (uint8_t*)io_data(io) + offset, chunk_size);
        if (!ber_write_sectors(vfss, sector, chunk_size))
            return;
    }
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

static inline void ber_start_trans(VFSS_TYPE* vfss)
{
    if (vfss->ber.trans_buffer != NULL)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
    if(array_create(&vfss->ber.trans_buffer, sizeof(BER_TRANS_ENTRY), BER_TRANSACTION_INCREMENT) == NULL)
        error(ERROR_OUT_OF_MEMORY);
}

static inline void ber_commit_trans(VFSS_TYPE* vfss)
{
    if(vfss->ber.trans_buffer == NULL)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
    if(array_size(vfss->ber.trans_buffer))
    {
        if(!ber_update_superblock(vfss))
            error(ERROR_OUT_OF_MEMORY);
    }
    ber_trans_clear_buffer(vfss);
}

static void ber_rollback_trans(VFSS_TYPE* vfss)
{
    uint32_t block_sectors;
    if(vfss->ber.trans_buffer == NULL)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
    if(array_size(vfss->ber.trans_buffer) == 0)
    {
        ber_trans_clear_buffer(vfss);
        return;
    }
    block_sectors = vfss->ber.block_size / FAT_SECTOR_SIZE;
    ber_close(vfss);
    ber_open(vfss, block_sectors);
}

void ber_request(VFSS_TYPE *vfss, IPC* ipc)
{
    if((!vfss->ber.active) && (HAL_ITEM(ipc->cmd) != IPC_OPEN) && (HAL_ITEM(ipc->cmd) != VFS_FORMAT) )
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }

    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        ber_open(vfss, ipc->param2);
        break;
    case IPC_CLOSE:
        ber_close(vfss);
        break;
    case IPC_READ:
        ber_read(vfss, (IO*)ipc->param2, ipc->param3);
        break;
    case IPC_WRITE:
        ber_write(vfss, (IO*)ipc->param2);
        break;
    case VFS_FORMAT:
        ber_format(vfss, (IO*)ipc->param2);
        break;
    case VFS_STAT:
        ber_stat(vfss, (IO*)ipc->param2);
        break;
    case VFS_START_TRANSACTION:
        ber_start_trans(vfss);
        break;
    case VFS_COMMIT_TRANSACTION:
        ber_commit_trans(vfss);
        break;
    case VFS_ROLLBACK_TRANSACTION:
        ber_rollback_trans(vfss);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}
