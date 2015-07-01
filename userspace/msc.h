/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MSC_H
#define MSC_H

#include "ipc.h"

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

typedef enum {
    //requests from host
    USB_MSC_GET_STORAGE_DESCRIPTOR = IPC_USER,
    USB_MSC_GET_MEDIA_DESCRIPTOR,
    USB_MSC_READ,
    USB_MSC_WRITE,
    USB_MSC_VERIFY,
    //storage informing
    USB_MSC_MEDIA_REMOVED
} USB_MSC_IPC;

#endif // MSC_H
