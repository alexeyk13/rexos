/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SCSI_IO_H
#define SCSI_IO_H

#include "types.h"
#include "scsi.h"

bool scsi_read(SCSI* scsi, unsigned long address, unsigned long sectors_count);
bool scsi_write(SCSI* scsi, unsigned long address, unsigned long sectors_count);
bool scsi_verify(SCSI* scsi, unsigned long address, unsigned long sectors_count);


#endif // SCSI_IO_H
