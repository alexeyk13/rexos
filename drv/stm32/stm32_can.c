/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_can.h"
#include "stm32_power.h"
#include "../../userspace/sys.h"
#include "error.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/stdio.h"
#include "../../userspace/stream.h"
#include "../../userspace/irq.h"
#include "../../userspace/object.h"
#include <string.h>
#include "stm32_core_private.h"

#define CAN_BS1_1tq (0UL << 16)
#define CAN_BS1_2tq (1UL << 16)
#define CAN_BS1_3tq (2UL << 16)
#define CAN_BS1_4tq (3UL << 16)
#define CAN_BS1_5tq (4UL << 16)
#define CAN_BS1_6tq (5UL << 16)
#define CAN_BS1_7tq (6UL << 16)
#define CAN_BS1_8tq (7UL << 16)
#define CAN_BS1_9tq (8UL << 16)
#define CAN_BS1_10tq (9UL << 16)
#define CAN_BS1_11tq (10UL << 16)

#define CAN_BS2_1tq (0UL << 20)
#define CAN_BS2_2tq (1UL << 20)
#define CAN_BS2_3tq (2UL << 20)
#define CAN_BS2_4tq (3UL << 20)
#define CAN_BS2_5tq (4UL << 20)
#define CAN_BS2_6tq (5UL << 20)
#define CAN_BS2_7tq (6UL << 20)
#define CAN_BS2_8tq (7UL << 20)

#define CAN_SJW_1tq (0UL << 24)

static inline uint32_t stm32_can_tir_encode(uint32_t param1)
{
    return (param1 & 0xFFE00000) | ((param1 >> 3) & 0x02) | CAN_TI0R_TXRQ;
}

static inline void stm32_can_tx_flush(CORE* core)
{
    core->can.queue.head = core->can.queue.tail = 0;
}

static inline void stm32_can_change_state_isr(CORE* core, CAN_IPCS cmd)
{
    IPC ipc;
    ipc.process = core->can.device;
    ipc.cmd = HAL_CMD(HAL_CAN, cmd);
    ipc.param2 = core->can.error;
    ipc.param3 = core->can.state;
    ipc_ipost(&ipc);
}

static inline void stm32_can_on_rx_isr(CORE* core)
{
    IPC ipc;
    ipc.process = core->can.device;
    ipc.cmd = HAL_CMD(HAL_CAN, IPC_READ);
    ipc.param1 = (CAN->sFIFOMailBox[0].RDTR & 0x0F) | (CAN->sFIFOMailBox[0].RIR & 0xFFE00000)
            | ((CAN->sFIFOMailBox[0].RIR << 3) & 0x10);
    ipc.param2 = CAN->sFIFOMailBox[0].RDHR;
    ipc.param3 = CAN->sFIFOMailBox[0].RDLR;
    CAN->RF0R |= CAN_RF0R_RFOM0;
    core->can.error = OK;
    ipc_ipost(&ipc);
}

static inline void stm32_can_on_err_isr(CORE* core)
{
    CAN->MSR |= CAN_MSR_ERRI;
    if ((CAN->ESR & (CAN_ESR_EPVF | CAN_ESR_BOFF)) == CAN_ESR_EPVF)
        core->can.state = PASSIVE_ERR;
    if ((CAN->ESR & CAN_ESR_BOFF) && (core->can.state != BUSOFF))
    {

        core->can.state = BUSOFF;
        if ((CAN->TSR & CAN_TSR_TME0) == 0)
        {
            CAN->TSR |= CAN_TSR_ABRQ0;
        } else
        {
            stm32_can_change_state_isr(core, IPC_CAN_BUS_ERROR);
        }
    }
    if (CAN->TSR & (CAN_TSR_ALST0 | CAN_TSR_TERR0))
        ++core->can.tx_err_cnt;
    if (core->can.tx_err_cnt >= core->can.max_tx_err)
    {
        CAN->TSR |= CAN_TSR_ABRQ0;
    }
}

static inline void stm32_can_on_txc_isr(CORE* core)
{
    core->can.tx_err_cnt = 0;
    if (CAN->TSR & CAN_TSR_TXOK0)
    {
        core->can.error = OK;
        if(core->can.queue.head != core->can.queue.tail){
            CAN_TX_ENTRY* ptr = &core->can.queue.entry[core->can.queue.tail];
            core->can.tx_err_cnt = 0;
            CAN->sTxMailBox[0].TDLR = ptr->param3;
            CAN->sTxMailBox[0].TDHR = ptr->param2;
            CAN->sTxMailBox[0].TDTR = ptr->param1 & 0x0F;
            CAN->sTxMailBox[0].TIR = stm32_can_tir_encode(ptr->param1);

            core->can.queue.tail++;
            if(core->can.queue.tail >= TX_QUEUE_LEN)
                core->can.queue.tail = 0;
            return; // IPC only after last write
        }
     } else {
        core->can.error = ARBITRATION_LOST;
        stm32_can_tx_flush(core);
        if (core->can.state == BUSOFF)
            core->can.error = BUSOFF_ABORT;
    }
    stm32_can_change_state_isr(core, IPC_CAN_TXC);
    CAN->TSR |= CAN_TSR_RQCP0;
}

void stm32_can_on_isr(int vector, void* param)
{
    CORE* core = (CORE*)param;
    if (CAN->TSR & CAN_TSR_RQCP0) // transmit complete
    {
        stm32_can_on_txc_isr(core);
    }
    if ((CAN->RF0R & CAN_RF0R_FMP0) != 0) // FIFO not empty
    {
        stm32_can_on_rx_isr(core);
    }
    if (CAN->MSR & CAN_MSR_ERRI) // bus error
    {
        stm32_can_on_err_isr(core);
    }
}

static uint32_t stm32_can_decode_baudrate(CORE* core, uint32_t baud)
{
    uint32_t bit_time, clock, brr;
    bit_time = 3 + ((CAN->BTR >> 24) & 0x03) + ((CAN->BTR >> 20) & 0x07) + ((CAN->BTR >> 16) & 0x0F);
    if (baud == 0)
        baud = 1000000;
    clock = stm32_power_get_clock_inside(core, STM32_CLOCK_APB1);
    brr = 2 * clock / (baud * bit_time);
    return ((brr >> 1) - 1) & CAN_BTR_BRP;
}

static inline void stm32_can_set_baudrate(CORE* core, uint32_t baudrate)
{
    if (core->can.state == NO_INIT)
    {
        error (ERROR_NOT_CONFIGURED);
        return;
    }
    CAN->TSR |= CAN_TSR_ABRQ0;
    CAN->MCR |= CAN_MCR_INRQ;
    while ((CAN->MSR & CAN_MSR_INAK) != CAN_MSR_INAK)
        ;
    CAN->BTR = (CAN->BTR & ~CAN_BTR_BRP) | stm32_can_decode_baudrate(core, baudrate);
    CAN->MCR &= ~CAN_MCR_INRQ; // enter norm mode
    core->can.state = INIT;
}

static inline void stm32_can_open(CORE* core, HANDLE device, uint32_t baudrate)
{
    core->can.device = device;
    stm32_can_tx_flush(core);
    if (core->can.state != NO_INIT)
    {
        error (ERROR_ALREADY_CONFIGURED);
        return;
    }
    RCC->APB1ENR |= RCC_APB1ENR_CANEN;

    CAN->MCR |= CAN_MCR_INRQ; // enter init mode
    while ((CAN->MSR & CAN_MSR_INAK) != CAN_MSR_INAK)
        ;
    CAN->MCR = CAN_MCR_INRQ | CAN_MCR_ABOM;
    CAN->BTR = CAN_BS1_6tq | CAN_BS2_5tq; // set baudrate
    CAN->BTR = (CAN->BTR & ~CAN_BTR_BRP) | stm32_can_decode_baudrate(core, baudrate);
    CAN->MCR &= ~CAN_MCR_INRQ; // enter norm mode
// set receive filter
    CAN->FMR |= CAN_FMR_FINIT;
    CAN->FA1R = CAN_FA1R_FACT0;
    CAN->sFilterRegister[0].FR1 = 0; // receive all
    CAN->sFilterRegister[0].FR2 = 0;
    CAN->FMR &= ~CAN_FMR_FINIT;

    CAN->IER = (CAN_IER_ERRIE | CAN_IER_BOFIE | CAN_IER_LECIE) | CAN_IER_FMPIE0 | CAN_IER_TMEIE;

//enable interrupts
    irq_register(CEC_CAN_IRQn, stm32_can_on_isr, (void*)core);
    NVIC_EnableIRQ (CEC_CAN_IRQn);
    NVIC_SetPriority(CEC_CAN_IRQn, 13);
    core->can.state = INIT;
}

static inline void stm32_can_close(CORE* core)
{
    if (core->can.state == NO_INIT)
    {
        error (ERROR_NOT_CONFIGURED);
        return;
    }

    NVIC_DisableIRQ (CEC_CAN_IRQn);
    irq_unregister(CEC_CAN_IRQn);

    CAN->TSR |= CAN_TSR_ABRQ0;
    CAN->MCR |= CAN_MCR_INRQ;
    while ((CAN->MSR & CAN_MSR_INAK) != CAN_MSR_INAK)
        ;
    RCC->APB1ENR &= ~RCC_APB1ENR_CANEN;
    core->can.state = NO_INIT;
}

static inline void stm32_can_check_state(CORE* core)
{
    switch (core->can.state)
    {
    case NO_INIT:
        error (ERROR_NOT_CONFIGURED);
        core->can.error = ERROR_OK;
        break;
    case INIT:
        if ((CAN->MSR & CAN_MSR_INAK) == 0)
            core->can.state = NORMAL;
        break;
    case PASSIVE_ERR:
        if ((CAN->ESR & CAN_ESR_EPVF) == 0)
            core->can.state = NORMAL;
        break;
    case BUSOFF:
        if ((CAN->ESR & CAN_ESR_BOFF) == 0)
            core->can.state = NORMAL;
        break;
    default:
        break;
    }
    if (core->can.state == NORMAL)
    {
        if (CAN->ESR & CAN_ESR_EPVF)
            core->can.state = PASSIVE_ERR;
    }
}

static inline void stm32_can_clear_error(CORE* core)
{
    if (core->can.state == NO_INIT)
    {
        error (ERROR_NOT_CONFIGURED);
        return;
    }
    core->can.error = OK;
}

void stm32_can_init_write(CORE* core, IPC* ipc)
{
   uint32_t next_pos;
    if ((core->can.state < CAN_TRANSMIT))
    {
        core->can.error = ILLEGAL_STATE;
        return;
    }

    if ((CAN->TSR & CAN_TSR_TME0) == 0)
    {
        next_pos = core->can.queue.head+1;
        if(next_pos >= TX_QUEUE_LEN)
            next_pos = 0;
        if(next_pos != core->can.queue.tail)
        {
            core->can.queue.entry[core->can.queue.head].param1 = ipc->param1;
                    core->can.queue.entry[core->can.queue.head].param2 = ipc->param2;
                    core->can.queue.entry[core->can.queue.head].param3 = ipc->param3;
            core->can.queue.head = next_pos;
        }else{                    // queue full
            core->can.error = BUSY;
            return;
         }
    }// mailbox is not empty
    core->can.tx_err_cnt = 0;
    CAN->sTxMailBox[0].TDLR = ipc->param3;
    CAN->sTxMailBox[0].TDHR = ipc->param2;
    CAN->sTxMailBox[0].TDTR = ipc->param1 & 0x0F;
    CAN->sTxMailBox[0].TIR = stm32_can_tir_encode(ipc->param1);
    core->can.error = OK;
}

void stm32_can_request(CORE* core, IPC* ipc)
{
    stm32_can_check_state(core);
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        stm32_can_open(core, ipc->process, ipc->param1);
        break;
    case IPC_WRITE:
        stm32_can_init_write(core, ipc);
        break;
    case IPC_CLOSE:
        stm32_can_close(core);
        break;
    case IPC_CAN_SET_BAUDRATE:
        stm32_can_set_baudrate(core, ipc->param1);
        break;
    case IPC_CAN_GET_STATE:
        break;
    case IPC_CAN_CLEAR_ERROR:
        stm32_can_clear_error(core);
        break;
    default:
        error (ERROR_NOT_SUPPORTED);
        break;
    }
    ipc->param2 = core->can.error;
    ipc->param3 = core->can.state;
}

void stm32_can_init(CORE *core)
{
    core->can.error = OK;
    core->can.state = NO_INIT;
    core->can.tx_err_cnt = 0;
    core->can.max_tx_err = MAX_TX_ERR_CNT;
}

