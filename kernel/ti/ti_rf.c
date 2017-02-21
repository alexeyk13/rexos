/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "ti_rf.h"
#include "ti_exo_private.h"
#include "ti_rf_private.h"
#include "../../userspace/process.h"
#include "../../userspace/error.h"
#include "../kstdlib.h"
#include "../kirq.h"
#include "../karray.h"
#include <stdint.h>
#include <string.h>

#define RF_DIRECT_CMD(cmd)                      (((cmd) << 16) | (1 << 0))
#define RF_RADIO_IMM_CMD(ptr)                   (((unsigned int)(ptr)) & ~3)

#define RF_BLE_BUF_SIZE                         64

//RFE patch, based on SmartRF
#define RFE_PATCH_WORDS                         13
static const uint32_t const __RFE_PATCH[RFE_PATCH_WORDS] =
                                                {   0x00001007, // Run patched RFE code from RAM
                                                    0x00354038, // Synth: Set RTRIM (POTAILRESTRIM) to 5
                                                    0x4001402D, // Synth: Correct CKVD latency setting (address)
                                                    0x00608402, // Synth: Correct CKVD latency setting (value)
                                                    0x4001405D, // Synth: Set ANADIV DIV_BIAS_MODE to PG1 (address)
                                                    0x1801F800, // Synth: Set ANADIV DIV_BIAS_MODE to PG1 (value)
                                                    0x000784A3, // Synth: Set FREF = 3.43 MHz (24 MHz / 7)
                                                    0xA47E0583, // Synth: Set loop bandwidth after lock to 80 kHz (K2)
                                                    0xEAE00603, // Synth: Set loop bandwidth after lock to 80 kHz (K3, LSB)
                                                    0x00010623, // Synth: Set loop bandwidth after lock to 80 kHz (K3, MSB)
                                                    0x00456088, // Adjust AGC reference level
                                                    0x013800C3, // Use enhanced BLE shape
                                                    0xFFFFFFFF};



// DBM proprietary encoding, based on SmartRF
#define TX_POWER_COUNT                          13
static const uint32_t const __TX_POWER[TX_POWER_COUNT] =
                                                {0x0cc7, 0x0cc9, 0x0ccb, 0x144b, 0x194e, 0x1d52, 0x2558, 0x3161,
                                                 0x4214, 0x4e18, 0x5a1c, 0x9324, 0x9330};
static const int8_t const __TX_POWER_DBM[TX_POWER_COUNT] =
                                                {-21,    -18,    -15,    -12,    -9,     -6,     -3,     0,
                                                 1,      2,      3,      4,      5};

static void* ti_rf_allocate_buf(EXO* exo)
{
    void* buf = NULL;
    unsigned int size;

    size = karray_size(exo->rf.free_bufs);
    if (size)
    {
        buf = *((void**)karray_at(exo->rf.free_bufs, size - 1));
        karray_remove(&exo->rf.free_bufs, size - 1);
    }
    if (buf == NULL)
    {
        if (exo->rf.buf_allocated < TI_RF_MAX_FRAMES_COUNT)
        {
            buf = kmalloc(RF_BLE_BUF_SIZE);
            if (buf)
                ++exo->rf.buf_allocated;
#if (TI_RF_DEBUG_ERRORS)
            else
                printk("RF: Out of memory\n");
#endif //TI_RF_DEBUG_ERRORS
        }
#if (TI_RF_DEBUG_ERRORS)
        else
            printk("RF: Too many frames\n");
#endif //TI_RF_DEBUG_ERRORS
    }
    return buf;
}

void ti_rf_release_buf(EXO* exo, void* buf)
{
    *((void**)karray_append(&exo->rf.free_bufs)) = buf;
}

static uint16_t ti_rf_encode_tx_power(int dbm)
{
    int dbm_delta, dbm_delta_cur;
    unsigned int i;
    uint16_t raw = __TX_POWER[0];
    dbm_delta = __TX_POWER_DBM[0] - dbm;
    if (dbm_delta < 0)
        dbm_delta = -dbm_delta;
    for (i = 1; (i < TX_POWER_COUNT) && dbm_delta; ++i)
    {
        dbm_delta_cur = __TX_POWER_DBM[i] - dbm;
        if (dbm_delta_cur < 0)
            dbm_delta_cur = -dbm_delta_cur;
        if (dbm_delta_cur < dbm_delta)
        {
            dbm_delta = dbm_delta_cur;
            raw = __TX_POWER[i];
        }
    }
    return raw;
}

static void ti_rf_direct_cmd(EXO* exo, uint16_t cmd, HANDLE process)
{
    exo->rf.process = process;
    exo->rf.cmd = cmd;
    RFC_DBELL->CMDR = RF_DIRECT_CMD(cmd);
}

static void ti_rf_imm_cmd(EXO* exo, uint16_t cmd, void* buf, HANDLE process)
{
    exo->rf.process = process;
    exo->rf.cmd = cmd;
    exo->rf.cur = buf;
    ((RF_CMD_IMMEDIATE_GENERIC_TYPE*)buf)->commandNo = RF_CMD_GET_FW_INFO;
    RFC_DBELL->CMDR = RF_RADIO_IMM_CMD(buf);
}

static inline void ti_rf_fw_info_fsm(EXO* exo)
{
    RF_CMD_GET_FW_INFO_TYPE* fw_info;

    exo->rf.cmd = RF_CMD_IDLE;
    if ((RFC_DBELL->CMDSTA & 0xff) == RF_RESULT_DONE)
    {
        fw_info = exo->rf.cur;
        exo->rf.rfc_free = (void*)(RFC_RAM_BASE + (unsigned int)fw_info->startOffset);
#if (TI_RF_DEBUG_INFO)
        printk("RF FW ver: %d.%d\n", fw_info->versionNo >> 8, fw_info->versionNo & 0xff);
#endif //TI_RF_DEBUG_INFO
        exo->rf.state = RF_STATE_CONFIGURED;
        ipc_ipost_inline(exo->rf.process, HAL_CMD(HAL_RF, IPC_OPEN), 0, 0, 0);
    }
    else
        ipc_ipost_inline(exo->rf.process, HAL_CMD(HAL_RF, IPC_OPEN), 0, 0, ERROR_HARDWARE);

    ti_rf_release_buf(exo, exo->rf.cur);
}

static inline void ti_rf_start_rat_fsm(EXO* exo)
{
    if ((RFC_DBELL->CMDSTA & 0xff) == RF_RESULT_DONE)
    {
        exo->rf.cmd = RF_CMD_RADIO_SETUP;
        RFC_DBELL->CMDR = RF_RADIO_IMM_CMD(exo->rf.rfc_free);
    }
    else
    {
        exo->rf.cmd = RF_CMD_IDLE;
        ipc_ipost_inline(exo->rf.process, HAL_CMD(HAL_RF, IPC_RF_POWER_UP), 0, 0, ERROR_HARDWARE);
    }
}

static inline void ti_rf_radio_up_fsm(EXO* exo)
{
    exo->rf.cmd = RF_CMD_IDLE;
    if ((RFC_DBELL->CMDSTA & 0xff) == RF_RESULT_DONE)
    {
        exo->rf.state = RF_STATE_READY;
        ipc_ipost_inline(exo->rf.process, HAL_CMD(HAL_RF, IPC_RF_POWER_UP), 0, 0, 0);
    }
    else
        ipc_ipost_inline(exo->rf.process, HAL_CMD(HAL_RF, IPC_RF_POWER_UP), 0, 0, ERROR_HARDWARE);
}

static inline void ti_rf_set_tx_power_fsm(EXO* exo)
{
    exo->rf.cmd = RF_CMD_IDLE;
    ti_rf_release_buf(exo, exo->rf.cur);
    ipc_ipost_inline(exo->rf.process, HAL_CMD(HAL_RF, IPC_RF_SET_TX_POWER), 0, 0, ((RFC_DBELL->CMDSTA & 0xff) == RF_RESULT_DONE) ? 0 : ERROR_HARDWARE);
}

static void ti_rf_cmd_ack_isr(int vector, void* param)
{
    EXO* exo = param;
    RFC_DBELL->RFACKIFG &= ~RFC_DBELL_RFACKIFG_ACKFLAG;

#if (TI_RF_DEBUG_ERRORS)
    if ((RFC_DBELL->CMDSTA & 0xff) != RF_RESULT_DONE)
        printk("RF CMD error: %x\n", RFC_DBELL->CMDSTA & 0xff);
#endif //(TI_RF_DEBUG_ERRORS

    switch (exo->rf.cmd)
    {
    case RF_CMD_GET_FW_INFO:
        ti_rf_fw_info_fsm(exo);
        break;
    case RF_CMD_START_RAT:
        ti_rf_start_rat_fsm(exo);
        break;
    case RF_CMD_RADIO_SETUP:
        ti_rf_radio_up_fsm(exo);
        break;
    case RF_CMD_SET_TX_POWER:
        ti_rf_set_tx_power_fsm(exo);
        break;
    default:
#if (TI_RF_DEBUG_ERRORS)
        printk("RF: Unknown FSM state: %d\n", exo->rf.cmd);
#endif //TI_RF_DEBUG_ERRORS
    }
}

static void ti_rf_cpe0_isr(int vector, void* param)
{
//    EXO* exo = param;
    RFC_DBELL->RFCPEIFG &= ~RFC_DBELL_RFCPEIFG_LAST_COMMAND_DONE;
    //TODO:
}

void ti_rf_init(EXO* exo)
{
    exo->rf.buf_allocated = 0;
    karray_create(&exo->rf.free_bufs, sizeof(void*), 1);
    exo->rf.state = RF_STATE_IDLE;
    exo->rf.cmd = RF_CMD_IDLE;
}

static inline void ti_rf_open(EXO* exo, HANDLE process)
{
    if (exo->rf.state != RF_STATE_IDLE)
        return;

    //power up RFC domain
    PRCM->PDCTL1RFC = PRCM_PDCTL1RFC_ON;
    while ((PRCM->PDSTAT1RFC & PRCM_PDSTAT1RFC_ON) == 0) {}

    //gate clock for RFC
    PRCM->RFCCLKG = PRCM_RFCCLKG_CLK_EN;
    PRCM->CLKLOADCTL = PRCM_CLKLOADCTL_LOAD;
    while ((PRCM->CLKLOADCTL & PRCM_CLKLOADCTL_LOAD_DONE) == 0) {}

    //enable clock for RFC modules
    RFC_PWR->PWMCLKEN = RFC_PWR_PWMCLKEN_RFCTRC | RFC_PWR_PWMCLKEN_FSCA | RFC_PWR_PWMCLKEN_PHA    | RFC_PWR_PWMCLKEN_RAT |
                        RFC_PWR_PWMCLKEN_RFERAM | RFC_PWR_PWMCLKEN_RFE  | RFC_PWR_PWMCLKEN_MDMRAM | RFC_PWR_PWMCLKEN_MDM |
                        RFC_PWR_PWMCLKEN_CPERAM | RFC_PWR_PWMCLKEN_CPR  | RFC_PWR_PWMCLKEN_RFC;

    //enable RF core cmd ack interrupt
    RFC_DBELL->RFACKIFG &= ~RFC_DBELL_RFACKIFG_ACKFLAG;
    NVIC_EnableIRQ(RFCCmdAck_IRQn);
    NVIC_SetPriority(RFCCmdAck_IRQn, 5);
    kirq_register(KERNEL_HANDLE, RFCCmdAck_IRQn, ti_rf_cmd_ack_isr, exo);

    //setup interrupts for radio interface
    RFC_DBELL->RFCPEIEN = 0;
    //CPE0 - command flow
///    RFC_DBELL->RFCPEIEN |= RFC_DBELL_RFCPEIEN_LAST_COMMAND_DONE;
    RFC_DBELL->RFCPEISL &= ~RFC_DBELL_RFCPEISL_LAST_COMMAND_DONE;

    RFC_DBELL->RFCPEIFG = 0;
///    NVIC_EnableIRQ(RFCCPE0_IRQn);
    NVIC_SetPriority(RFCCPE0_IRQn, 3);
    kirq_register(KERNEL_HANDLE, RFCCPE0_IRQn, ti_rf_cpe0_isr, exo);

    // ---> FW_INFO ---> CONFIGURED
    ti_rf_imm_cmd(exo, RF_CMD_GET_FW_INFO, ti_rf_allocate_buf(exo), process);
    error(ERROR_SYNC);
}

static inline void ti_rf_close(EXO* exo)
{
    NVIC_DisableIRQ(RFCCPE0_IRQn);
    kirq_unregister(KERNEL_HANDLE, RFCCPE0_IRQn);

    NVIC_DisableIRQ(RFCCmdAck_IRQn);
    kirq_unregister(KERNEL_HANDLE, RFCCmdAck_IRQn);

    //disable clocks
    RFC_PWR->PWMCLKEN = RFC_PWR_PWMCLKEN_RFC;

    //disable clock for RFC
    PRCM->RFCCLKG &= ~PRCM_RFCCLKG_CLK_EN;
    PRCM->CLKLOADCTL = PRCM_CLKLOADCTL_LOAD;
    while ((PRCM->CLKLOADCTL & PRCM_CLKLOADCTL_LOAD_DONE) == 0) {}

    //power down RFC domain
    PRCM->PDCTL1RFC = 0;
    while (PRCM->PDSTAT1RFC & PRCM_PDSTAT1RFC_ON) {}
}

static inline void ti_rf_power_up(EXO* exo, HANDLE process)
{
    RF_CMD_RADIO_SETUP_TYPE* setup;
    void* rfe_patch;
    if (exo->rf.state != RF_STATE_CONFIGURED)
    {
        error(ERROR_INVALID_STATE);
        return;
    }

    //prepare radio setup command
    setup = exo->rf.rfc_free;
    //RFE patch must be located somewhere in RAM. It's not documented by TRM, but generating error by CPE if it's in flash.
    rfe_patch = (void*)(((uint32_t)(exo->rf.rfc_free) + sizeof(RF_CMD_RADIO_SETUP_TYPE) + 3) & ~3);
    memcpy(rfe_patch, __RFE_PATCH, RFE_PATCH_WORDS * sizeof(uint32_t));

    setup->header.commandNo = RF_CMD_RADIO_SETUP;
    setup->header.status = RF_STATE_IDLE;
    setup->header.pNextOp = NULL;
    setup->header.startTime = 0;
    setup->header.startTrigger = RF_CMD_HEADER_TRIG_TYPE_NOW;
    setup->header.condition = RF_CMD_HEADER_COND_RULE_NEVER;

    setup->mode = RF_CMD_RADIO_SETUP_MODE_BLE;
    setup->loDivider = 0;
    setup->config = RF_CMD_RADIO_SETUP_CONFIG_BIAS_INTERNAL | RF_CMD_RADIO_SETUP_CONFIG_MODE_DIFFERENTIAL;
    setup->txPower = ti_rf_encode_tx_power(0);
    setup->pRegOverride = rfe_patch;

    // CONFIGURED ---> Start RAT ---> Radio Setup ---> READY
    ti_rf_direct_cmd(exo, RF_CMD_START_RAT, process);
    error(ERROR_SYNC);
}

static inline void ti_rf_set_tx_power(EXO* exo, int dbm, HANDLE process)
{
    RF_CMD_SET_TX_POWER_TYPE* tx_power;
    if (exo->rf.state != RF_STATE_READY)
    {
        error(ERROR_INVALID_STATE);
        return;
    }

    // READY ---> Set TX Power ---> READY
    tx_power = ti_rf_allocate_buf(exo);
    if (tx_power == NULL)
        return;
    tx_power->txPower = ti_rf_encode_tx_power(dbm);
    ti_rf_imm_cmd(exo, RF_CMD_SET_TX_POWER, tx_power, process);
    error(ERROR_SYNC);
}

void ti_rf_request(EXO* exo, IPC* ipc)
{
    if (exo->rf.cmd != RF_CMD_IDLE)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        ti_rf_open(exo, ipc->process);
        break;
    case IPC_CLOSE:
        ti_rf_close(exo);
        break;
    case IPC_RF_POWER_UP:
        ti_rf_power_up(exo, ipc->process);
        break;
    case IPC_RF_POWER_DOWN:
        //TODO:
        break;
    case IPC_RF_SET_TX_POWER:
        ti_rf_set_tx_power(exo, (int)ipc->param1, ipc->process);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}
