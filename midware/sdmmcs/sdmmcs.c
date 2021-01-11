/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#include "sdmmcs.h"
#include "../../userspace/stdio.h"
#include "../../userspace/types.h"
#include "../../userspace/process.h"
#include "../../userspace/sdmmc.h"
#include "../../userspace/sys.h"
#include "error.h"
//#include "../kernel.h"
#include "sys_config.h"
#include <string.h>

#define ARG_RCA(sdmmcs)                         ((uint32_t)((sdmmcs)->rca) << 16)


static inline uint32_t sdmmcs_set_clock(SDMMCS* sdmmcs, uint32_t clock)
{
    return get_size_exo(HAL_REQ(HAL_SDMMC, SDMMC_SET_CLOCK), sdmmcs->user, 0, clock);
}
static inline uint32_t sdmmcs_set_bus_width(SDMMCS* sdmmcs, uint32_t width)
{
    return get_size_exo(HAL_REQ(HAL_SDMMC, SDMMC_SET_DATA_WIDTH), sdmmcs->user, 0, width);
}

static inline SDMMC_ERROR sdmmcs_send_cmd(SDMMCS* sdmmcs, uint8_t cmd, uint32_t arg, SDMMC_RESPONSE_TYPE resp_type)
{
    sdmmcs->cmd_stack->cmd = cmd;
    sdmmcs->cmd_stack->arg = arg;
    sdmmcs->cmd_stack->resp_type = resp_type;
    return get_size_exo(HAL_IO_REQ(HAL_SDMMC, SDMMC_CMD), sdmmcs->user, (uint32_t)sdmmcs->cmd_io, 0);
}


void sdmmcs_init(SDMMCS* sdmmcs)
{
    sdmmcs->card_type = SDMMC_NO_CARD;
    sdmmcs->cmd_io = io_create(64 + sizeof(SDMMC_CMD_STACK));
    sdmmcs->cmd_stack = io_push(sdmmcs->cmd_io, sizeof(SDMMC_CMD_STACK));
}

static bool sdmmcs_cmd(SDMMCS* sdmmcs, uint8_t cmd, uint32_t arg, SDMMC_RESPONSE_TYPE resp_type)
{
    unsigned int retry;
    for (retry = 0; retry < 3; ++retry)
    {
        if ((sdmmcs->last_error = sdmmcs_send_cmd(sdmmcs, cmd, arg, resp_type)) != SDMMC_ERROR_CRC_FAIL)
            //busy for R1b
            return (sdmmcs->last_error == SDMMC_ERROR_OK) || (sdmmcs->last_error == SDMMC_ERROR_BUSY);
    }
    return false;
}

static bool sdmmcs_cmd_r1x(SDMMCS* sdmmcs, uint8_t cmd, uint32_t arg, SDMMC_RESPONSE_TYPE resp_type)
{
    unsigned int retry;
    uint32_t r1;
    for (retry = 0; retry < 3; ++retry)
    {
        if (!sdmmcs_cmd(sdmmcs, cmd, arg, resp_type))
            return false;
        r1 = sdmmcs->cmd_stack->resp[0];
        if (r1 & (SDMMC_R1_FATAL_ERROR | SDMMC_R1_APP_ERROR))
            return false;
        if ((r1 & SDMMC_R1_COM_CRC_ERROR) == 0)
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
    uint32_t* pr1;
    for (retry = 0; retry < 3; ++retry)
    {
        if (!sdmmcs_cmd(sdmmcs, cmd, arg, SDMMC_RESPONSE_R6))
            return false;
        //r6 status -> r1 status
        pr1 = sdmmcs->cmd_stack->resp;
        r6 = *pr1;
        *pr1 &= 0x1fff;
        if (r6 & (1<< 13))
            *pr1 |= 1 << 19;
        if (r6 & (1 << 14))
            *pr1 |= 1 << 22;
        if (r6 & (1 << 15))
            *pr1 |= 1 << 23;

        if (*pr1 & (SDMMC_R1_FATAL_ERROR | SDMMC_R1_APP_ERROR))
            return false;
        if ((*pr1 & SDMMC_R1_COM_CRC_ERROR) == 0)
        {
            sdmmcs->rca = r6 >> 16;
            return true;
        }
    }
    return false;
}

static bool sdmmcs_acmd(SDMMCS* sdmmcs, uint8_t acmd, uint32_t arg, SDMMC_RESPONSE_TYPE resp_type)
{
    unsigned int retry;
    for (retry = 0; retry < 3; ++retry)
    {
        if (!sdmmcs_cmd_r1(sdmmcs, SDMMC_CMD_APP_CMD, ARG_RCA(sdmmcs)))
            return false;
        if (sdmmcs->cmd_stack->resp[0] & SDMMC_R1_APP_CMD)
            return sdmmcs_cmd(sdmmcs, acmd, arg, resp_type);
    }
    return false;
}

static bool sdmmcs_acmd_r1(SDMMCS* sdmmcs, uint8_t acmd, uint32_t arg)
{
    uint32_t r1;
    if (!sdmmcs_acmd(sdmmcs, acmd, arg, SDMMC_RESPONSE_R1))
        return false;
    r1 = sdmmcs->cmd_stack->resp[0];
    if (r1 & (SDMMC_R1_FATAL_ERROR | SDMMC_R1_APP_ERROR))
        return false;
    if ((r1 & SDMMC_R1_COM_CRC_ERROR) == 0)
        return true;
    return false;
}

static inline bool sdmmcs_reset(SDMMCS* sdmmcs)
{
    if (!sdmmcs_cmd(sdmmcs, SDMMC_CMD_GO_IDLE_STATE, 0, SDMMC_NO_RESPONSE))
    {
#if (SDMMC_DEBUG)
        printf("SDMMC: Hardware failure\n");
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
    if (sdmmcs_cmd(sdmmcs, SDMMC_CMD_SEND_IF_COND, IF_COND_VOLTAGE_3_3V | IF_COND_CHECK_PATTERN, SDMMC_RESPONSE_R7))
    {
        resp = sdmmcs->cmd_stack->resp[0];
        if ((resp & IF_COND_VOLTAGE_3_3V) == 0)
        {
#if (SDMMC_DEBUG)
            printf("SDMMC: Unsupported voltage\n");
#endif //SDMMC_DEBUG
            error(ERROR_HARDWARE);
            return false;
        }
        if ((resp & IF_COND_CHECK_PATTERN_MASK) != IF_COND_CHECK_PATTERN)
        {
#if (SDMMC_DEBUG)
            printf("SDMMC: Pattern failure. Unsupported or broken card\n");
#endif //SDMMC_DEBUG
            error(ERROR_HARDWARE);
            return false;
        }
        v2 = true;
    }

    query = OP_COND_VOLTAGE_WINDOW;
    if (v2)
        query |= OP_COND_HCS | OP_COND_XPC | OP_COND_S18;
    for (i = 0; i < 1000; ++i)
    {
        if (!sdmmcs_acmd(sdmmcs, SDMMC_ACMD_SD_SEND_OP_COND, query, SDMMC_RESPONSE_R3))
        {
#if (SDMMC_DEBUG)
            printf("SDMMC: Not SD card or no card\n");
#endif //SDMMC_DEBUG
            error(ERROR_NOT_FOUND);
            return false;
        }
        resp = sdmmcs->cmd_stack->resp[0];
        if (resp & OP_COND_BUSY)
        {
            if (resp & OP_COND_HCS)
            {
                sdmmcs->card_type = SDMMC_CARD_HD;
#if (SDMMC_DEBUG)
                if (resp & OP_COND_S18)
                    printf("SDMMC: Found HC/XC card with UHS-I\n");
                else
                    printf("SDMMC: Found HC/XC card, no UHS\n");

#endif //SDMMC_DEBUG
            }
            else if (v2)
            {
                sdmmcs->card_type = SDMMC_CARD_SD_V2;
#if (SDMMC_DEBUG)
                printf("SDMMC: Found SD v.2 card\n");
#endif //SDMMC_DEBUG
            }
            else
            {
                sdmmcs->card_type = SDMMC_CARD_SD_V1;
#if (SDMMC_DEBUG)
                printf("SDMMC: Found SD v.1 card\n");
#endif //SDMMC_DEBUG
            }
            return true;
        }
        sleep_ms(1);
    }
#if (SDMMC_DEBUG)
    printf("SDMMC: Card init timeout\n");
#endif //SDMMC_DEBUG
    error(ERROR_TIMEOUT);
    return false;
}

static inline bool sdmmcs_card_address(SDMMCS* sdmmcs)
{
    CID* cid = (CID*)sdmmcs->cmd_stack->resp ;
    if (!sdmmcs_cmd(sdmmcs, SDMMC_CMD_ALL_SEND_CID, 0, SDMMC_RESPONSE_R2))
    {
        error(ERROR_HARDWARE);
        return false;
    }

#if (SDMMC_DEBUG)
    printf("SDMMC CID:\n");
    printf("MID: %#02x\n", cid->mid);
    printf("OID: %c%c\n", cid->oid[1],  cid->oid[0]);
    printf("PNM: %c%c%c%c%c\n", cid->pnm[4], cid->pnm[3], cid->pnm[2], cid->pnm[1], cid->pnm[0]);
    printf("PRV: %x.%x\n", cid->prv >> 4, cid->prv & 0xf);
    printf("PSN: %08x\n", cid->psn);
    printf("MDT: %d,%d\n", cid->mdt & 0xf, ((cid->mdt >> 4) & 0xff) + 2000);
#endif //SDMMC_DEBUG

    sdmmcs->serial = cid->psn;
    if (!sdmmcs_cmd_r6(sdmmcs, SDMMC_CMD_SEND_RELATIVE_ADDR, 0))
    {
        error(ERROR_HARDWARE);
        return false;
    }
#if (SDMMC_DEBUG)
    printf("SDMMC RCA: %04X\n", sdmmcs->rca);
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
    if (!sdmmcs_cmd(sdmmcs, SDMMC_CMD_SEND_CSD, ARG_RCA(sdmmcs), SDMMC_RESPONSE_R2))
    {
        error(ERROR_HARDWARE);
        return false;
    }
    memcpy(csd, sdmmcs->cmd_stack->resp, 16);
    if ((csd[15] >> 6) == 0x01)
    {
#if (SDMMC_DEBUG)
        printf("SDMMC: CSD v2\n");
#endif //SDMMC_DEBUG
        sdmmcs->sector_size = 512;
        c_size = ((((uint32_t)csd[8]) & 0x3f) << 16) | (((uint32_t)csd[7]) << 8) | (((uint32_t)csd[6]) << 0);
        sdmmcs->num_sectors = (c_size + 1) * 1024;
    }
    else
    {
#if (SDMMC_DEBUG)
        printf("SDMMC: CSD v1\n");
#endif //SDMMC_DEBUG
        c_size = ((((uint32_t)csd[9]) & 0x03) << 10) | (((uint32_t)csd[8]) << 2) | (((uint32_t)csd[7]) >> 6);
        mult = (((uint32_t)csd[5]) >> 7) | ((((uint32_t)csd[6]) & 0x03) << 1);
        sdmmcs->num_sectors = (c_size + 1) * (1 << (mult + 2));
        sdmmcs->sector_size = 1 << (((uint32_t)csd[10]) & 0x0f);
    }
#if (SDMMC_DEBUG)
        printf("SDMMC: speed grad:%x\n", csd[12]);
#endif //SDMMC_DEBUG

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
    printf("Capacity: %d.%d%cB\n", capacity / 10, capacity % 10, c);
    printf("Max clock: %dMHz\n", sdmmcs->max_clock / 1000000);
    if (sdmmcs->write_protected)
        printf("Data is write protected\n");
#endif //SDMMC_DEBUG
    return true;
}

static inline bool sdmmcs_card_select(SDMMCS* sdmmcs)
{
    if (!sdmmcs_cmd_r1b(sdmmcs, SDMMC_CMD_SELECT_DESELECT_CARD, ARG_RCA(sdmmcs)))
    {
#if (SDMMC_DEBUG)
        printf("SDMMC: card selection failure\n");
#endif //SDMMC_DEBUG
        error(ERROR_HARDWARE);
        return false;
    }

    sdmmcs_set_clock(sdmmcs, sdmmcs->max_clock);

    //disable pullap on DAT3
    if (!sdmmcs_acmd_r1(sdmmcs, SDMMC_ACMD_SET_CLR_CARD_DETECT, 0x00))
        return false;
    //set 4 bit bus
    if (sdmmcs_acmd_r1(sdmmcs, SDMMC_ACMD_SET_BUS_WIDTH, 0x2))
        sdmmcs_set_bus_width(sdmmcs, 4);

    return true;
}

static void sdmmcs_set_r1_error(SDMMCS* sdmmcs)
{
    uint32_t r1 = sdmmcs->cmd_stack->resp[0];
    if (r1 & SDMMC_R1_CARD_ECC_FAILED)
    {
        error(ERROR_CRC);
        return;
    }
    if (r1 & SDMMC_R1_FATAL_ERROR)
    {
        error(ERROR_HARDWARE);
        return;
    }
    if (r1 & SDMMC_R1_APP_ERROR)
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
    for (retries = 1000; retries; --retries)
    {
        if (sdmmcs_cmd_r1(sdmmcs, SDMMC_CMD_SEND_STATUS, ARG_RCA(sdmmcs)))
        {
            if ((sdmmcs->cmd_stack->resp[0] & SDMMC_R1_CURRENT_STATE_MSK) == SDMMC_R1_STATE_TRAN)
                return true;
        }
        sleep_ms(1);
    }
    error(ERROR_TIMEOUT);
    return false;
}

static inline void sdmmcs_close(SDMMCS* sdmmcs)
{
    sdmmcs_reset(sdmmcs);
    get_size_exo(HAL_REQ(HAL_SDMMC, IPC_CLOSE), sdmmcs->user, 0, 0);
}

static inline void sdmmcs_open(SDMMCS* sdmmcs, uint32_t user)
{
    uint32_t res;
    sdmmcs->card_type = SDMMC_NO_CARD;
    sdmmcs->last_error = SDMMC_ERROR_OK;
    sdmmcs->serial = 0x00000000;
    sdmmcs->rca = 0x0000;
    sdmmcs->num_sectors = sdmmcs->sector_size = sdmmcs->max_clock = 0;
    sdmmcs->write_protected = false;
    sdmmcs->writed_before = false;

    sdmmcs->user = user;
    sdmmcs->activity = INVALID_HANDLE;
    if( get_size_exo(HAL_REQ(HAL_SDMMC, IPC_OPEN), sdmmcs->user, 0, 0) < 0)
        return;
    sdmmcs_set_clock(sdmmcs, 400000);
    sdmmcs_set_bus_width(sdmmcs, 1);

    do
    {
        if(!sdmmcs_reset(sdmmcs))
            break;

        if(!sdmmcs_card_init(sdmmcs))
            break;
        if(!sdmmcs_card_address(sdmmcs))
            break;
        if(!sdmmcs_card_read_csd(sdmmcs))
            break;
        if(!sdmmcs_card_select(sdmmcs))
            break;
        return;
    }while(false);

    res = get_last_error();
    get_size_exo(HAL_REQ(HAL_SDMMC, IPC_CLOSE), sdmmcs->user, 0, 0);
    error(res);
    return;
}

static inline bool sdmmcs_prepare_write(SDMMCS* sdmmcs, unsigned int sectors)
{
    if (sectors > 1)
    {
        if (!sdmmcs_acmd_r1(sdmmcs, SDMMC_ACMD_SET_WR_BLK_ERASE_COUNT, sectors))
        {
            sdmmcs_set_r1_error(sdmmcs);
            return false;
        }
    }
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

static inline void sdmmcs_io(SDMMCS* sdmmcs, HANDLE process, HANDLE user, IO* io, unsigned int size, bool read)
{
    uint32_t sectors;
    int res;
    STORAGE_STACK* stack = io_stack(io);

    if ((user != sdmmcs->user) || (size == 0))
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if ((sdmmcs->activity != INVALID_HANDLE) && !(stack->flags & STORAGE_FLAG_IGNORE_ACTIVITY_ON_REQUEST))
    {
        ipc_post_inline(sdmmcs->activity, HAL_CMD(HAL_SDMMC, STORAGE_NOTIFY_ACTIVITY), sdmmcs->user, read ? 0 : STORAGE_FLAG_WRITE, 0);
        sdmmcs->activity = INVALID_HANDLE;
    }

    // only erase
    if (!read && ((stack->flags & STORAGE_MASK_MODE) == 0))
    {
        if(!sdmmcs_erase(sdmmcs, stack->sector, size))
            error(ERROR_HARDWARE);
        return;
    }

    sectors = size / sdmmcs->sector_size;
    if (size != (sectors * sdmmcs->sector_size))
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }

    if (sdmmcs->card_type == SDMMC_CARD_SD_V1 || sdmmcs->card_type == SDMMC_CARD_SD_V2)
        stack->sector *= sdmmcs->sector_size;

    if (!sdmmcs_wait_for_ready(sdmmcs))
        return;

    if(read)
    {
        res = io_read_sync_exo(HAL_IO_REQ(HAL_SDMMC, IPC_READ), sdmmcs->user, io, size);
        if(res)
            sdmmcs->writed_before = false;

    }else{
        if(!sdmmcs_prepare_write(sdmmcs, sectors))
                return;
        res = io_read_sync_exo(HAL_IO_REQ(HAL_SDMMC, IPC_WRITE), sdmmcs->user, io, size);
        if(res)
            sdmmcs->writed_before = true;
    }

}


static inline void sdmmcs_get_media_descriptor(SDMMCS* sdmmcs, IO* io)
{
    STORAGE_MEDIA_DESCRIPTOR* media;
    media = io_data(io);
    media->num_sectors = sdmmcs->num_sectors;
    //SDSC/HC
    media->num_sectors_hi = 0;
    media->sector_size = sdmmcs->sector_size;
    sprintf(STORAGE_MEDIA_SERIAL(media), "%08X", sdmmcs->serial);
    io->data_size = sizeof(STORAGE_MEDIA_DESCRIPTOR) + 8 + 1;
//    kexo_io(process, HAL_IO_CMD(HAL_SDMMC, STORAGE_GET_MEDIA_DESCRIPTOR), exo->sdmmc.user, io);
  //  kerror(ERROR_SYNC);
}

static inline void sdmmcs_flush(SDMMCS* sdmmcs)
{
    sdmmcs_cmd_r1b(sdmmcs, SDMMC_CMD_STOP_TRANSMISSION, 0x00);
    get_size_exo(HAL_REQ(HAL_SDMMC, IPC_FLUSH), sdmmcs->user, 0, 0);
}


static void sdmmcs_request(SDMMCS* sdmmcs, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        sdmmcs_open(sdmmcs, ipc->param1);
        ipc->param3 = get_last_error();
        break;
    case IPC_CLOSE:
        sdmmcs_close(sdmmcs);
        break;
    case IPC_READ:
        sdmmcs_io(sdmmcs, ipc->process, (HANDLE)ipc->param1, (IO*)ipc->param2, ipc->param3, true);
        break;
    case IPC_WRITE:
        sdmmcs_io(sdmmcs, ipc->process, (HANDLE)ipc->param1, (IO*)ipc->param2, ipc->param3, false);
        break;

    case IPC_FLUSH:
        sdmmcs_flush(sdmmcs);
        break;
    case STORAGE_GET_MEDIA_DESCRIPTOR:
        sdmmcs_get_media_descriptor(sdmmcs, (IO*)ipc->param2);
        break;
    case STORAGE_NOTIFY_ACTIVITY:
        sdmmcs->activity = ipc->process;
        error(ERROR_SYNC);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}



static void sdmmcs_exo_request(SDMMCS* sdmmcs, IPC* ipc)
{

}

void sdmmcs_main()
{
    IPC ipc;
    SDMMCS sdmmcs;
    sdmmcs_init(&sdmmcs);
#ifdef SDMMC_DEBUG
    open_stdout();
#endif //WEBS_DEBUG
    for (;;)
    {
        ipc_read(&ipc);
        if(HAL_GROUP(ipc.cmd) == HAL_SDMMC)
        {
            if(ipc.process == KERNEL_HANDLE)
                sdmmcs_exo_request(&sdmmcs, &ipc);
            else
                sdmmcs_request(&sdmmcs, &ipc);
        }else
            error(ERROR_NOT_SUPPORTED);
        ipc_write(&ipc);
    }
}



