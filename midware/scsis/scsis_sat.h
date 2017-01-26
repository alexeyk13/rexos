/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SCSIS_SAT_H
#define SCSIS_SAT_H

/*
        SCSI ATA translation standart. Based on revision 3, release 07
 */

#include "scsis.h"

void scsis_sat_ata_pass_through12(SCSIS* scsis, uint8_t* req);
void scsis_sat_ata_pass_through16(SCSIS* scsis, uint8_t* req);

#endif // SCSIS_SAT_H
