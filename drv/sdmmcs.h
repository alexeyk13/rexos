/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SDMMCS_H
#define SDMMCS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SDMMC_NO_CARD,
    SDMMC_CARD_MMC,
    SDMMC_CARD_SDIO,
    SDMMC_CARD_SD_V1,
    SDMMC_CARD_SD_V2,
    SDMMC_CARD_HD,
    SDMMC_CARD_XD
} SDMMC_CARD_TYPE;

typedef enum {
    SDMMC_NO_RESPONSE,
    SDMMC_RESPONSE_R1,
    SDMMC_RESPONSE_R1B,
    SDMMC_RESPONSE_R2,
    SDMMC_RESPONSE_R3,
    SDMMC_RESPONSE_R4,
    SDMMC_RESPONSE_R5,
    SDMMC_RESPONSE_R6,
    SDMMC_RESPONSE_R7
} SDMMC_RESPONSE_TYPE;

typedef enum {
    SDMMC_ERROR_OK = 0,
    SDMMC_ERROR_TIMEOUT,
    SDMMC_ERROR_CRC_FAIL,
    SDMMC_ERROR_BUSY,
    SDMMC_ERROR_HARDWARE_FAILURE
} SDMMC_ERROR;

typedef struct {
    SDMMC_CARD_TYPE card_type;
    SDMMC_ERROR last_error;
    //TODO: more data
} SDMMCS;

//prototypes, that must be defined by driver
extern void sdmmcs_set_clock(void* param, unsigned int speed);
extern void sdmmcs_set_bus_width(void* param, int width);
extern SDMMC_ERROR sdmmcs_send_cmd(void* param, uint8_t cmd, uint32_t arg, void* resp, SDMMC_RESPONSE_TYPE resp_type);

void sdmmcs_init(SDMMCS* sdmmcs);
bool sdmmcs_open(SDMMCS* sdmmcs, void* param);

#endif // SDMMCS_H
