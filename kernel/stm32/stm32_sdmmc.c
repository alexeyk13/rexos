/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2018, Alexey Kramarenko
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#include "stm32_sdmmc.h"

//#include "stm32_pin.h"
#include "stm32_exo_private.h"
#include "stm32_power.h"
#include "../kirq.h"
#include "../kexo.h"
#include "../kerror.h"
#include "../../userspace/stdio.h"
#include "../../userspace/stm32/stm32_driver.h"
#include "../../userspace/storage.h"
#include "../../userspace/endian.h"
#include "../../userspace/sdmmc.h"
#include "../../midware/sdmmcs/sdmmcs.h"
#include "../kstdlib.h"
#include <string.h>

#define SDMMC_READ_TIMEOUT_MS         100
#define SDMMC_WRITE_TIMEOUT_MS        250

#define SDMMC_POWER_RESET             0
#define SDMMC_POWER_OFF               2
#define SDMMC_POWER_ON                3

#define SDMMC_WAITRESP_NO             (0 << SDMMC_CMD_WAITRESP_Pos)
#define SDMMC_WAITRESP_SHORT          (1 << SDMMC_CMD_WAITRESP_Pos)
#define SDMMC_WAITRESP_SHORT_NO_CRC   (2 << SDMMC_CMD_WAITRESP_Pos)
#define SDMMC_WAITRESP_LONG           (3 << SDMMC_CMD_WAITRESP_Pos)

//CMDSENT   Set at the end of the command without response. (CPSM moves from SEND to IDLE)
//CMDREND   Set at the end of the command response when the CRC is OK. (CPSM moves from RECEIVE to IDLE)
//CCRCFAIL  Set at the end of the command response when the CRC is FAIL. (CPSM moves from RECEIVE to IDLE)
//CTIMEOUT  Set after the command when no response start bit received before the timeout. (CPSM moves from WAIT to IDLE)

#define SDMMC_INTMASK_CMD            (SDMMC_MASK_CMDSENTIE | SDMMC_MASK_CMDRENDIE | SDMMC_MASK_CCRCFAILIE | SDMMC_MASK_CTIMEOUTIE)
#define SDMMC_INTMASK_READ           (SDMMC_MASK_DABORTIE | SDMMC_MASK_DCRCFAILIE | SDMMC_MASK_DTIMEOUTIE | SDMMC_MASK_DATAENDIE | SDMMC_MASK_RXOVERRIE)
#define SDMMC_INTMASK_WRITE          (SDMMC_MASK_DABORTIE | SDMMC_MASK_DCRCFAILIE | SDMMC_MASK_DTIMEOUTIE | SDMMC_MASK_DATAENDIE | SDMMC_MASK_TXUNDERRIE)


#define SDMMC_NUM    0

typedef SDMMC_TypeDef* SDMMC_TypeDef_P;
static const SDMMC_TypeDef_P __SD_REGS[] =   {SDMMC1, SDMMC2};

static const uint32_t __SD_IRQ[] = {SDMMC1_IRQn, SDMMC2_IRQn};

#define SD_IRQ    __SD_IRQ[SDMMC_NUM]
#define SD_REG    __SD_REGS[SDMMC_NUM]

static inline int stm32_sdmmc_cmd_err(uint32_t resp_type, uint32_t sta)
{
    if (resp_type == SDMMC_NO_RESPONSE)
        return ERROR_OK;
    if (sta & SDMMC_STA_CTIMEOUT)
        return ERROR_TIMEOUT;
    if ((resp_type != SDMMC_RESPONSE_R3) && (sta & SDMMC_STA_CCRCFAIL))
        return ERROR_CRC;
    if ((resp_type == SDMMC_RESPONSE_R1B) && (sta & SDMMC_STA_BUSYD0))
        return ERROR_BUSY;
    return ERROR_OK;
}

static inline void stm32_sdmmc_on_cmd_isr(EXO* exo, uint32_t sta)
{
    int err;
    SDMMC_CMD_STACK* stack = io_stack(exo->sdmmc.io);
    SDMMC_RESPONSE_TYPE resp_type = stack->resp_type;
    err = stm32_sdmmc_cmd_err(resp_type, sta);
    kerror(err);
    SD_REG->ICR = SDMMC_INTMASK_CMD;
    stack->resp[0] = SD_REG->RESP1;
    if(resp_type == SDMMC_RESPONSE_DATA && err == ERROR_OK)
    {
        exo->sdmmc.state = SDMMC_STATE_DATA;

    }else{

        if(resp_type == SDMMC_RESPONSE_R2)
        {
            stack->resp[0] = SD_REG->RESP4;
            stack->resp[1] = SD_REG->RESP3;
            stack->resp[2] = SD_REG->RESP2;
            stack->resp[3] = SD_REG->RESP1;
        }
        iio_complete_ex(exo->sdmmc.process, HAL_IO_CMD(HAL_SDMMC, SDMMC_CMD), exo->sdmmc.user, exo->sdmmc.io, 0);
        exo->sdmmc.state = SDMMC_STATE_IDLE;
    }
}

static inline void stm32_sdmmc_v_switch_isr(EXO* exo, uint32_t sta)
{
    SD_REG->MASK &= ~SDMMC_MASK_VSWENDIE;
    SD_REG->POWER &= ~(SDMMC_POWER_VSWITCH | SDMMC_POWER_VSWITCHEN);
    SD_REG->ICR = SDMMC_ICR_VSWENDC;
    exo->sdmmc.state = SDMMC_STATE_IDLE;

    ipc_ipost_inline(exo->sdmmc.process, HAL_CMD(HAL_SDMMC, SDMMC_V_SWITCH), exo->sdmmc.user, 0, sta & SDMMC_STA_BUSYD0);
}

static inline void stm32_sdmmc_on_cmd_data_isr(EXO* exo, uint32_t sta)
{
    int err;
    uint32_t resp = SD_REG->RESP1;
    SD_REG->ICR = SDMMC_INTMASK_CMD;
    if( (sta & (SDMMC_STA_CMDREND) && (resp & (SDMMC_R1_FATAL_ERROR |SDMMC_R1_COM_CRC_ERROR)) == 0) )
    {
        exo->sdmmc.state = SDMMC_STATE_DATA;
        return;
    }
    if((sta & SDMMC_STA_CCRCFAIL) || (resp & SDMMC_R1_COM_CRC_ERROR))
    {
#if(STM32_SDMMC_DEBUG)
        printk("SDMMC32: repeat cmd sta:%x\n", sta);
#endif // STM32_SDMMC_DEBUG
        if(++exo->sdmmc.rep_cnt < 4)
        {
            SD_REG->CMD |= SDMMC_CMD_CPSMEN; // repeat command
            return;
        }
        err = ERROR_CRC;
    }
#if(STM32_SDMMC_DEBUG)
        printk("SDMMC32: error cmd sta:%x\n", sta);
#endif // STM32_SDMMC_DEBUG
    err = ERROR_HARDWARE;
    if( sta & SDMMC_STA_CTIMEOUT)
        err = ERROR_TIMEOUT;

    exo->sdmmc.state = SDMMC_STATE_IDLE;
    SD_REG->MASK = 0;
    iio_complete_ex(exo->sdmmc.process, HAL_IO_CMD(HAL_SDMMC, exo->sdmmc.data_ipc), exo->sdmmc.user, exo->sdmmc.io, err);
}

static inline void stm32_sdmmc_on_io_complete_isr(EXO* exo, int res)
{
    iio_complete_ex(exo->sdmmc.process, HAL_IO_CMD(HAL_SDMMC, exo->sdmmc.data_ipc), exo->sdmmc.user, exo->sdmmc.io, (uint32_t)res);
    exo->sdmmc.state = SDMMC_STATE_IDLE;
    SD_REG->MASK = 0;
    exo->sdmmc.io = NULL;
    exo->sdmmc.process = INVALID_HANDLE;
#if (STM32_SDMMC_PWR_SAVING)
    SD_REG->CLKCR |= SDMMC_CLKCR_PWRSAV;
#endif // STM32_SDMMC_PWR_SAVING

}


void stm32_sdmmc_on_isr(int vector, void* param)
{
    EXO* exo = (EXO*)param;
    uint32_t sta = SD_REG->STA;
    uint32_t res;
    switch (exo->sdmmc.state)
    {
    case SDMMC_STATE_DATA:
        SD_REG->ICR = SDMMC_INTMASK_WRITE | SDMMC_INTMASK_READ;
        SD_REG->DCTRL = 0;
        if (exo->sdmmc.data_ipc != IPC_WRITE)
            exo->sdmmc.io->data_size = exo->sdmmc.total - SD_REG->DCOUNT;
        if(sta & (SDMMC_MASK_DABORTIE | SDMMC_MASK_DATAENDIE))
        {
            if (exo->sdmmc.data_ipc == IPC_READ)
                exo->sdmmc.io->data_size = exo->sdmmc.total - SD_REG->DCOUNT;
            res = exo->sdmmc.io->data_size;
        }else{// error
#if(STM32_SDMMC_DEBUG)
        printk("SDMMC32: error data sta:%x cnt:%d\n", sta, SD_REG->DCOUNT);
#endif // STM32_SDMMC_DEBUG
            res = ERROR_HARDWARE;
            if(sta & SDMMC_STA_DCRCFAIL)
                res = ERROR_CRC;
            if(sta & SDMMC_STA_DTIMEOUT)
                res = ERROR_TIMEOUT;
        }
        if(exo->sdmmc.total > 512)
        {
            SD_REG->MASK = 0;
            SD_REG->CMD = (SDMMC_CMD_STOP_TRANSMISSION & SDMMC_CMD_CMDINDEX_Msk)| SDMMC_CMD_CPSMEN | SDMMC_CMD_CMDSTOP;//SDMMC_WAITRESP_SHORT
            while(SD_REG->CMD & SDMMC_CMD_CPSMEN);
            SD_REG->ICR = SDMMC_INTMASK_CMD;
        }
        stm32_sdmmc_on_io_complete_isr(exo, res);
        break;
    case SDMMC_STATE_CMD_DATA:
        stm32_sdmmc_on_cmd_data_isr(exo, sta);
        break;
    case SDMMC_STATE_CMD:
        stm32_sdmmc_on_cmd_isr(exo, sta);
        break;
    case SDMMC_STATE_V_SWITCH:
        stm32_sdmmc_v_switch_isr(exo, sta);
        break;
    default:
#if(STM32_SDMMC_DEBUG)
        printk("SDMMC32: unknown state sta:%x icr:%x cmd_reg:%x\n", sta, SD_REG->MASK, SD_REG->CMD);
#endif // STM32_SDMMC_DEBUG
        SD_REG->ICR = 0xffffffff;
        break;
    }
}

void stm32_sdmmc_init(EXO* exo)
{
    exo->sdmmc.active = false;
    exo->sdmmc.io = NULL;
    exo->sdmmc.process = INVALID_HANDLE;
    exo->sdmmc.user = INVALID_HANDLE;
    exo->sdmmc.state = SDMMC_STATE_IDLE;
    exo->sdmmc.activity = INVALID_HANDLE;
}


static inline void stm32_sdmmc_set_bus_width(int width)
{
    uint32_t w = 0;
    switch (width)
    {
    case 1:
        break;
    case 4:
        w = (1 << SDMMC_CLKCR_WIDBUS_Pos);
        break;
    case 8:
        w = (2 << SDMMC_CLKCR_WIDBUS_Pos);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
    SD_REG->CLKCR = (SD_REG->CLKCR & ~SDMMC_CLKCR_WIDBUS_Msk) | w;
}

static inline uint32_t stm32_sdmmc_set_clock(EXO* exo, unsigned int speed)
{
    uint32_t clock;
#if (SDMMC_CLOCK_SRC == SDMMC_CLOCK_SRC_PLL2_R)
    clock = stm32_power_get_clock_inside(exo, STM32_CLOCK_PLL2_R);
#else
    clock = stm32_power_get_clock_inside(exo, STM32_CLOCK_PLL1_Q);
#endif

    if (speed > STM32_SDMMC_MAX_CLOCK)
        speed = STM32_SDMMC_MAX_CLOCK;

    SD_REG->DTIMER = speed /4; // timeout 250ms always
    uint32_t div = (clock + speed)/(2* speed);
    if(speed > 50000000)
        SD_REG->CLKCR = (SD_REG->CLKCR & ~(SDMMC_CLKCR_CLKDIV_Msk | SDMMC_CLKCR_BUSSPEED)) | div;
    else
        SD_REG->CLKCR = (SD_REG->CLKCR & ~(SDMMC_CLKCR_CLKDIV_Msk)) | div | SDMMC_CLKCR_BUSSPEED;
    return div ? (clock / (2*div)): clock;
}

static inline void stm32_sdmmc_v_switch(EXO* exo)
{
    exo->sdmmc.state = SDMMC_STATE_V_SWITCH;
    SD_REG->MASK |= SDMMC_MASK_VSWENDIE;
    SD_REG->POWER |= SDMMC_POWER_VSWITCH;
    kerror(ERROR_SYNC);

}

/*
CMDSENT   Set at the end of the command without response. (CPSM moves from SEND to IDLE)
CMDREND   Set at the end of the command response when the CRC is OK. (CPSM moves from RECEIVE to IDLE)
CCRCFAIL  Set at the end of the command response when the CRC is FAIL. (CPSM moves from RECEIVE to IDLE)
CTIMEOUT  Set after the command when no response start bit received before the timeout. (CPSM moves from WAIT to IDLE)

CKSTOP    Set after the voltage switch (VSWITCHEN = 1) command response when the CRC is OK and the SDMMC_CK is stopped. (no impact on CPSM)
VSWEND    Set after the voltage switch (VSWITCH = 1) timeout of 5 ms + 1 ms. (no impact on CPSM)
CPSMACT   Command transfer in progress. (CPSM not in Idle state)
*/

static void sdmmcs_init_cmd(EXO* exo, uint8_t cmd, uint32_t arg, SDMMC_RESPONSE_TYPE resp_type)
{
    uint32_t reg;
    SD_REG->ARG = arg;
    reg = 0;
    switch (cmd)
    {
    case SDMMC_CMD_STOP_TRANSMISSION:
        reg |= SDMMC_CMD_CMDSTOP;
        break;
    case SDMMC_CMD_WRITE_SINGLE_BLOCK:
    case SDMMC_CMD_WRITE_MULTIPLE_BLOCK:
    case SDMMC_CMD_READ_SINGLE_BLOCK:
    case SDMMC_CMD_READ_MULTIPLE_BLOCK:
        reg |= SDMMC_CMD_CMDTRANS;
        break;
    case SDMMC_CMD_VOLTAGE_SWITCH:
        SD_REG->POWER |= SDMMC_POWER_VSWITCHEN;
        break;
    default:
        break;
    }

    switch(resp_type)
    {
    case SDMMC_NO_RESPONSE:
        reg |= SDMMC_WAITRESP_NO;
        break;
    case SDMMC_RESPONSE_R2:
        reg |= SDMMC_WAITRESP_LONG;
        break;
    case SDMMC_RESPONSE_R3:
        reg |= SDMMC_WAITRESP_SHORT_NO_CRC;
        break;
    default:
        reg |= SDMMC_WAITRESP_SHORT;
        break;

    }
    SD_REG->ICR = 0xffffffff;
    SD_REG->CMD = (cmd & SDMMC_CMD_CMDINDEX_Msk) | reg | SDMMC_CMD_CPSMEN;
}

static void stm32_sdmmc_flush(EXO* exo)
{
    IO* io;
    HANDLE process;
    SDMMC_STATE state = SDMMC_STATE_IDLE;
    __disable_irq();
    if (exo->sdmmc.state != SDMMC_STATE_IDLE)
    {
        process = exo->sdmmc.process;
        io = exo->sdmmc.io;
        state = exo->sdmmc.state;
        exo->sdmmc.state = SDMMC_STATE_IDLE;
    }
    __enable_irq();
    if (state != SDMMC_STATE_IDLE)
    {
        kexo_io_ex(process, HAL_IO_CMD(HAL_SDMMC, exo->sdmmc.data_ipc), exo->sdmmc.user, io, ERROR_IO_CANCELLED);
    }
}

static inline void stm32_sdmmc_open(EXO* exo, HANDLE user)
{
    if (exo->sdmmc.active)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }
#if(SDMMC_NUM == 0)
    RCC->AHB3ENR |= RCC_AHB3ENR_SDMMC1EN;
    RCC->AHB3RSTR |= RCC_AHB3RSTR_SDMMC1RST;
    RCC->AHB3RSTR &= ~RCC_AHB3RSTR_SDMMC1RST;
#else
    RCC->AHB2ENR |= RCC_AHB2ENR_SDMMC2EN;
    RCC->AHB2RSTR |= RCC_AHB2RSTR_SDMMC2RST;
    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_SDMMC2RST;
#endif

    SD_REG->IDMACTRL = SDMMC_IDMA_IDMAEN;
#if (SDMMC_CLOCK_SRC == SDMMC_CLOCK_SRC_PLL2_R)
    stm32_power_pll2_on();
#endif

    RCC->D1CCIPR = (RCC->D1CCIPR & RCC_D1CCIPR_SDMMCSEL) | SDMMC_CLOCK_SRC;
    SD_REG->CLKCR =  (STM32_SDMMC_RECEIVE_CLK_SRC << SDMMC_CLKCR_SELCLKRX_Pos);
#if (STM32_SDMMC_PWR_SAVING)
    SD_REG->CLKCR |= SDMMC_CLKCR_PWRSAV;
#endif // STM32_SDMMC_PWR_SAVING

    stm32_sdmmc_set_clock(exo, 300000);
#if (STM32_SDMMC_DIR_INVERSE)
    SD_REG->POWER = SDMMC_POWER_ON | SDMMC_POWER_DIRPOL;
#else
    SD_REG->POWER = SDMMC_POWER_ON;
#endif
    exodriver_delay_us(500); // 74 CLK cycle ??

    SD_REG->MASK = 0;
    kirq_register(KERNEL_HANDLE, SD_IRQ, stm32_sdmmc_on_isr, (void*)exo);
    NVIC_EnableIRQ(SD_IRQ);
    NVIC_SetPriority(SD_IRQ, 3);

    exo->sdmmc.user = user;
    exo->sdmmc.active = true;
}

static inline void stm32_sdmmc_close(EXO* exo)
{
    //disable interrupts
    NVIC_DisableIRQ(SD_IRQ);

    stm32_sdmmc_flush(exo);

    kirq_unregister(KERNEL_HANDLE, SD_IRQ);

#if(SDMMC_NUM == 0)
    RCC->AHB3RSTR |= RCC_AHB3RSTR_SDMMC1RST;
    RCC->AHB3RSTR &= ~RCC_AHB3RSTR_SDMMC1RST;
    RCC->AHB3ENR &= ~RCC_AHB3ENR_SDMMC1EN;
#else
    RCC->AHB2RSTR |= RCC_AHB2RSTR_SDMMC2RST;
    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_SDMMC2RST;
    RCC->AHB2ENR &= ~RCC_AHB2ENR_SDMMC2EN;
#endif

    exo->sdmmc.active = false;
    exo->sdmmc.user = INVALID_HANDLE;
}

static void stm32_sdmmc_prepare_io(EXO* exo, bool read, uint32_t block_size_log2)
{
    uint32_t reg;
    SD_REG->IDMACTRL = SDMMC_IDMA_IDMAEN;
    SD_REG->DLEN = exo->sdmmc.total;
    SD_REG->IDMABASE0 = (uint32_t)io_data(exo->sdmmc.io);
    SD_REG->ICR = 0xffffffff;
  //  SD_REG->MASK &= ~(SDMMC_INTMASK_READ | SDMMC_INTMASK_WRITE);


    reg = (block_size_log2 << SDMMC_DCTRL_DBLOCKSIZE_Pos);
    if(read)
    {
#if (STM32_DCACHE_ENABLE)
        if(exo->sdmmc.total > 16*1024)
            SCB_CleanInvalidateDCache();
        else
            SCB_InvalidateDCache_by_Addr(io_data(exo->sdmmc.io), exo->sdmmc.total);
#endif //STM32_DCACHE_ENABLE
        reg |= SDMMC_DCTRL_DTDIR;
        SD_REG->MASK |= SDMMC_INTMASK_READ;
    }else{
#if (STM32_DCACHE_ENABLE)
        if(exo->sdmmc.total > 16*1024)
            SCB_CleanDCache();
        else
            SCB_CleanDCache_by_Addr(io_data(exo->sdmmc.io), exo->sdmmc.total);
#endif //STM32_DCACHE_ENABLE
        SD_REG->MASK |= SDMMC_INTMASK_WRITE;
    }

    SD_REG->DCTRL = reg;
    SD_REG->IDMACTRL = SDMMC_IDMA_IDMAEN;

}


static inline void stm32_sdmmc_io(EXO* exo, HANDLE process, IO* io, unsigned int size, bool read)
{
    unsigned int count;
    STORAGE_STACK* stack = io_stack(io);
    io_pop(io, sizeof(STORAGE_STACK));
    if (size == 0)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
#if (STM32_SDMMC_PWR_SAVING)
    SD_REG->CLKCR &= ~SDMMC_CLKCR_PWRSAV;
#endif // STM32_SDMMC_PWR_SAVING

    count = size / 512;
    exo->sdmmc.total = size;

    exo->sdmmc.io = io;
    exo->sdmmc.process = process;

    stm32_sdmmc_prepare_io(exo, read, 9);

    exo->sdmmc.state = SDMMC_STATE_CMD_DATA;
    if (read)
    {
        io->data_size = 0;
        exo->sdmmc.data_ipc = IPC_READ;
        sdmmcs_init_cmd(exo, count == 1 ? SDMMC_CMD_READ_SINGLE_BLOCK : SDMMC_CMD_READ_MULTIPLE_BLOCK, stack->sector, SDMMC_RESPONSE_R1);
    }
    else if(stack->flags & STORAGE_FLAG_WRITE)
    {
        exo->sdmmc.data_ipc = IPC_WRITE;
        sdmmcs_init_cmd(exo, count == 1 ? SDMMC_CMD_WRITE_SINGLE_BLOCK : SDMMC_CMD_WRITE_MULTIPLE_BLOCK, stack->sector, SDMMC_RESPONSE_R1);
    }
    kerror(ERROR_SYNC);
}

static inline void stm32_sdmmc_run_cmd(EXO* exo, IO* io)
{
    SDMMC_CMD_STACK* cmd = io_stack(io);
    exo->sdmmc.io = io;
    exo->sdmmc.rep_cnt = 0;
    SD_REG->ICR = 0xffffffff;
    SD_REG->MASK |= SDMMC_INTMASK_CMD;
    exo->sdmmc.state = SDMMC_STATE_CMD;
    if(cmd->resp_type == SDMMC_RESPONSE_DATA)
    {
        io->data_size = 0;
        exo->sdmmc.total = 64;
        exo->sdmmc.data_ipc = SDMMC_CMD;
        stm32_sdmmc_prepare_io(exo, true, 6);
        SD_REG->ARG = cmd->arg;
        SD_REG->CMD = (cmd->cmd & SDMMC_CMD_CMDINDEX_Msk) | SDMMC_WAITRESP_SHORT | SDMMC_CMD_CMDTRANS| SDMMC_CMD_CPSMEN;
    }else
        sdmmcs_init_cmd(exo, cmd->cmd, cmd->arg, cmd->resp_type);

    kerror(ERROR_SYNC);
}

static inline void stm32_sdmmc_request_notify_activity(EXO* exo, HANDLE process)
{
    if (exo->sdmmc.activity != INVALID_HANDLE)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }
    exo->sdmmc.activity = process;
    kerror(ERROR_SYNC);
}

void stm32_sdmmc_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        stm32_sdmmc_open(exo, (HANDLE)ipc->param1);
        return;
    default:
        break;
    }

    if (!exo->sdmmc.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    if ((HANDLE)ipc->param1 != exo->sdmmc.user)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }

    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_CLOSE:
        stm32_sdmmc_close(exo);
        return;
    case IPC_FLUSH:
        stm32_sdmmc_flush(exo);
        return;
    default:
        break;
    }

    if (exo->sdmmc.state != SDMMC_STATE_IDLE)
    {
        kerror(ERROR_IN_PROGRESS);
        return;
    }

    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_READ:
        stm32_sdmmc_io(exo, ipc->process, (IO*)ipc->param2, ipc->param3, true);
        break;
    case IPC_WRITE:
        stm32_sdmmc_io(exo, ipc->process, (IO*)ipc->param2, ipc->param3, false);
        break;
    case SDMMC_CMD:
        exo->sdmmc.process = ipc->process;
        stm32_sdmmc_run_cmd(exo, (IO*)ipc->param2);
        break;
    case SDMMC_SET_DATA_WIDTH:
        stm32_sdmmc_set_bus_width(ipc->param3);
        break;
    case SDMMC_SET_CLOCK:
        ipc->param3 = stm32_sdmmc_set_clock(exo, ipc->param3);
        break;
    case SDMMC_V_SWITCH:
        stm32_sdmmc_v_switch(exo);
        break;

    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
}
