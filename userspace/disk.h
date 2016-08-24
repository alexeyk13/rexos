/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
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
    uint16_t sector_size;
    uint8_t cluster_size;
    uint16_t reserved;
    uint8_t fat_count;
    uint16_t root_count;
    uint16_t sectors_short;
    uint8_t mediat_descriptor;
    uint16_t fat_sectors;
} FAT_BOOT_SECTOR_BPB_TYPE;
#pragma pack(pop)

#endif // DISK_H
