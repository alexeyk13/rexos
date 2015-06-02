#ifndef MSC_H
#define MSC_H

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

#endif // MSC_H
