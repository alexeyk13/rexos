/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SCSIS_PC_H
#define SCSIS_PC_H

/*
        SCSI primary commands. Based on revision 4, release 37a
 */

#include "scsis.h"

SCSIS_RESPONSE scsis_pc_inquiry(SCSIS* scsis, uint8_t* req, IO* io);

#endif // SCSIS_PC_H
