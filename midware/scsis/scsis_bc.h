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
#include <stdbool.h>

SCSIS_RESPONSE scsis_bc_read_capacity10(SCSIS* scsis, uint8_t* req, IO* io);

//for scsis_pc
void scsis_bc_mode_sense_fill_header(SCSIS* scsis, IO* io, bool dbd);
void scsis_bc_mode_sense_fill_header_long(SCSIS* scsis, IO* io, bool dbd, bool long_lba);
SCSIS_RESPONSE scsis_bc_mode_sense_add_page(SCSIS* scsis, IO* io, unsigned int psp);

#endif // SCSIS_BC_H
