/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SCSIS_MMC_H
#define SCSIS_MMC_H

#include "scsis.h"

#define SCSIS_MMC_FEATURE_PROFILE_LIST                  0x0000
#define SCSIS_MMC_FEATURE_CORE                          0x0001

#define SCSIS_MMC_PROFILE_DVD_ROM                       0x0010

void scsis_mmc_read_toc(SCSIS* scsis, uint8_t* req);
void scsis_mmc_get_configuration(SCSIS* scsis, uint8_t* req);
void scsis_mmc_get_event_status_notification(SCSIS* scsis, uint8_t* req);

#endif // SCSIS_MMC_H
