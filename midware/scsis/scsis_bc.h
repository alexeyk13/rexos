/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SCSIS_BC_H
#define SCSIS_BC_H

/*
        SCSI block commands. Based on revision 3, release 25
 */

#include "scsis.h"

SCSIS_RESPONSE scsis_bc_read_capacity10(SCSIS* scsis, uint8_t* req, IO* io);


#endif // SCSIS_BC_H
