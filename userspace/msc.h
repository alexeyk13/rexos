/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MSC_H
#define MSC_H

#include "ipc.h"
#include "scsi.h"

#define MSC_INTERFACE_CLASS                                                             0x08

#define MSC_SUBCLASS_RBC                                                                0x01
#define MSC_SUBCLASS_CD_MMC                                                             0x02
#define MSC_SUBCLASS_TAPE                                                               0x03
#define MSC_SUBCLASS_UFI                                                                0x04
#define MSC_SUBCLASS_SFF                                                                0x05
#define MSC_SUBCLASS_SCSI                                                               0x06

#define MSC_PROTOCOL_CBI_INTERRUPT                                                      0x00
#define MSC_PROTOCOL_CBI_NO_INTERRUPT                                                   0x01
#define MSC_PROTOCOL_BULK_ONLY                                                          0x50

#define MSC_LUN_COUNT(ptr)                                                              (*(uint32_t*)(ptr))
#define MSC_LUN_CONFIGURATION(ptr, idx)                                                 ((SCSI_STORAGE_DESCRIPTOR*)((uint8_t*)(ptr) + \
                                                                                        sizeof(SCSI_STORAGE_DESCRIPTOR) * (idx) + sizeof(uint32_t)))

#endif // MSC_H
