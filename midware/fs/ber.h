/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef BER_H
#define BER_H

#include "vfss.h"
#include <stdbool.h>
#include <stdint.h>
#include "../../userspace/vfs.h"
#include "../../userspace/io.h"
#include "../../userspace/ipc.h"
#include "../../userspace/array.h"

#if  (VFS_BER2)
typedef struct {
//    uint32_t min_idx;
    uint16_t free;
    uint16_t outdated;
} BER2_BLOCK_INFO;

typedef struct {
    ARRAY* trans_buffer;
    uint8_t* cash_buffer;
    uint16_t cash_lsector;
    uint16_t* remap_list;
    BER2_BLOCK_INFO* block_info;
    uint32_t curr_idx;
    uint32_t curr_block;
    uint32_t next_block;
    uint32_t bad_blocks;
    uint32_t free_blocks;
    bool is_transaction, req_transaction, req_end_transaction;

    unsigned int total_blocks, total_sectors, block_size, sectors_per_block, sector_size;
    IO* io;
    bool active;
} BER_TYPE;

#else
typedef struct {
    VFS_BER_FORMAT_TYPE volume;
    ARRAY* trans_buffer;
    uint16_t superblock;
    unsigned int total_blocks, ber_revision, block_size, crc_count;
    uint16_t* remap_list;
    uint32_t* stat_list;
    IO* io;
    bool active;
} BER_TYPE;

#endif

unsigned int ber_get_volume_sectors(VFSS_TYPE *vfss);

bool ber_read_sectors(VFSS_TYPE* vfss, unsigned long sector, unsigned size);
bool ber_write_sectors(VFSS_TYPE* vfss, unsigned long sector, unsigned size);

void ber_init(VFSS_TYPE *vfss);
void ber_request(VFSS_TYPE *vfss, IPC* ipc);

//only for ber2
unsigned int ber_get_sector_size(VFSS_TYPE* vfss);
void ber_defrag(VFSS_TYPE* vfss);
void ber_delete_sector(VFSS_TYPE* vfss, uint32_t lsector);


#endif // BER_H
