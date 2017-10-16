/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "ti_rf.h"
#include "ti_exo_private.h"
#include "../../userspace/process.h"
#include "../kerror.h"
#include "../kstdlib.h"
#include "../kirq.h"
#include "../kheap.h"
#include <stdint.h>
#include <string.h>
#include "rf_common_cmd.h"
#include "rf_mailbox.h"
#include "rf_ble_cmd.h"
#include "rf_data_entry.h"

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
    rfc_radioOp_t* hdr = BUF_ALLOC(exo, size);
    if (hdr == NULL)
        return NULL;
    memset(hdr, 0x00, size);
    hdr->commandNo = cmd;
    hdr->condition.rule = COND_NEVER;
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
    ((rfc_command_t*)buf)->commandNo = cmd;
    RFC_DBELL->CMDR = RF_RADIO_IMM_CMD(buf);
}

static inline void ti_rf_fw_info_fsm(EXO* exo)
{
    rfc_CMD_GET_FW_INFO_t* fw_info;

    exo->rf.cmd = 0;
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
    rfc_CMD_RADIO_SETUP_t* setup;
    void* rfe_patch;
    setup = ti_rf_prepare_radio_cmd(exo, ((sizeof(rfc_CMD_RADIO_SETUP_t) + 3) & ~3) + RFE_PATCH_WORDS * sizeof(uint32_t), CMD_RADIO_SETUP);
    //RFE patch must be located somewhere in RAM. It's not documented by TRM, but generating error by CPE if it's in flash.
    rfe_patch = (void*)(((uint32_t)(setup) + sizeof(rfc_CMD_RADIO_SETUP_t) + 3) & ~3);
    memcpy(rfe_patch, __RFE_PATCH, RFE_PATCH_WORDS * sizeof(uint32_t));

    setup->mode = 0;
    setup->txPower = ti_rf_encode_tx_power(0);
    setup->pRegOverride = rfe_patch;

    ti_rf_radio_cmd(setup);
}

static inline void ti_rf_set_tx_power_fsm(EXO* exo)
{
    exo->rf.cmd = 0;
    kheap_free(exo->rf.cur);
    ipc_ipost_inline(exo->rf.process, HAL_CMD(HAL_RF, IPC_RF_SET_TX_POWER), 0, 0, 0);
}

static void ti_rf_cmd_ack_isr(int vector, void* param)
{
    EXO* exo = param;
    RFC_DBELL->RFACKIFG &= ~RFC_DBELL_RFACKIFG_ACKFLAG;

    if ((RFC_DBELL->CMDSTA & 0xff) == CMDSTA_Done)
    //normal processing
    {
        switch (exo->rf.cmd)
        {
        case CMD_GET_FW_INFO:
            ti_rf_fw_info_fsm(exo);
            break;
        case CMD_START_RAT:
            ti_rf_start_rat_fsm(exo);
            break;
        case CMD_SET_TX_POWER:
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
        case CMD_START_RAT:
            break;
        default:
            kheap_free(exo->rf.cur);
        }

#if (TI_RF_DEBUG_ERRORS)
        printk("RF CMD error: %x\n", RFC_DBELL->CMDSTA & 0xff);
#endif //(TI_RF_DEBUG_ERRORS

        switch (exo->rf.cmd)
        {
        case CMD_GET_FW_INFO:
            ipc_ipost_inline(exo->rf.process, HAL_CMD(HAL_RF, IPC_OPEN), 0, 0, ERROR_HARDWARE);
            break;
        case CMD_START_RAT:
        case CMD_RADIO_SETUP:
            ipc_ipost_inline(exo->rf.process, HAL_CMD(HAL_RF, IPC_RF_POWER_UP), 0, 0, ERROR_HARDWARE);
            break;
        case CMD_SET_TX_POWER:
            ipc_ipost_inline(exo->rf.process, HAL_CMD(HAL_RF, IPC_RF_SET_TX_POWER), 0, 0, ERROR_HARDWARE);
            break;
        default:
            break;
        }
        exo->rf.cmd = 0;
    }
}

static inline void ti_rf_radio_up_fsm(EXO* exo)
{
    kheap_free(exo->rf.cur);
    exo->rf.cmd = 0;
    exo->rf.state = RF_STATE_READY;
    ipc_ipost_inline(exo->rf.process, HAL_CMD(HAL_RF, IPC_RF_POWER_UP), 0, 0, 0);
}

static void ti_rf_cpe0_isr(int vector, void* param)
{
    EXO* exo = param;
    uint16_t status;
    RFC_DBELL->RFCPEIFG = ~RFC_DBELL_RFCPEIFG_LAST_COMMAND_DONE;

    status = ((rfc_radioOp_t*)exo->rf.cur)->status;
    if (status < 0x800)
    //normal processing
    {
        switch (exo->rf.cmd)
        {
        case CMD_RADIO_SETUP:
            ti_rf_radio_up_fsm(exo);
            break;
        default:
#if (TI_RF_DEBUG_ERRORS)
            printk("RF unexpected cmd: %x\n", exo->rf.cmd);
#endif //(TI_RF_DEBUG_ERRORS
            kheap_free(exo->rf.cur);
            exo->rf.cmd = 0;
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
        case CMD_RADIO_SETUP:
            ipc_ipost_inline(exo->rf.process, HAL_CMD(HAL_RF, IPC_RF_POWER_UP), 0, 0, ERROR_HARDWARE);
            break;
        default:
            break;
        }
        exo->rf.cmd = 0;
    }
}

static void ti_rf_cpe1_isr(int vector, void* param)
{
    EXO* exo = param;
    //TODO:
    printk("DTA CPE1!!!! %08x\n", RFC_DBELL->RFCPEIFG);
    RFC_DBELL->RFACKIFG = 0;
//    RFC_DBELL->RFACKIFG = ~0xfff0000ul;
//    dump(exo->rf.cur, 128);
}

void ti_rf_init(EXO* exo)
{
    exo->rf.state = RF_STATE_IDLE;
    exo->rf.cmd = 0;
}

static inline void ti_rf_open(EXO* exo, HANDLE process)
{
    if (exo->rf.state != RF_STATE_IDLE)
        return;

    exo->rf.heap = kheap_create(TI_RF_HEAP_SIZE);

    //very poorly documented.
    PRCM->RFCMODESEL = RF_MODE_BLE;

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

    //CPE1 - data flow
    RFC_DBELL->RFCPEIEN |= RFC_DBELL_RFCPEIEN_RX_ABORTED | RFC_DBELL_RFCPEIEN_RX_N_DATA_WRITTEN | RFC_DBELL_RFCPEIEN_RX_DATA_WRITTEN |
                           RFC_DBELL_RFCPEIEN_RX_ENTRY_DONE | RFC_DBELL_RFCPEIEN_RX_BUF_FULL | RFC_DBELL_RFCPEIEN_RX_CTRL_ACK |
                           RFC_DBELL_RFCPEIEN_RX_CTRL | RFC_DBELL_RFCPEIEN_RX_EMPTY | RFC_DBELL_RFCPEIEN_RX_IGNORED |
                           RFC_DBELL_RFCPEIEN_RX_NOK | RFC_DBELL_RFCPEIEN_RX_OK;

    RFC_DBELL->RFCPEISL  = RFC_DBELL_RFCPEISL_RX_ABORTED | RFC_DBELL_RFCPEISL_RX_N_DATA_WRITTEN | RFC_DBELL_RFCPEISL_RX_DATA_WRITTEN |
                           RFC_DBELL_RFCPEISL_RX_ENTRY_DONE | RFC_DBELL_RFCPEISL_RX_BUF_FULL | RFC_DBELL_RFCPEISL_RX_CTRL_ACK |
                           RFC_DBELL_RFCPEISL_RX_CTRL | RFC_DBELL_RFCPEISL_RX_EMPTY | RFC_DBELL_RFCPEISL_RX_IGNORED |
                           RFC_DBELL_RFCPEISL_RX_NOK | RFC_DBELL_RFCPEISL_RX_OK;

    RFC_DBELL->RFCPEIFG = 0;

    kirq_register(KERNEL_HANDLE, RFCCPE0_IRQn, ti_rf_cpe0_isr, exo);
    NVIC_EnableIRQ(RFCCPE0_IRQn);
    NVIC_SetPriority(RFCCPE0_IRQn, 3);

    kirq_register(KERNEL_HANDLE, RFCCPE1_IRQn, ti_rf_cpe1_isr, exo);
    NVIC_EnableIRQ(RFCCPE1_IRQn);
    NVIC_SetPriority(RFCCPE1_IRQn, 2);

    // ---> FW_INFO ---> CONFIGURED
    ti_rf_imm_cmd(exo, CMD_GET_FW_INFO, BUF_ALLOC(exo, sizeof(rfc_CMD_GET_FW_INFO_t)), process);
    kerror(ERROR_SYNC);
}

static inline void ti_rf_close(EXO* exo)
{
    NVIC_DisableIRQ(RFCCPE0_IRQn);
    kirq_unregister(KERNEL_HANDLE, RFCCPE0_IRQn);

    NVIC_DisableIRQ(RFCCPE1_IRQn);
    kirq_unregister(KERNEL_HANDLE, RFCCPE1_IRQn);

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
        kerror(ERROR_INVALID_STATE);
        return;
    }

    // CONFIGURED ---> Start RAT ---> Radio Setup ---> READY
    ti_rf_direct_cmd(exo, CMD_START_RAT, process);
    kerror(ERROR_SYNC);
}

static inline void ti_rf_set_tx_power(EXO* exo, int dbm, HANDLE process)
{
    rfc_CMD_SET_TX_POWER_t* tx_power;
    if (exo->rf.state != RF_STATE_READY)
    {
        kerror(ERROR_INVALID_STATE);
        return;
    }

    // READY ---> Set TX Power ---> READY
    tx_power = BUF_ALLOC(exo, sizeof(rfc_CMD_SET_TX_POWER_t));
    if (tx_power == NULL)
        return;
    tx_power->txPower = ti_rf_encode_tx_power(dbm);
    ti_rf_imm_cmd(exo, CMD_SET_TX_POWER, tx_power, process);
    kerror(ERROR_SYNC);
}

static inline void ti_rf_test(EXO* exo)
{
    rfc_CMD_BLE_ADV_NC_t* cmd;
    rfc_bleAdvPar_t* params;
    rfc_bleAdvOutput_t* out;
    uint8_t* adv_data;
    uint8_t* dev_addr;
    if (exo->rf.state != RF_STATE_READY)
    {
        kerror(ERROR_INVALID_STATE);
        return;
    }

    //test packet rx
    cmd = ti_rf_prepare_radio_cmd(exo, sizeof(rfc_CMD_BLE_ADV_NC_t) + sizeof(rfc_bleAdvPar_t) + sizeof(rfc_bleAdvOutput_t) +
                                  6 + 6, CMD_BLE_ADV_NC);
    if (cmd == NULL)
        return;
    params = (rfc_bleAdvPar_t*)(((uint8_t*)cmd) + sizeof(rfc_CMD_BLE_ADV_NC_t));
    out = (rfc_bleAdvOutput_t*)(((uint8_t*)params) + sizeof(rfc_bleAdvPar_t));
    adv_data = (uint8_t*)out + sizeof(rfc_bleAdvOutput_t);
    dev_addr = (uint8_t*)adv_data + sizeof(rfc_bleAdvOutput_t);

    cmd->channel = 37;
    cmd->pParams = params;
    cmd->pOutput = out;


    params->pRxQ = NULL;
    params->advLen = 6;
    params->pAdvData = adv_data;
    adv_data[0] = 't';
    adv_data[1] = 'e';
    adv_data[2] = 's';
    adv_data[3] = 't';
    adv_data[4] = ' ';
    adv_data[5] = '1';

    params->pDeviceAddress = (uint16_t*)dev_addr;
    dev_addr[0] = 0x10;
    dev_addr[1] = 0x12;
    dev_addr[2] = 0x13;
    dev_addr[3] = 0x14;
    dev_addr[4] = 0x15;
    dev_addr[5] = 0x16;

    params->endTrigger.triggerType = TRIG_NEVER;

    ti_rf_radio_cmd(cmd);
}

void ti_rf_request(EXO* exo, IPC* ipc)
{
    if (exo->rf.cmd != 0)
    {
        kerror(ERROR_IN_PROGRESS);
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
        kerror(ERROR_NOT_SUPPORTED);
    }
}
