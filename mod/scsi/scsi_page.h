/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SCSI_PAGE_H
#define SCSI_PAGE_H

#include "scsi.h"

void scsi_fill_error_page(SCSI* scsi, uint8_t code, uint16_t asq);
void scsi_fill_capacity_page(SCSI* scsi);
void scsi_fill_format_capacity_page(SCSI* scsi);
void scsi_fill_standart_inquiry_page(SCSI* scsi);

void scsi_fill_evpd_page_00(SCSI* scsi);
void scsi_fill_evpd_page_80(SCSI* scsi);
void scsi_fill_evpd_page_83(SCSI* scsi);

void scsi_fill_sense_page_1c(SCSI* scsi);
void scsi_fill_sense_page_3f(SCSI* scsi);

#endif // SCSI_PAGE_H
