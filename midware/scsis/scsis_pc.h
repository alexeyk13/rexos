/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SCSIS_PC_H
#define SCSIS_PC_H

/*
        SCSI primary commands. Based on revision 4, release 37a
 */

#include "scsis.h"

void scsis_pc_inquiry(SCSIS* scsis, uint8_t* req);
void scsis_pc_test_unit_ready(SCSIS* scsis, uint8_t* req);
void scsis_pc_mode_sense6(SCSIS* scsis, uint8_t* req);
void scsis_pc_mode_sense10(SCSIS* scsis, uint8_t* req);
void scsis_pc_mode_select6(SCSIS* scsis, uint8_t* req);
void scsis_pc_mode_select10(SCSIS* scsis, uint8_t* req);
void scsis_pc_request_sense(SCSIS* scsis, uint8_t* req);
void scsis_pc_prevent_allow_medium_removal(SCSIS* scsis, uint8_t* req);

#endif // SCSIS_PC_H
