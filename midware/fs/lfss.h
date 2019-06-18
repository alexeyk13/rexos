/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: zurabob (zurabob@list.ru)
*/

#ifndef LFSS_H
#define LFSS_H

#include "sys_config.h"
#include "object.h"
#include "ipc.h"
#include "io.h"
#include <stdint.h>

#define RBS_SIGN_HI                                         0xCFFF1234
#define RBS_SIGN_LO                                         0x5678FFFC
#define RBS_CHUNK_SIZE                                      16
#define RBS_CHUNK_SHIFT                                     4
#define RBS_CHUNKS_PER_SECTOR                               (LFS_SECTOR_SIZE / RBS_CHUNK_SIZE)
#define RBS_SECTOR_MASK                                     (RBS_CHUNKS_PER_SECTOR - 1)

#define RBS_IO_SIZE                                         (RBS_CHUNK_SIZE * 1)
#define FAT_SECTOR_SIZE                                     512

#define RBS_MAX_DATA_SIZE                                   1024

#pragma pack(push, 2)
typedef struct {
    uint32_t id;
    uint16_t len;
    uint16_t crc;
    uint32_t signat_hi;
    uint32_t signat_lo;
} LFS_HEADER;
#pragma pack(pop)

typedef struct {
    uint32_t offset_c; // in chunks
    uint32_t len_c;
    uint32_t curr_pos;
    uint32_t curr_id;
} LFSS_FILE;

typedef struct {
    HAL hal;
    IO* io;
    bool active;
    uint32_t curr_file_offset_c;
    LFSS_FILE* curr_file;
    uint32_t files_cnt;
    LFSS_FILE* files;
} LFSS;

#endif // LFSS_H
