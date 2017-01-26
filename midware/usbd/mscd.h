/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MSCD_H
#define MSCD_H

#include "usbd.h"

#define MSC_BO_MASS_STORAGE_RESET                                                       0xff
#define MSC_BO_GET_MAX_LUN                                                              0xfe

#pragma pack(push, 1)

#define MSC_CBW_SIGNATURE                                                               0x43425355
#define MSC_CBW_FLAG_DATA_IN(flags)                                                     ((flags) & (1 << 7))

typedef struct {
    uint32_t    dCBWSignature;
    uint32_t    dCBWTag;
    uint32_t    dCBWDataTransferLength;
    uint8_t     bmCBWFlags;
    uint8_t     bCBWLUN;
    uint8_t     bCBWCBLength;
    uint8_t     CBWCB[16];
}CBW;

#define MSC_CSW_SIGNATURE                                                               0x53425355

#define MSC_CSW_COMMAND_PASSED                                                          0x0
#define MSC_CSW_COMMAND_FAILED                                                          0x1
#define MSC_CSW_PHASE_ERROR                                                             0x2

typedef struct {
    uint32_t    dCSWSignature;
    uint32_t    dCSWTag;
    uint32_t    dCSWDataResidue;
    uint8_t     bCSWStatus;
}CSW;

#pragma pack(pop)

extern const USBD_CLASS __MSCD_CLASS;

#endif // MSCD_H
