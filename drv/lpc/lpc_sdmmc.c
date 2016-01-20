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
    SDMMC_RESULT_OK = 0,
    SDMMC_RESULT_TIMEOUT,
    SDMMC_RESULT_CRC_FAIL,
    SDMMC_RESULT_BUSY,
    SDMMC_RESULT_HARDWARE_FAILURE
} SDMMC_RESULT;

#define LPC_SDMMC_CMD_FLAGS                         (SDMMC_RINTSTS_RE_Msk | SDMMC_RINTSTS_CDONE_Msk | SDMMC_RINTSTS_RCRC_Msk | SDMMC_RINTSTS_RTO_BAR_Msk | \
                                                     SDMMC_RINTSTS_SBE_Msk | SDMMC_RINTSTS_EBE_Msk | SDMMC_RINTSTS_HLE_Msk)


void lpc_sdmmc_init(CORE* core)
{
    //TODO:
}

static void lpc_sdmmc_start_cmd(unsigned int cmd)
{
    LPC_SDMMC->CMD = cmd | SDMMC_CMD_START_CMD_Msk;
    while (LPC_SDMMC->CMD & SDMMC_CMD_START_CMD_Msk) {}
}

void lpc_sdmmc_set_clock(CORE* core, unsigned int speed)
{
    LPC_SDMMC->CLKENA = 0;
    lpc_sdmmc_start_cmd(SDMMC_CMD_UPDATE_CLOCK_REGISTERS_ONLY_Msk | SDMMC_CMD_WAIT_PRVDATA_COMPLETE_Msk);

    LPC_SDMMC->CLKDIV &= ~SDMMC_CLKDIV_CLK_DIVIDER0_Msk;
    LPC_SDMMC->CLKDIV |= (((lpc_power_get_core_clock_inside(core) / speed + 1) >> 1) << SDMMC_CLKDIV_CLK_DIVIDER0_Pos) & SDMMC_CLKDIV_CLK_DIVIDER0_Msk;
    lpc_sdmmc_start_cmd(SDMMC_CMD_UPDATE_CLOCK_REGISTERS_ONLY_Msk | SDMMC_CMD_WAIT_PRVDATA_COMPLETE_Msk);

    LPC_SDMMC->CLKENA |= SDMMC_CLKENA_CCLK_ENABLE;
    lpc_sdmmc_start_cmd(SDMMC_CMD_UPDATE_CLOCK_REGISTERS_ONLY_Msk | SDMMC_CMD_WAIT_PRVDATA_COMPLETE_Msk);
}

SDMMC_RESULT lpc_sdmmc_send_cmd(CORE* core, uint8_t cmd, uint32_t arg, void* resp, SDMMC_RESPONSE_TYPE resp_type)
{
    //clear pending interrupts
    LPC_SDMMC->RINTSTS = LPC_SDMMC_CMD_FLAGS;
    LPC_SDMMC->CMDARG = arg;
    switch (resp_type)
    {
    case SDMMC_NO_RESPONSE:
        //TODO: 0 - init
        lpc_sdmmc_start_cmd((cmd & 0x3f) | SDMMC_CMD_WAIT_PRVDATA_COMPLETE_Msk);
        break;
    case SDMMC_RESPONSE_R2:
        lpc_sdmmc_start_cmd((cmd & 0x3f) | SDMMC_CMD_WAIT_PRVDATA_COMPLETE_Msk | SDMMC_CMD_RESPONSE_EXPECT_Msk | SDMMC_CMD_RESPONSE_LENGTH_Msk |
                                                                              SDMMC_CMD_CHECK_RESPONSE_CRC_Msk);
        break;
    case SDMMC_RESPONSE_R3:
        lpc_sdmmc_start_cmd((cmd & 0x3f) | SDMMC_CMD_WAIT_PRVDATA_COMPLETE_Msk | SDMMC_CMD_RESPONSE_EXPECT_Msk);
        break;
    default:
        lpc_sdmmc_start_cmd((cmd & 0x3f) | SDMMC_CMD_WAIT_PRVDATA_COMPLETE_Msk | SDMMC_CMD_RESPONSE_EXPECT_Msk | SDMMC_CMD_CHECK_RESPONSE_CRC_Msk);
        break;
    }
    while ((LPC_SDMMC->RINTSTS & LPC_SDMMC_CMD_FLAGS) == 0) {}
    if (LPC_SDMMC->RINTSTS & (SDMMC_RINTSTS_SBE_Msk | SDMMC_RINTSTS_EBE_Msk | SDMMC_RINTSTS_HLE_Msk))
        return SDMMC_RESULT_HARDWARE_FAILURE;

    if (resp_type == SDMMC_NO_RESPONSE)
        return SDMMC_RESULT_OK;
    if (LPC_SDMMC->RINTSTS & (SDMMC_RINTSTS_RE_Msk | SDMMC_RINTSTS_RTO_BAR_Msk))
        return SDMMC_RESULT_TIMEOUT;
    if ((resp_type != SDMMC_RESPONSE_R3) && (LPC_SDMMC->RINTSTS & SDMMC_RINTSTS_RCRC_Msk))
        return SDMMC_RESULT_CRC_FAIL;
    if ((resp_type == SDMMC_RESPONSE_R1B) && (LPC_SDMMC->STATUS & SDMMC_STATUS_DATA_BUSY_Msk))
        return SDMMC_RESULT_BUSY;
    ((uint32_t*)resp)[0] = LPC_SDMMC->RESP0;

    if (resp_type == SDMMC_RESPONSE_R2)
    {
        ((uint32_t*)resp)[1] = LPC_SDMMC->RESP1;
        ((uint32_t*)resp)[2] = LPC_SDMMC->RESP2;
        ((uint32_t*)resp)[3] = LPC_SDMMC->RESP3;
    }
    return SDMMC_RESULT_OK;
}

void lpc_sdmmc_set_bus_width(CORE* core, int width)
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


void lpc_sdmmc_open(CORE* core)
{
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

    //TODO: setup DMA, buffer offset
    LPC_SDMMC->CTRL |= SDMMC_CTRL_USE_INTERNAL_DMAC_Msk;

    //mask and clear pending suspious interrupts
    LPC_SDMMC->INTMASK = 0;
    LPC_SDMMC->RINTSTS = 0x1ffff;
    //TODO: setup interrupt vector, enable generic interrupts
    //enable global interrupt mask
    LPC_SDMMC->CTRL |= SDMMC_CTRL_INT_ENABLE_Msk;

    //TODO: check
    lpc_sdmmc_set_clock(core, 400000);
    lpc_sdmmc_set_bus_width(core, 1);

    uint32_t resp;
    unsigned int d;
    d = lpc_sdmmc_send_cmd(core, 0x00, 0, &resp, SDMMC_NO_RESPONSE);
    printd("d0: %#X\n", d);
    d = lpc_sdmmc_send_cmd(core, 0x08, 0x000001AA, &resp, SDMMC_RESPONSE_R7);
    printd("d1: %#X\n", d);
    printd("resp: %#X\n", resp);
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
        //TODO:
    case IPC_WRITE:
        //TODO:
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}
