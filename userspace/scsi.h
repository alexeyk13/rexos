/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SCSI_H
#define SCSI_H

#include <stdint.h>

#define SCSI_PERIPHERAL_DEVICE_TYPE_DIRECT_ACCESS                       (0 << 0)
#define SCSI_PERIPHERAL_DEVICE_TYPE_SEQUENTAL_ACCESS                    (1 << 0)
#define SCSI_PERIPHERAL_DEVICE_TYPE_PRINTER                             (2 << 0)
#define SCSI_PERIPHERAL_DEVICE_TYPE_PROCESSOR                           (3 << 0)
#define SCSI_PERIPHERAL_DEVICE_TYPE_WRITE_ONCE                          (4 << 0)
#define SCSI_PERIPHERAL_DEVICE_TYPE_CD_DVD                              (5 << 0)
#define SCSI_PERIPHERAL_DEVICE_TYPE_OPTICAL_MEMORY                      (7 << 0)
#define SCSI_PERIPHERAL_DEVICE_TYPE_MEDIA_CHANGER                       (8 << 0)
#define SCSI_PERIPHERAL_DEVICE_TYPE_RAID                                (0x0c << 0)
#define SCSI_PERIPHERAL_DEVICE_TYPE_ENCLOSURE                           (0x0d << 0)
#define SCSI_PERIPHERAL_DEVICE_TYPE_RBC                                 (0x0e << 0)
#define SCSI_PERIPHERAL_DEVICE_TYPE_OCRW                                (0x0f << 0)
#define SCSI_PERIPHERAL_DEVICE_TYPE_OSD                                 (0x11 << 0)
#define SCSI_PERIPHERAL_DEVICE_TYPE_ADI                                 (0x12 << 0)
#define SCSI_PERIPHERAL_DEVICE_TYPE_CLAUSE9                             (0x13 << 0)
#define SCSI_PERIPHERAL_DEVICE_TYPE_ZBC                                 (0x14 << 0)
#define SCSI_PERIPHERAL_DEVICE_TYPE_WELL_KNOWN_LU                       (0x1e << 0)
#define SCSI_PERIPHERAL_DEVICE_TYPE_NO_DEVICE                           (0x1f << 0)

#pragma pack(push, 1)

typedef enum {
    SCSI_REQUEST_READ = 0,
    SCSI_REQUEST_WRITE,
    SCSI_REQUEST_VERIFY,
    SCSI_REQUEST_STORAGE_DESCRIPTOR,
    SCSI_REQUEST_MEDIA_DESCRIPTOR
} SCSI_REQUEST;

typedef struct {
    char* vendor;
    char* product;
    char* revision;
    uint16_t flags;
    uint8_t scsi_device_type;
    uint8_t align;
} SCSI_STORAGE_DESCRIPTOR;

typedef struct {
    uint32_t num_sectors;
    uint32_t num_sectors_hi;
    uint32_t sector_size;
    char* serial;
} SCSI_MEDIA_DESCRIPTOR;

#define SCSI_STORAGE_DESCRIPTOR_REMOVABLE                               (1 << 0)

typedef struct {
    SCSI_REQUEST request;
    unsigned int addr, addr_hi;
    //or error code on failure
    int size;
} SCSI_STACK;

#pragma pack(pop)

#endif // SCSI_H
