/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SCSIS_BC_H
#define SCSIS_BC_H

/*
        SCSI block commands. Based on revision 3, release 25
 */

#include "scsis.h"
#include <stdbool.h>

void scsis_bc_host_io_complete(SCSIS* scsis, int resp_size);
void scsis_bc_storage_io_complete(SCSIS* scsis, int resp_size);

void scsis_bc_read_capacity10(SCSIS* scsis, uint8_t* req);
void scsis_bc_read_capacity16(SCSIS* scsis, uint8_t* req);
void scsis_bc_start_stop_unit(SCSIS* scsis, uint8_t* req);
void scsis_bc_read6(SCSIS* scsis, uint8_t* req);
void scsis_bc_read10(SCSIS* scsis, uint8_t* req);
void scsis_bc_read12(SCSIS* scsis, uint8_t* req);
void scsis_bc_write6(SCSIS* scsis, uint8_t* req);
void scsis_bc_write10(SCSIS* scsis, uint8_t* req);
void scsis_bc_write12(SCSIS* scsis, uint8_t* req);
void scsis_bc_verify10(SCSIS* scsis, uint8_t* req);
void scsis_bc_verify12(SCSIS* scsis, uint8_t* req);
void scsis_bc_write_verify10(SCSIS* scsis, uint8_t* req);
void scsis_bc_write_verify12(SCSIS* scsis, uint8_t* req);
void scsis_bc_read16(SCSIS* scsis, uint8_t* req);
void scsis_bc_read32(SCSIS* scsis, uint8_t* req);
void scsis_bc_write16(SCSIS* scsis, uint8_t* req);
void scsis_bc_write32(SCSIS* scsis, uint8_t* req);
void scsis_bc_verify16(SCSIS* scsis, uint8_t* req);
void scsis_bc_verify32(SCSIS* scsis, uint8_t* req);
void scsis_bc_write_verify16(SCSIS* scsis, uint8_t* req);
void scsis_bc_write_verify32(SCSIS* scsis, uint8_t* req);

//for scsis_pc
void scsis_bc_mode_sense_fill_header(SCSIS* scsis, bool dbd);
void scsis_bc_mode_sense_fill_header_long(SCSIS* scsis, bool dbd, bool long_lba);
bool scsis_bc_mode_sense_add_page(SCSIS* scsis, unsigned int psp);

#endif // SCSIS_BC_H
