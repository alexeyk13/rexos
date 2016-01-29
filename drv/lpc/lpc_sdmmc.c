/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_sdmmc.h"
#include "lpc_pin.h"
#include "lpc_core_private.h"
#include "lpc_power.h"
#include "../../userspace/irq.h"
#include "../../userspace/stdio.h"
#include "../../userspace/lpc/lpc_driver.h"
#include "../../userspace/storage.h"
#include "../../userspace/stdlib.h"

#define LPC_SDMMC_CMD_FLAGS                         (SDMMC_RINTSTS_RE_Msk | SDMMC_RINTSTS_CDONE_Msk | SDMMC_RINTSTS_RCRC_Msk | SDMMC_RINTSTS_RTO_BAR_Msk | \
                                                     SDMMC_RINTSTS_SBE_Msk | SDMMC_RINTSTS_EBE_Msk | SDMMC_RINTSTS_HLE_Msk)

#define LPC_MAX_CLOCK                               52000000

void lpc_sdmmc_on_isr(int vector, void* param)
{
    CORE* core = (CORE*)param;
    if (LPC_SDMMC->IDSTS & SDMMC_IDSTS_NIS_Msk)
    {
        //TODO: if not CMD23 set, stop transmission here
        if (core->sdmmc.process != INVALID_HANDLE)
        {
            core->sdmmc.io->data_size = core->sdmmc.count * core->sdmmc.sdmmcs.sector_size;
            iio_complete(core->sdmmc.process, HAL_IO_CMD(HAL_SDMMC, core->sdmmc.state == SDMMC_STATE_RX ? IPC_READ : IPC_WRITE), 0, core->sdmmc.io);
            core->sdmmc.io = NULL;
            core->sdmmc.process = INVALID_HANDLE;
            core->sdmmc.state = SDMMC_STATE_IDLE;
        }
        LPC_SDMMC->IDSTS = SDMMC_IDSTS_RI_Msk | SDMMC_IDSTS_TI_Msk | SDMMC_IDSTS_NIS_Msk;
    }
    //TODO: abnormal processing
}

void lpc_sdmmc_init(CORE* core)
{
    sdmmcs_init(&core->sdmmc.sdmmcs, core);
    core->sdmmc.active = false;
    core->sdmmc.io = NULL;
    core->sdmmc.process = INVALID_HANDLE;
    core->sdmmc.state = SDMMC_STATE_IDLE;
    core->sdmmc.descr = NULL;
}

static void lpc_sdmmc_start_cmd(unsigned int cmd)
{
    LPC_SDMMC->CMD = cmd | SDMMC_CMD_START_CMD_Msk;
    while (LPC_SDMMC->CMD & SDMMC_CMD_START_CMD_Msk) {}
}

void sdmmcs_set_bus_width(void* param, int width)
{
    switch (width)
    {
    case 1:
        LPC_SDMMC->CTYPE = 0;
        break;
    case 4:
        LPC_SDMMC->CTYPE = SDMMC_CTYPE_CARD_WIDTH0;
        break;
    case 8:
        LPC_SDMMC->CTYPE = SDMMC_CTYPE_CARD_WIDTH1;
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

void sdmmcs_set_clock(void* param, unsigned int speed)
{
    if (speed > LPC_MAX_CLOCK)
        speed = LPC_MAX_CLOCK;
    LPC_SDMMC->CLKENA = 0;
    lpc_sdmmc_start_cmd(SDMMC_CMD_UPDATE_CLOCK_REGISTERS_ONLY_Msk | SDMMC_CMD_WAIT_PRVDATA_COMPLETE_Msk);

    LPC_SDMMC->CLKDIV &= ~SDMMC_CLKDIV_CLK_DIVIDER0_Msk;
    LPC_SDMMC->CLKDIV |= (((lpc_power_get_core_clock_inside((CORE*)param) / speed + 1) >> 1) << SDMMC_CLKDIV_CLK_DIVIDER0_Pos) & SDMMC_CLKDIV_CLK_DIVIDER0_Msk;
    lpc_sdmmc_start_cmd(SDMMC_CMD_UPDATE_CLOCK_REGISTERS_ONLY_Msk | SDMMC_CMD_WAIT_PRVDATA_COMPLETE_Msk);

    LPC_SDMMC->CLKENA |= SDMMC_CLKENA_CCLK_ENABLE;
    lpc_sdmmc_start_cmd(SDMMC_CMD_UPDATE_CLOCK_REGISTERS_ONLY_Msk | SDMMC_CMD_WAIT_PRVDATA_COMPLETE_Msk);
}

SDMMC_ERROR sdmmcs_send_cmd(void* param, uint8_t cmd, uint32_t arg, void* resp, SDMMC_RESPONSE_TYPE resp_type)
{
    //clear pending interrupts
    LPC_SDMMC->RINTSTS = LPC_SDMMC_CMD_FLAGS;
    LPC_SDMMC->CMDARG = arg;
    switch (resp_type)
    {
    case SDMMC_NO_RESPONSE:
        if (cmd == SDMMC_CMD_GO_IDLE_STATE)
            lpc_sdmmc_start_cmd((cmd & 0x3f) | SDMMC_CMD_WAIT_PRVDATA_COMPLETE_Msk | SDMMC_CMD_SEND_INITIALIZATION_Msk);
        else
            lpc_sdmmc_start_cmd((cmd & 0x3f) | SDMMC_CMD_WAIT_PRVDATA_COMPLETE_Msk);
        break;
    case SDMMC_RESPONSE_R2:
        lpc_sdmmc_start_cmd((cmd & 0x3f) | SDMMC_CMD_WAIT_PRVDATA_COMPLETE_Msk | SDMMC_CMD_RESPONSE_EXPECT_Msk | SDMMC_CMD_RESPONSE_LENGTH_Msk |
                                                                              SDMMC_CMD_CHECK_RESPONSE_CRC_Msk);
        break;
    case SDMMC_RESPONSE_R3:
        lpc_sdmmc_start_cmd((cmd & 0x3f) | SDMMC_CMD_WAIT_PRVDATA_COMPLETE_Msk | SDMMC_CMD_RESPONSE_EXPECT_Msk);
        break;
    case SDMMC_RESPONSE_R1D:
        lpc_sdmmc_start_cmd((cmd & 0x3f) | SDMMC_CMD_WAIT_PRVDATA_COMPLETE_Msk | SDMMC_CMD_RESPONSE_EXPECT_Msk | SDMMC_CMD_DATA_EXPECTED_Msk |
                                           SDMMC_CMD_CHECK_RESPONSE_CRC_Msk);
        break;
    default:
        lpc_sdmmc_start_cmd((cmd & 0x3f) | SDMMC_CMD_WAIT_PRVDATA_COMPLETE_Msk | SDMMC_CMD_RESPONSE_EXPECT_Msk | SDMMC_CMD_CHECK_RESPONSE_CRC_Msk);
        break;
    }
    while ((LPC_SDMMC->RINTSTS & LPC_SDMMC_CMD_FLAGS) == 0) {}
    if (LPC_SDMMC->RINTSTS & (SDMMC_RINTSTS_SBE_Msk | SDMMC_RINTSTS_EBE_Msk | SDMMC_RINTSTS_HLE_Msk))
        return SDMMC_ERROR_HARDWARE_FAILURE;

    if (resp_type == SDMMC_NO_RESPONSE)
        return SDMMC_ERROR_OK;
    if (LPC_SDMMC->RINTSTS & (SDMMC_RINTSTS_RE_Msk | SDMMC_RINTSTS_RTO_BAR_Msk))
        return SDMMC_ERROR_TIMEOUT;
    if ((resp_type != SDMMC_RESPONSE_R3) && (LPC_SDMMC->RINTSTS & SDMMC_RINTSTS_RCRC_Msk))
        return SDMMC_ERROR_CRC_FAIL;
    if ((resp_type == SDMMC_RESPONSE_R1B) && (LPC_SDMMC->STATUS & SDMMC_STATUS_DATA_BUSY_Msk))
        return SDMMC_ERROR_BUSY;
    ((uint32_t*)resp)[0] = LPC_SDMMC->RESP0;

    if (resp_type == SDMMC_RESPONSE_R2)
    {
        ((uint32_t*)resp)[1] = LPC_SDMMC->RESP1;
        ((uint32_t*)resp)[2] = LPC_SDMMC->RESP2;
        ((uint32_t*)resp)[3] = LPC_SDMMC->RESP3;
    }
    return SDMMC_ERROR_OK;
}

static inline void lpc_sdmmc_open(CORE* core)
{
    if (core->sdmmc.active)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    //enable SDIO bus interface to PLL1
    LPC_CGU->BASE_SDIO_CLK = CGU_BASE_SDIO_CLK_PD_Pos;
    LPC_CGU->BASE_SDIO_CLK |= CGU_CLK_PLL1;
    LPC_CGU->BASE_SDIO_CLK &= ~CGU_BASE_SDIO_CLK_PD_Pos;

    //According to datasheet, we need to set SDDELAY here. But there is no SDDELAY register in CMSIS libs. WTF?

    //reset bus, controller, DMA, FIFO
    LPC_SDMMC->BMOD = SDMMC_BMOD_SWR_Msk;
    LPC_SDMMC->CTRL = SDMMC_CTRL_CONTROLLER_RESET_Msk | SDMMC_CTRL_FIFO_RESET_Msk | SDMMC_CTRL_DMA_RESET_Msk ;
    __NOP();
    __NOP();
    //mask and clear pending suspious interrupts
    LPC_SDMMC->INTMASK = 0;
    LPC_SDMMC->RINTSTS = 0x1ffff;
    LPC_SDMMC->IDSTS = 0x337;

    if (!sdmmcs_open(&core->sdmmc.sdmmcs))
        return;
    //it's critical to call malloc here, because of align
    core->sdmmc.descr = malloc(4 * sizeof(uint32_t));
    //TODO: setup block size here

    //recommended values
    LPC_SDMMC->FIFOTH = (8 << SDMMC_FIFOTH_TX_WMARK_Pos) | (7 << SDMMC_FIFOTH_RX_WMARK_Pos) | SDMMC_FIFOTH_DMA_MTS_8;

    //enable internal DMA
    LPC_SDMMC->BMOD = SDMMC_BMOD_DE_Msk | SDMMC_BMOD_FB_Msk;
    LPC_SDMMC->CTRL |= SDMMC_CTRL_USE_INTERNAL_DMAC_Msk;


    //setup interrupt vector
    irq_register(SDIO_IRQn, lpc_sdmmc_on_isr, (void*)core);
    NVIC_EnableIRQ(SDIO_IRQn);
    NVIC_SetPriority(SDIO_IRQn, 3);

    //Enable interrupts
    LPC_SDMMC->IDINTEN = SDMMC_IDINTEN_TI_Msk | SDMMC_IDINTEN_RI_Msk | SDMMC_IDINTEN_CES_Msk | SDMMC_IDINTEN_NIS_Msk | SDMMC_IDINTEN_AIS_Msk ;
    //enable global interrupt mask
    LPC_SDMMC->CTRL |= SDMMC_CTRL_INT_ENABLE_Msk;

    core->sdmmc.active = true;
}

static inline void lpc_sdmmc_read(CORE* core, HANDLE process, IO* io)
{
    STORAGE_STACK* stack = io_stack(io);
    io->data_size = 0;
    io_pop(io, sizeof(STORAGE_STACK));
    if (!core->sdmmc.active)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    if (core->sdmmc.state != SDMMC_STATE_IDLE)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    core->sdmmc.count = stack->count;
    core->sdmmc.io = io;
    core->sdmmc.process = process;

    //configure descriptor
    //TODO: bits definition
    core->sdmmc.descr[0] = (1 << 2) | (1 << 3) | (1 << 4);
    LPC_SDMMC->BYTCNT = core->sdmmc.descr[1] = 512 * stack->count;
    core->sdmmc.descr[2] = (unsigned int)io_data(io);
    core->sdmmc.descr[3] = 0x00000000;
    //give buffer to DMA
    LPC_SDMMC->DBADDR = (unsigned int)core->sdmmc.descr;
    core->sdmmc.descr[0] |= (1 << 31);
    LPC_SDMMC->PLDMND = 1;
    core->sdmmc.state = SDMMC_STATE_RX;
    if (sdmmcs_read_single_block(&core->sdmmc.sdmmcs, stack->sector))
        error(ERROR_SYNC);
}

void lpc_sdmmc_request(CORE* core, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        lpc_sdmmc_open(core);
        break;
    case IPC_CLOSE:
        //TODO:
    case IPC_READ:
        lpc_sdmmc_read(core, ipc->process, (IO*)ipc->param2);
        break;
    case IPC_WRITE:
        //TODO:
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}
