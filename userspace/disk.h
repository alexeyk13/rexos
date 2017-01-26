/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef DISK_H
#define DISK_H

#include <stdint.h>

//also FAT boot sector
#define MBR_MAGIC                                           0xaa55
#define MBR_MAGIC_OFFSET                                    0x1fe

#define MBR_PARTITION_OFFSET                                0x1be
#define MBR_PARTITIONS_COUNT                                4

#pragma pack(push, 1)
typedef struct {
    uint8_t active;
    uint8_t start_head;
    uint8_t start_sector;
    uint8_t start_cylinder;
    uint8_t type;
    uint8_t end_head;
    uint8_t end_sector;
    uint8_t end_cylinder;
    uint32_t first_sector;
    uint32_t sectors_count;
} PARTITION_TYPE;

typedef struct {
    uint8_t jmp[3];
    char oem_name[8];
} FAT_BOOT_SECTOR_HEADER_TYPE;

typedef struct {
    //DOS 2.0 BPB
    uint16_t sector_size;
    uint8_t cluster_sectors;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_count;
    uint16_t sectors_short;
    uint8_t media_type;
    uint16_t fat_sectors;
    //DOS 3.31 BPB
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden;
    uint32_t sectors;
    //EPBP
    uint8_t drive_num;
    uint8_t reserved;
    uint8_t ext_signature;
    uint32_t serial;
    char label[11];
    char fs_type[8];
} FAT_BOOT_SECTOR_BPB_TYPE;
#pragma pack(pop)

#endif // DISK_H
