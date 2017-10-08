/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
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

unsigned int ber_get_volume_sectors(VFSS_TYPE *vfss);

bool ber_read_sectors(VFSS_TYPE* vfss, unsigned long sector, unsigned size);
bool ber_write_sectors(VFSS_TYPE* vfss, unsigned long sector, unsigned size);

void ber_init(VFSS_TYPE *vfss);
void ber_request(VFSS_TYPE *vfss, IPC* ipc);

#endif // BER_H
