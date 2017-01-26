/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef FAT16_H
#define FAT16_H

#include <stdint.h>
#include "../../userspace/so.h"
#include "vfss.h"

#define FAT_SECTOR_SIZE                                     512

#define FAT_BPB_EXT_SIGNATURE                               0x29

#define FAT_FILE_ENTRY_EMPTY                                0x00
#define FAT_FILE_ENTRY_STUFF_MAGIC                          0x05
#define FAT_FILE_ENTRY_MAGIC_ERASED                         0xe5
#define FAT_FILE_ENTRY_DOT                                  '.'

#define FAT_FILE_ATTR_READ_ONLY                             0x01
#define FAT_FILE_ATTR_HIDDEN                                0x02
#define FAT_FILE_ATTR_SYSTEM                                0x04
#define FAT_FILE_ATTR_LABEL                                 0x08
#define FAT_FILE_ATTR_SUBFOLDER                             0x10
#define FAT_FILE_ATTR_ARCHIVE                               0x20
#define FAT_FILE_ATTR_DEVICE                                0x40
#define FAT_FILE_ATTR_RESERVED                              0x80
#define FAT_FILE_ATTR_LFN                                   0x0f
#define FAT_FILE_ATTR_REAL_MASK                             0xff
#define FAT_FILE_ATTR_DOT_OR_DOT_DOT                        0x100

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

#define FAT_FILE_SYS_ATTR_NAME_LOWER_CASE                   (1 << 3)
#define FAT_FILE_SYS_ATTR_EXT_LOWER_CASE                    (1 << 4)

#define FAT_LFN_SEQ_LAST                                    (1 << 6)
#define FAT_LFN_SEQ_MASK                                    0x1f
#define FAT_LFN_CHUNK_SIZE                                  13

#define FAT_CLUSTER_RESERVED                                0xfff8
#define FAT_CLUSTER_LAST                                    0xffff
#define FAT_CLUSTER_FREE                                    0x0000

#pragma pack(push, 1)
typedef struct {
    char name[8];
    char ext[3];
    uint8_t attr;
    uint8_t sys_attr;
    uint8_t crt_ztime;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t acc_date;
    uint16_t security;
    uint16_t mod_time;
    uint16_t mod_date;
    uint16_t first_cluster;
    uint32_t size;
} FAT_FILE_ENTRY;

typedef struct {
    uint8_t seq;
    uint16_t name1[5];
    uint8_t attr;
    uint8_t type;
    uint8_t checksum;
    uint16_t name2[6];
    uint16_t first_cluster;
    uint16_t name3[2];
} FAT_LFN_ENTRY;
#pragma pack(pop)

typedef struct {
    unsigned long sectors_count, cluster_sectors, root_count, root_sectors, reserved_sectors, fat_sectors, cluster_size, clusters_count, fat_count;
    SO finds;
    SO file_handles;
    bool active;
} FAT16_TYPE;

void fat16_init(VFSS_TYPE* vfss);
void fat16_request(VFSS_TYPE* vfss, IPC* ipc);

#endif // FAT16_H
