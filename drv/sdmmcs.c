/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "sdmmcs.h"
#include "../userspace/stdio.h"

void sdmmcs_init(SDMMCS* sdmmcs)
{
    sdmmcs->card_type = SDMMC_NO_CARD;
    sdmmcs->last_error = SDMMC_ERROR_OK;
}

bool sdmmcs_open(SDMMCS* sdmmcs, void* param)
{
    //TODO: check
    sdmmcs_set_clock(param, 400000);
    sdmmcs_set_bus_width(param, 1);


    uint32_t resp;
    unsigned int d;
    d = sdmmcs_send_cmd(param, 0x00, 0, &resp, SDMMC_NO_RESPONSE);
    printd("d0: %#X\n", d);
    d = sdmmcs_send_cmd(param, 0x08, 0x000001AA, &resp, SDMMC_RESPONSE_R7);
    printd("d1: %#X\n", d);
    printd("resp: %#X\n", resp);
    return true;
}
