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
#include "../kheap.h"
#include <stdint.h>
#include <string.h>

#define RF_DIRECT_CMD(cmd)                      (((cmd) << 16) | (1 << 0))
#define RF_RADIO_IMM_CMD(ptr)                   (((unsigned int)(ptr)) & ~3)

//avoid too much fragmenation
#define RF_MIN_BUF_SIZE                         32
#define RF_BLE_BUF_SIZE                         64

//RFE patch, based on SmartRF
#define RFE_PATCH_WORDS                         7
static const uint32_t const __RFE_PATCH[RFE_PATCH_WORDS] =
                                                {
                                                    0x00364038, // Synth: Set RTRIM (POTAILRESTRIM) to 6
                                                    0x000784A3, // Synth: Set FREF = 3.43 MHz (24 MHz / 7)
                                                    0xA47E0583, // Synth: Set loop bandwidth after lock to 80 kHz (K2)
                                                    0xEAE00603, // Synth: Set loop bandwidth after lock to 80 kHz (K3, LSB)
                                                    0x00010623, // Synth: Set loop bandwidth after lock to 80 kHz (K3, MSB)
                                                    0x00456088, // Adjust AGC reference level
                                                    0xFFFFFFFF};


// DBM proprietary encoding, based on SmartRF
#define TX_POWER_COUNT                          13
static const uint32_t const __TX_POWER[TX_POWER_COUNT] =
                                                {0x0cc7, 0x0cc9, 0x0ccb, 0x144b, 0x194e, 0x1d52, 0x2558, 0x3161,
                                                 0x4214, 0x4e18, 0x5a1c, 0x9324, 0x9330};
static const int8_t const __TX_POWER_DBM[TX_POWER_COUNT] =
                                                {-21,    -18,    -15,    -12,    -9,     -6,     -3,     0,
                                                 1,      2,      3,      4,      5};

#define BUF_ALLOC(exo, size)                    kheap_malloc((exo)->rf.heap, (size) < RF_MIN_BUF_SIZE ? RF_MIN_BUF_SIZE : (size))

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

static void* ti_rf_prepare_radio_cmd(EXO* exo, unsigned int size, uint16_t cmd)
{
    RF_CMD_HEADER_TYPE* hdr = BUF_ALLOC(exo, size);
    if (hdr == NULL)
        return NULL;
    hdr->commandNo = cmd;
    hdr->status = RF_STATE_IDLE;
    hdr->pNextOp = NULL;
    hdr->startTime = 0;
    hdr->startTrigger = RF_CMD_HEADER_TRIG_TYPE_NOW;
    hdr->condition = RF_CMD_HEADER_COND_RULE_NEVER;
    exo->rf.cmd = cmd;
    exo->rf.cur = hdr;
    return hdr;
}

static inline void ti_rf_radio_cmd(void* buf)
{
    RFC_DBELL->CMDR = RF_RADIO_IMM_CMD(buf);
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
    fw_info = exo->rf.cur;
#if (TI_RF_DEBUG_INFO)
    printk("RF FW ver: %d.%d\n", fw_info->versionNo >> 8, fw_info->versionNo & 0xff);
#endif //TI_RF_DEBUG_INFO
    exo->rf.state = RF_STATE_CONFIGURED;
    kheap_free(exo->rf.cur);
    ipc_ipost_inline(exo->rf.process, HAL_CMD(HAL_RF, IPC_OPEN), 0, 0, 0);
}

static inline void ti_rf_start_rat_fsm(EXO* exo)
{
    RF_CMD_RADIO_SETUP_TYPE* setup;
    void* rfe_patch;
    setup = ti_rf_prepare_radio_cmd(exo, ((sizeof(RF_CMD_RADIO_SETUP_TYPE) + 3) & ~3) + RFE_PATCH_WORDS * sizeof(uint32_t), RF_CMD_RADIO_SETUP);
    //RFE patch must be located somewhere in RAM. It's not documented by TRM, but generating error by CPE if it's in flash.
    rfe_patch = (void*)(((uint32_t)(setup) + sizeof(RF_CMD_RADIO_SETUP_TYPE) + 3) & ~3);
    memcpy(rfe_patch, __RFE_PATCH, RFE_PATCH_WORDS * sizeof(uint32_t));

    setup->mode = RF_CMD_RADIO_SETUP_MODE_BLE;
    setup->loDivider = 0;
    setup->config = RF_CMD_RADIO_SETUP_CONFIG_BIAS_INTERNAL | RF_CMD_RADIO_SETUP_CONFIG_MODE_DIFFERENTIAL;
    setup->txPower = ti_rf_encode_tx_power(0);
    setup->pRegOverride = rfe_patch;

    ti_rf_radio_cmd(setup);
}

static inline void ti_rf_set_tx_power_fsm(EXO* exo)
{
    exo->rf.cmd = RF_CMD_IDLE;
    kheap_free(exo->rf.cur);
    ipc_ipost_inline(exo->rf.process, HAL_CMD(HAL_RF, IPC_RF_SET_TX_POWER), 0, 0, 0);
}

static void ti_rf_cmd_ack_isr(int vector, void* param)
{
    EXO* exo = param;
    RFC_DBELL->RFACKIFG &= ~RFC_DBELL_RFACKIFG_ACKFLAG;

    if ((RFC_DBELL->CMDSTA & 0xff) == RF_RESULT_DONE)
    //normal processing
    {
        switch (exo->rf.cmd)
        {
        case RF_CMD_GET_FW_INFO:
            ti_rf_fw_info_fsm(exo);
            break;
        case RF_CMD_START_RAT:
            ti_rf_start_rat_fsm(exo);
            break;
        case RF_CMD_SET_TX_POWER:
            ti_rf_set_tx_power_fsm(exo);
            break;
        default:
            //radio command sheduled - do nothing, will be processed on CPE0/CPE1
            break;
        }
    }
    else
    //error processing
    {
        //release buffer if not direct cmd
        switch (exo->rf.cmd)
        {
        case RF_CMD_START_RAT:
            break;
        default:
            kheap_free(exo->rf.cur);
        }

#if (TI_RF_DEBUG_ERRORS)
        printk("RF CMD error: %x\n", RFC_DBELL->CMDSTA & 0xff);
#endif //(TI_RF_DEBUG_ERRORS

        switch (exo->rf.cmd)
        {
        case RF_CMD_GET_FW_INFO:
            ipc_ipost_inline(exo->rf.process, HAL_CMD(HAL_RF, IPC_OPEN), 0, 0, ERROR_HARDWARE);
            break;
        case RF_CMD_START_RAT:
        case RF_CMD_RADIO_SETUP:
            ipc_ipost_inline(exo->rf.process, HAL_CMD(HAL_RF, IPC_RF_POWER_UP), 0, 0, ERROR_HARDWARE);
            break;
        case RF_CMD_SET_TX_POWER:
            ipc_ipost_inline(exo->rf.process, HAL_CMD(HAL_RF, IPC_RF_SET_TX_POWER), 0, 0, ERROR_HARDWARE);
            break;
        default:
            break;
        }
        exo->rf.cmd = RF_CMD_IDLE;
    }
}

static inline void ti_rf_radio_up_fsm(EXO* exo)
{
    kheap_free(exo->rf.cur);
    exo->rf.cmd = RF_CMD_IDLE;
    exo->rf.state = RF_STATE_READY;
    ipc_ipost_inline(exo->rf.process, HAL_CMD(HAL_RF, IPC_RF_POWER_UP), 0, 0, 0);
}

static void ti_rf_cpe0_isr(int vector, void* param)
{
    EXO* exo = param;
    uint16_t status;
    RFC_DBELL->RFCPEIFG = ~RFC_DBELL_RFCPEIFG_LAST_COMMAND_DONE;

    status = ((RF_CMD_HEADER_TYPE*)exo->rf.cur)->status;
    if (status < 0x800)
    //normal processing
    {
        switch (exo->rf.cmd)
        {
        case RF_CMD_RADIO_SETUP:
            ti_rf_radio_up_fsm(exo);
            break;
        case RF_CMD_COUNT:
            kheap_free(exo->rf.cur);
            exo->rf.cmd = RF_CMD_IDLE;
            break;
        default:
#if (TI_RF_DEBUG_ERRORS)
            printk("RF unexpected cmd: %x\n", exo->rf.cmd);
#endif //(TI_RF_DEBUG_ERRORS
            kheap_free(exo->rf.cur);
            exo->rf.cmd = RF_CMD_IDLE;
            break;
        }
    }
    else
    //error processing
    {
#if (TI_RF_DEBUG_ERRORS)
        printk("RF radio error: %x\n", status);
#endif //(TI_RF_DEBUG_ERRORS

        kheap_free(exo->rf.cur);

        switch (exo->rf.cmd)
        {
        case RF_CMD_RADIO_SETUP:
            ipc_ipost_inline(exo->rf.process, HAL_CMD(HAL_RF, IPC_RF_POWER_UP), 0, 0, ERROR_HARDWARE);
            break;
        default:
            break;
        }
        exo->rf.cmd = RF_CMD_IDLE;
    }
}

void ti_rf_init(EXO* exo)
{
    exo->rf.state = RF_STATE_IDLE;
    exo->rf.cmd = RF_CMD_IDLE;
}

static inline void ti_rf_open(EXO* exo, HANDLE process)
{
    if (exo->rf.state != RF_STATE_IDLE)
        return;

    exo->rf.heap = kheap_create(TI_RF_HEAP_SIZE);

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
    RFC_DBELL->RFCPEIEN |= RFC_DBELL_RFCPEIEN_LAST_COMMAND_DONE;
    RFC_DBELL->RFCPEISL &= ~RFC_DBELL_RFCPEISL_LAST_COMMAND_DONE;

    RFC_DBELL->RFCPEIFG = 0;
    NVIC_EnableIRQ(RFCCPE0_IRQn);
    NVIC_SetPriority(RFCCPE0_IRQn, 3);
    kirq_register(KERNEL_HANDLE, RFCCPE0_IRQn, ti_rf_cpe0_isr, exo);

    // ---> FW_INFO ---> CONFIGURED
    ti_rf_imm_cmd(exo, RF_CMD_GET_FW_INFO, BUF_ALLOC(exo, sizeof(RF_CMD_GET_FW_INFO_TYPE)), process);
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

    kheap_destroy(exo->rf.heap);
}

static inline void ti_rf_power_up(EXO* exo, HANDLE process)
{
    if (exo->rf.state != RF_STATE_CONFIGURED)
    {
        error(ERROR_INVALID_STATE);
        return;
    }

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
    tx_power = BUF_ALLOC(exo, sizeof(RF_CMD_SET_TX_POWER_TYPE));
    if (tx_power == NULL)
        return;
    tx_power->txPower = ti_rf_encode_tx_power(dbm);
    ti_rf_imm_cmd(exo, RF_CMD_SET_TX_POWER, tx_power, process);
    error(ERROR_SYNC);
}

static inline void ti_rf_test(EXO* exo)
{
    RF_CMD_COUNT_TYPE* count;
    if (exo->rf.state != RF_STATE_READY)
    {
        error(ERROR_INVALID_STATE);
        return;
    }

    //test counter
    count = ti_rf_prepare_radio_cmd(exo, sizeof(RF_CMD_COUNT_TYPE), RF_CMD_COUNT);
    if (count == NULL)
        return;
    count->counter = 1;
    ti_rf_radio_cmd(count);
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
    case IPC_RF_GET_RSSI:
        //TODO: used for test receive
        ti_rf_test(exo);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}
