/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "sdmmcs.h"
#include "../userspace/stdio.h"
#include "../userspace/types.h"
#include "../userspace/process.h"
#include "../userspace/error.h"
#include "../userspace/core/core.h"
#include "../kernel/kernel.h"
#include "sys_config.h"
#include <string.h>

#define ARG_RCA(sdmmcs)                         ((uint32_t)((sdmmcs)->rca) << 16)

void sdmmcs_init(SDMMCS* sdmmcs, void *param)
{
    sdmmcs->card_type = SDMMC_NO_CARD;
    sdmmcs->param = param;
}

static bool sdmmcs_cmd(SDMMCS* sdmmcs, uint8_t cmd, uint32_t arg, void* resp, SDMMC_RESPONSE_TYPE resp_type)
{
    unsigned int retry;
    for (retry = 0; retry < 3; ++retry)
    {
        if ((sdmmcs->last_error = sdmmcs_send_cmd(sdmmcs->param, cmd, arg, resp, resp_type)) != SDMMC_ERROR_CRC_FAIL)
            //busy for R1b
            return (sdmmcs->last_error == SDMMC_ERROR_OK) || (sdmmcs->last_error == SDMMC_ERROR_BUSY);
    }
    return false;
}

static bool sdmmcs_cmd_r1x(SDMMCS* sdmmcs, uint8_t cmd, uint32_t arg, SDMMC_RESPONSE_TYPE resp_type)
{
    unsigned int retry;
    for (retry = 0; retry < 3; ++retry)
    {
        if (!sdmmcs_cmd(sdmmcs, cmd, arg, &sdmmcs->r1, resp_type))
            return false;
        if (sdmmcs->r1 & (SDMMC_R1_FATAL_ERROR | SDMMC_R1_APP_ERROR))
            return false;
        if ((sdmmcs->r1 & SDMMC_R1_COM_CRC_ERROR) == 0)
            return true;
    }
    return false;
}

static inline bool sdmmcs_cmd_r1(SDMMCS* sdmmcs, uint8_t cmd, uint32_t arg)
{
    return sdmmcs_cmd_r1x(sdmmcs, cmd, arg, SDMMC_RESPONSE_R1);
}

static inline bool sdmmcs_cmd_r1b(SDMMCS* sdmmcs, uint8_t cmd, uint32_t arg)
{
    return sdmmcs_cmd_r1x(sdmmcs, cmd, arg, SDMMC_RESPONSE_R1B);
}

static bool sdmmcs_cmd_r6(SDMMCS* sdmmcs, uint8_t cmd, uint32_t arg)
{
    unsigned int retry;
    unsigned int r6;
    for (retry = 0; retry < 3; ++retry)
    {
        if (!sdmmcs_cmd(sdmmcs, cmd, arg, &r6, SDMMC_RESPONSE_R6))
            return false;
        //r6 status -> r1 status
        sdmmcs->r1 = r6 & 0x1fff;
        if (r6 & (1<< 13))
            sdmmcs->r1 |= 1 << 19;
        if (r6 & (1 << 14))
            sdmmcs->r1 |= 1 << 22;
        if (r6 & (1 << 15))
            sdmmcs->r1 |= 1 << 23;

        if (sdmmcs->r1 & (SDMMC_R1_FATAL_ERROR | SDMMC_R1_APP_ERROR))
            return false;
        if ((sdmmcs->r1 & SDMMC_R1_COM_CRC_ERROR) == 0)
        {
            sdmmcs->rca = r6 >> 16;
            return true;
        }
    }
    return false;
}

static bool sdmmcs_acmd(SDMMCS* sdmmcs, uint8_t acmd, uint32_t arg, void* resp, SDMMC_RESPONSE_TYPE resp_type)
{
    unsigned int retry;
    for (retry = 0; retry < 3; ++retry)
    {
        if (!sdmmcs_cmd_r1(sdmmcs, SDMMC_CMD_APP_CMD, ARG_RCA(sdmmcs)))
            return false;
        if (sdmmcs->r1 & SDMMC_R1_APP_CMD)
            return sdmmcs_cmd(sdmmcs, acmd, arg, resp, resp_type);
    }
    return false;
}

static bool sdmmcs_acmd_r1(SDMMCS* sdmmcs, uint8_t acmd, uint32_t arg)
{
    if (!sdmmcs_acmd(sdmmcs, acmd, arg, &sdmmcs->r1, SDMMC_RESPONSE_R1))
        return false;
    if (sdmmcs->r1 & (SDMMC_R1_FATAL_ERROR | SDMMC_R1_APP_ERROR))
        return false;
    if ((sdmmcs->r1 & SDMMC_R1_COM_CRC_ERROR) == 0)
        return true;
    return false;
}

bool sdmmcs_reset(SDMMCS* sdmmcs)
{
    if (!sdmmcs_cmd(sdmmcs, SDMMC_CMD_GO_IDLE_STATE, 0, NULL, SDMMC_NO_RESPONSE))
    {
#if (SDMMC_DEBUG)
        printd("SDMMC: Hardware failure\n");
#endif //SDMMC_DEBUG
        error(ERROR_HARDWARE);
        return false;
    }
    return true;
}

static inline bool sdmmcs_card_init(SDMMCS* sdmmcs)
{
    unsigned int i, query, resp;
    bool v2 = false;
    if (sdmmcs_cmd(sdmmcs, SDMMC_CMD_SEND_IF_COND, IF_COND_VOLTAGE_3_3V | IF_COND_CHECK_PATTERN, &resp, SDMMC_RESPONSE_R7))
    {
        if ((resp & IF_COND_VOLTAGE_3_3V) == 0)
        {
#if (SDMMC_DEBUG)
            printd("SDMMC: Unsupported voltage\n");
#endif //SDMMC_DEBUG
            error(ERROR_HARDWARE);
            return false;
        }
        if ((resp & IF_COND_CHECK_PATTERN_MASK) != IF_COND_CHECK_PATTERN)
        {
#if (SDMMC_DEBUG)
            printd("SDMMC: Pattern failure. Unsupported or broken card\n");
#endif //SDMMC_DEBUG
            error(ERROR_HARDWARE);
            return false;
        }
        v2 = true;
    }

    query = OP_COND_VOLTAGE_WINDOW;
    if (v2)
        query |= OP_COND_HCS | OP_COND_XPC;
    for (i = 0; i < 1000; ++i)
    {
        if (!sdmmcs_acmd(sdmmcs, SDMMC_ACMD_SD_SEND_OP_COND, query, &resp, SDMMC_RESPONSE_R3))
        {
#if (SDMMC_DEBUG)
            printd("SDMMC: Not SD card or no card\n");
#endif //SDMMC_DEBUG
            error(ERROR_NOT_FOUND);
            return false;
        }
        if (resp & OP_COND_BUSY)
        {
            if (resp & OP_COND_HCS)
            {
                sdmmcs->card_type = SDMMC_CARD_HD;
#if (SDMMC_DEBUG)
                printd("SDMMC: Found HC/XC card\n");
#endif //SDMMC_DEBUG
            }
            else if (v2)
            {
                sdmmcs->card_type = SDMMC_CARD_SD_V2;
#if (SDMMC_DEBUG)
                printd("SDMMC: Found SD v.2 card\n");
#endif //SDMMC_DEBUG
            }
            else
            {
                sdmmcs->card_type = SDMMC_CARD_SD_V1;
#if (SDMMC_DEBUG)
                printd("SDMMC: Found SD v.1 card\n");
#endif //SDMMC_DEBUG
            }
            return true;
        }
#ifdef EXODRIVERS
        exodriver_delay_us(1 * 1000);
#else
        sleep_ms(1);
#endif //EXODRIVERS
    }
#if (SDMMC_DEBUG)
    printd("SDMMC: Card init timeout\n");
#endif //SDMMC_DEBUG
    error(ERROR_TIMEOUT);
    return false;
}

static inline bool sdmmcs_card_address(SDMMCS* sdmmcs)
{
    CID cid;
    if (!sdmmcs_cmd(sdmmcs, SDMMC_CMD_ALL_SEND_CID, 0, &cid, SDMMC_RESPONSE_R2))
    {
        error(ERROR_HARDWARE);
        return false;
    }

#if (SDMMC_DEBUG)
    printd("SDMMC CID:\n");
    printd("MID: %#02x\n", cid.mid);
    printd("OID: %c%c\n", cid.oid[1],  cid.oid[0]);
    printd("PNM: %c%c%c%c%c\n", cid.pnm[4], cid.pnm[3], cid.pnm[2], cid.pnm[1], cid.pnm[0]);
    printd("PRV: %x.%x\n", cid.prv >> 4, cid.prv & 0xf);
    printd("PSN: %08x\n", cid.psn);
    printd("MDT: %d,%d\n", cid.mdt & 0xf, ((cid.mdt >> 4) & 0xff) + 2000);
#endif //SDMMC_DEBUG

    sdmmcs->serial = cid.psn;
    if (!sdmmcs_cmd_r6(sdmmcs, SDMMC_CMD_SEND_RELATIVE_ADDR, 0))
    {
        error(ERROR_HARDWARE);
        return false;
    }
#if (SDMMC_DEBUG)
    printd("SDMMC RCA: %04X\n", sdmmcs->rca);
#endif //SDMMC_DEBUG

    return true;
}

static inline bool sdmmcs_card_read_csd(SDMMCS* sdmmcs)
{
    uint8_t csd[16];
    uint32_t c_size, mult;
#if (SDMMC_DEBUG)
    unsigned int capacity;
    char c;
#endif //SDMMC_DEBUG
    if (!sdmmcs_cmd(sdmmcs, SDMMC_CMD_SEND_CSD, ARG_RCA(sdmmcs), csd, SDMMC_RESPONSE_R2))
    {
        error(ERROR_HARDWARE);
        return false;
    }
    if ((csd[15] >> 6) == 0x01)
    {
#if (SDMMC_DEBUG)
        printd("SDMMC: CSD v2\n");
#endif //SDMMC_DEBUG
        sdmmcs->sector_size = 512;
        c_size = ((((uint32_t)csd[8]) & 0x3f) << 16) | (((uint32_t)csd[7]) << 8) | (((uint32_t)csd[6]) << 0);
        sdmmcs->num_sectors = (c_size + 1) * 1024;
    }
    else
    {
#if (SDMMC_DEBUG)
        printd("SDMMC: CSD v1\n");
#endif //SDMMC_DEBUG
        c_size = ((((uint32_t)csd[9]) & 0x03) << 10) | (((uint32_t)csd[8]) << 2) | (((uint32_t)csd[7]) >> 6);
        mult = (((uint32_t)csd[5]) >> 7) | ((((uint32_t)csd[6]) & 0x03) << 1);
        sdmmcs->num_sectors = (c_size + 1) * (1 << (mult + 2));
        sdmmcs->sector_size = 1 << (((uint32_t)csd[10]) & 0x0f);
    }
    switch(csd[12])
    {
    case 0x32:
        sdmmcs->max_clock = 25000000;
        break;
    case 0x5a:
        sdmmcs->max_clock = 50000000;
        break;
    case 0x0b:
        sdmmcs->max_clock = 100000000;
        break;
    case 0x2b:
        sdmmcs->max_clock = 200000000;
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        return false;
    }
    if (csd[1] & 0x30)
        sdmmcs->write_protected = true;
#if (SDMMC_DEBUG)
    capacity = (((sdmmcs->num_sectors / 1024) * sdmmcs->sector_size) * 10) / 1024;
    if (capacity < 10240)
        c = 'M';
    else
    {
        capacity /= 1024;
        if (capacity < 10240)
            c = 'G';
        else
        {
            capacity /= 1024;
            c = 'T';
        }
    }
    printd("Capacity: %d.%d%cB\n", capacity / 10, capacity % 10, c);
    printd("Max clock: %dMHz\n", sdmmcs->max_clock / 1000000);
    if (sdmmcs->write_protected)
        printd("Data is write protected\n");
#endif //SDMMC_DEBUG
    return true;
}

static inline bool sdmmcs_card_select(SDMMCS* sdmmcs)
{
    if (!sdmmcs_cmd_r1b(sdmmcs, SDMMC_CMD_SELECT_DESELECT_CARD, ARG_RCA(sdmmcs)))
    {
#if (SDMMC_DEBUG)
        printd("SDMMC: card selection failure\n");
#endif //SDMMC_DEBUG
        error(ERROR_HARDWARE);
        return false;
    }

    sdmmcs_set_clock(sdmmcs->param, sdmmcs->max_clock);

    //disable pullap on DAT3
    if (!sdmmcs_acmd_r1(sdmmcs, SDMMC_ACMD_SET_CLR_CARD_DETECT, 0x00))
        return false;
    //set 4 bit bus
    if (sdmmcs_acmd_r1(sdmmcs, SDMMC_ACMD_SET_BUS_WIDTH, 0x2))
        sdmmcs_set_bus_width(sdmmcs->param, 4);

    return true;
}

static void sdmmcs_set_r1_error(SDMMCS* sdmmcs)
{
    if (sdmmcs->r1 & SDMMC_R1_CARD_ECC_FAILED)
    {
        error(ERROR_CRC);
        return;
    }
    if (sdmmcs->r1 & SDMMC_R1_FATAL_ERROR)
    {
        error(ERROR_HARDWARE);
        return;
    }
    if (sdmmcs->r1 & SDMMC_R1_APP_ERROR)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    error(ERROR_TIMEOUT);
}

static bool sdmmcs_wait_for_ready(SDMMCS* sdmmcs)
{
    int retries;
    if (!sdmmcs->writed_before)
        return true;
    //recovery, based on TOSHIBA recomendations
    for (retries = 1000 * 10; retries; --retries)
    {
        if (sdmmcs_cmd_r1(sdmmcs, SDMMC_CMD_SEND_STATUS, ARG_RCA(sdmmcs)))
        {
            if ((sdmmcs->r1 & SDMMC_R1_CURRENT_STATE_MSK) == SDMMC_R1_STATE_TRAN)
                return true;
        }
#ifdef EXODRIVERS
        exodriver_delay_us(100);
#else
        sleep_us(100);
#endif //EXODRIVERS
    }
    error(ERROR_TIMEOUT);
    return false;
}

bool sdmmcs_open(SDMMCS* sdmmcs)
{
    sdmmcs->card_type = SDMMC_NO_CARD;
    sdmmcs->last_error = SDMMC_ERROR_OK;
    sdmmcs->serial = 0x00000000;
    sdmmcs->rca = 0x0000;
    sdmmcs->num_sectors = sdmmcs->sector_size = sdmmcs->max_clock = 0;
    sdmmcs->write_protected = false;
    sdmmcs->writed_before = false;

    sdmmcs_set_clock(sdmmcs->param, 400000);
    sdmmcs_set_bus_width(sdmmcs->param, 1);

    if (!sdmmcs_reset(sdmmcs))
        return false;

    if (!sdmmcs_card_init(sdmmcs))
        return false;

    if (!sdmmcs_card_address(sdmmcs))
        return false;

    if (!sdmmcs_card_read_csd(sdmmcs))
        return false;

    if (!sdmmcs_card_select(sdmmcs))
        return false;

    return true;
}

bool sdmmcs_read(SDMMCS* sdmmcs, unsigned int block, unsigned int count)
{
    uint32_t addr = block;
    if (sdmmcs->card_type == SDMMC_CARD_SD_V1 || sdmmcs->card_type == SDMMC_CARD_SD_V2)
        addr *= sdmmcs->sector_size;
    if (!sdmmcs_wait_for_ready(sdmmcs))
        return false;
    if (!sdmmcs_cmd_r1(sdmmcs, count == 1 ? SDMMC_CMD_READ_SINGLE_BLOCK : SDMMC_CMD_READ_MULTIPLE_BLOCK, addr))
    {
        sdmmcs_set_r1_error(sdmmcs);
        return false;
    }
    sdmmcs->writed_before = false;
    return true;
}

bool sdmmcs_write(SDMMCS* sdmmcs, unsigned int block, unsigned int count)
{
    uint32_t addr = block;
    if (sdmmcs->card_type == SDMMC_CARD_SD_V1 || sdmmcs->card_type == SDMMC_CARD_SD_V2)
        addr *= sdmmcs->sector_size;
    if (!sdmmcs_wait_for_ready(sdmmcs))
        return false;
    if (count > 1)
    {
        if (!sdmmcs_acmd_r1(sdmmcs, SDMMC_ACMD_SET_WR_BLK_ERASE_COUNT, count))
        {
            sdmmcs_set_r1_error(sdmmcs);
            return false;
        }
    }
    if (!sdmmcs_cmd_r1(sdmmcs, count == 1 ? SDMMC_CMD_WRITE_SINGLE_BLOCK : SDMMC_CMD_WRITE_MULTIPLE_BLOCK, addr))
    {
        sdmmcs_set_r1_error(sdmmcs);
        return false;
    }
    sdmmcs->writed_before = true;
    return true;
}

bool sdmmcs_erase(SDMMCS* sdmmcs, unsigned int block, unsigned int count)
{
    uint32_t start, end;
    start = block;
    if (sdmmcs->card_type == SDMMC_CARD_SD_V1 || sdmmcs->card_type == SDMMC_CARD_SD_V2)
    {
        start *= sdmmcs->sector_size;
        end = start + count * sdmmcs->sector_size - 1;
    }
    else
        end = block + count - 1;
    if (!sdmmcs_wait_for_ready(sdmmcs))
        return false;
    if (sdmmcs_cmd_r1(sdmmcs, SDMMC_CMD_ERASE_WR_BLK_START_ADDR, start) &&
        sdmmcs_cmd_r1(sdmmcs, SDMMC_CMD_ERASE_WR_BLK_END_ADDR, end) &&
        sdmmcs_cmd_r1(sdmmcs, SDMMC_CMD_ERASE, 0))
    {
        sdmmcs->writed_before = true;
        return true;
    }
    sdmmcs_set_r1_error(sdmmcs);
    return false;
}

void sdmmcs_stop(SDMMCS* sdmmcs)
{
    sdmmcs_cmd_r1b(sdmmcs, SDMMC_CMD_STOP_TRANSMISSION, 0x00);
}
