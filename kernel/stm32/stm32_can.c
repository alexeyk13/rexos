/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_can.h"
#include "stm32_power.h"
#include "../../userspace/sys.h"
#include "../kerror.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/stdio.h"
#include "../../userspace/stream.h"
#include "../kirq.h"
#include "../../userspace/object.h"
#include <string.h>
#include "stm32_exo_private.h"

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

static inline void stm32_can_tx_flush(EXO* exo)
{
    exo->can.queue.head = exo->can.queue.tail = 0;
}

static inline void stm32_can_change_state_isr(EXO* exo, CAN_IPCS cmd)
{
    IPC ipc;
    ipc.process = exo->can.device;
    ipc.cmd = HAL_CMD(HAL_CAN, cmd);
    ipc.param2 = exo->can.error;
    ipc.param3 = exo->can.state;
    ipc_ipost(&ipc);
}

static inline void stm32_can_on_rx_isr(EXO* exo)
{
    IPC ipc;
    ipc.process = exo->can.device;
    ipc.cmd = HAL_CMD(HAL_CAN, IPC_READ);
    ipc.param1 = (CAN->sFIFOMailBox[0].RDTR & 0x0F) | (CAN->sFIFOMailBox[0].RIR & 0xFFE00000)
            | ((CAN->sFIFOMailBox[0].RIR << 3) & 0x10);
    ipc.param2 = CAN->sFIFOMailBox[0].RDHR;
    ipc.param3 = CAN->sFIFOMailBox[0].RDLR;
    CAN->RF0R |= CAN_RF0R_RFOM0;
    exo->can.error = OK;
    ipc_ipost(&ipc);
}

static inline void stm32_can_on_err_isr(EXO* exo)
{
    CAN->MSR |= CAN_MSR_ERRI;
    if ((CAN->ESR & (CAN_ESR_EPVF | CAN_ESR_BOFF)) == CAN_ESR_EPVF)
        exo->can.state = PASSIVE_ERR;
    if ((CAN->ESR & CAN_ESR_BOFF) && (exo->can.state != BUSOFF))
    {

        exo->can.state = BUSOFF;
        if ((CAN->TSR & CAN_TSR_TME0) == 0)
        {
            CAN->TSR |= CAN_TSR_ABRQ0;
        } else
        {
            stm32_can_change_state_isr(exo, IPC_CAN_BUS_ERROR);
        }
    }
    if (CAN->TSR & (CAN_TSR_ALST0 | CAN_TSR_TERR0))
        ++exo->can.tx_err_cnt;
    if (exo->can.tx_err_cnt >= exo->can.max_tx_err)
    {
        CAN->TSR |= CAN_TSR_ABRQ0;
    }
}

static inline void stm32_can_on_txc_isr(EXO* exo)
{
    exo->can.tx_err_cnt = 0;
    if (CAN->TSR & CAN_TSR_TXOK0)
    {
        exo->can.error = OK;
        if(exo->can.queue.head != exo->can.queue.tail){
            CAN_TX_ENTRY* ptr = &exo->can.queue.entry[exo->can.queue.tail];
            exo->can.tx_err_cnt = 0;
            CAN->sTxMailBox[0].TDLR = ptr->param3;
            CAN->sTxMailBox[0].TDHR = ptr->param2;
            CAN->sTxMailBox[0].TDTR = ptr->param1 & 0x0F;
            CAN->sTxMailBox[0].TIR = stm32_can_tir_encode(ptr->param1);

            exo->can.queue.tail++;
            if(exo->can.queue.tail >= TX_QUEUE_LEN)
                exo->can.queue.tail = 0;
            return; // IPC only after last write
        }
     } else {
        exo->can.error = ARBITRATION_LOST;
        stm32_can_tx_flush(exo);
        if (exo->can.state == BUSOFF)
            exo->can.error = BUSOFF_ABORT;
    }
    stm32_can_change_state_isr(exo, IPC_CAN_TXC);
    CAN->TSR |= CAN_TSR_RQCP0;
}

void stm32_can_on_isr(int vector, void* param)
{
    EXO* exo = (EXO*)param;
    if (CAN->TSR & CAN_TSR_RQCP0) // transmit complete
    {
        stm32_can_on_txc_isr(exo);
    }
    if ((CAN->RF0R & CAN_RF0R_FMP0) != 0) // FIFO not empty
    {
        stm32_can_on_rx_isr(exo);
    }
    if (CAN->MSR & CAN_MSR_ERRI) // bus error
    {
        stm32_can_on_err_isr(exo);
    }
}

static uint32_t stm32_can_decode_baudrate(EXO* exo, uint32_t baud)
{
    uint32_t bit_time, clock, brr;
    bit_time = 3 + ((CAN->BTR >> 24) & 0x03) + ((CAN->BTR >> 20) & 0x07) + ((CAN->BTR >> 16) & 0x0F);
    if (baud == 0)
        baud = 1000000;
    clock = stm32_power_get_clock_inside(exo, STM32_CLOCK_APB1);
    brr = 2 * clock / (baud * bit_time);
    return ((brr >> 1) - 1) & CAN_BTR_BRP;
}

static inline void stm32_can_set_baudrate(EXO* exo, uint32_t baudrate)
{
    if (exo->can.state == NO_INIT)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    CAN->TSR |= CAN_TSR_ABRQ0;
    CAN->MCR |= CAN_MCR_INRQ;
    while ((CAN->MSR & CAN_MSR_INAK) != CAN_MSR_INAK)
        ;
    CAN->BTR = (CAN->BTR & ~CAN_BTR_BRP) | stm32_can_decode_baudrate(exo, baudrate);
    CAN->MCR &= ~CAN_MCR_INRQ; // enter norm mode
    exo->can.state = INIT;
}

static inline void stm32_can_open(EXO* exo, HANDLE device, uint32_t baudrate)
{
    exo->can.device = device;
    stm32_can_tx_flush(exo);
    if (exo->can.state != NO_INIT)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }
    RCC->APB1ENR |= RCC_APB1ENR_CANEN;

    CAN->MCR |= CAN_MCR_INRQ; // enter init mode
    while ((CAN->MSR & CAN_MSR_INAK) != CAN_MSR_INAK)
        ;
    CAN->MCR = CAN_MCR_INRQ | CAN_MCR_ABOM;
    CAN->BTR = CAN_BS1_6tq | CAN_BS2_5tq; // set baudrate
    CAN->BTR = (CAN->BTR & ~CAN_BTR_BRP) | stm32_can_decode_baudrate(exo, baudrate);
    CAN->MCR &= ~CAN_MCR_INRQ; // enter norm mode
// set receive filter
    CAN->FMR |= CAN_FMR_FINIT;
    CAN->FA1R = CAN_FA1R_FACT0;
    CAN->sFilterRegister[0].FR1 = 0; // receive all
    CAN->sFilterRegister[0].FR2 = 0;
    CAN->FMR &= ~CAN_FMR_FINIT;

    CAN->IER = (CAN_IER_ERRIE | CAN_IER_BOFIE | CAN_IER_LECIE) | CAN_IER_FMPIE0 | CAN_IER_TMEIE;

//enable interrupts
    kirq_register(KERNEL_HANDLE, CEC_CAN_IRQn, stm32_can_on_isr, (void*)exo);
    NVIC_EnableIRQ (CEC_CAN_IRQn);
    NVIC_SetPriority(CEC_CAN_IRQn, 13);
    exo->can.state = INIT;
}

static inline void stm32_can_close(EXO* exo)
{
    if (exo->can.state == NO_INIT)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }

    NVIC_DisableIRQ (CEC_CAN_IRQn);
    kirq_unregister(KERNEL_HANDLE, CEC_CAN_IRQn);

    CAN->TSR |= CAN_TSR_ABRQ0;
    CAN->MCR |= CAN_MCR_INRQ;
    while ((CAN->MSR & CAN_MSR_INAK) != CAN_MSR_INAK)
        ;
    RCC->APB1ENR &= ~RCC_APB1ENR_CANEN;
    exo->can.state = NO_INIT;
}

static inline void stm32_can_check_state(EXO* exo)
{
    switch (exo->can.state)
    {
    case NO_INIT:
        kerror(ERROR_NOT_CONFIGURED);
        exo->can.error = ERROR_OK;
        break;
    case INIT:
        if ((CAN->MSR & CAN_MSR_INAK) == 0)
            exo->can.state = NORMAL;
        break;
    case PASSIVE_ERR:
        if ((CAN->ESR & CAN_ESR_EPVF) == 0)
            exo->can.state = NORMAL;
        break;
    case BUSOFF:
        if ((CAN->ESR & CAN_ESR_BOFF) == 0)
            exo->can.state = NORMAL;
        break;
    default:
        break;
    }
    if (exo->can.state == NORMAL)
    {
        if (CAN->ESR & CAN_ESR_EPVF)
            exo->can.state = PASSIVE_ERR;
    }
}

static inline void stm32_can_clear_error(EXO* exo)
{
    if (exo->can.state == NO_INIT)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    exo->can.error = OK;
}

void stm32_can_init_write(EXO* exo, IPC* ipc)
{
   uint32_t next_pos;
    if ((exo->can.state < CAN_TRANSMIT))
    {
        exo->can.error = ILLEGAL_STATE;
        return;
    }

    if ((CAN->TSR & CAN_TSR_TME0) == 0)
    {
        next_pos = exo->can.queue.head+1;
        if(next_pos >= TX_QUEUE_LEN)
            next_pos = 0;
        if(next_pos != exo->can.queue.tail)
        {
            exo->can.queue.entry[exo->can.queue.head].param1 = ipc->param1;
                    exo->can.queue.entry[exo->can.queue.head].param2 = ipc->param2;
                    exo->can.queue.entry[exo->can.queue.head].param3 = ipc->param3;
            exo->can.queue.head = next_pos;
        }else{                    // queue full
            exo->can.error = BUSY;
            return;
         }
    }// mailbox is not empty
    exo->can.tx_err_cnt = 0;
    CAN->sTxMailBox[0].TDLR = ipc->param3;
    CAN->sTxMailBox[0].TDHR = ipc->param2;
    CAN->sTxMailBox[0].TDTR = ipc->param1 & 0x0F;
    CAN->sTxMailBox[0].TIR = stm32_can_tir_encode(ipc->param1);
    exo->can.error = OK;
}

void stm32_can_request(EXO* exo, IPC* ipc)
{
    stm32_can_check_state(exo);
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        stm32_can_open(exo, ipc->process, ipc->param1);
        break;
    case IPC_WRITE:
        stm32_can_init_write(exo, ipc);
        break;
    case IPC_CLOSE:
        stm32_can_close(exo);
        break;
    case IPC_CAN_SET_BAUDRATE:
        stm32_can_set_baudrate(exo, ipc->param1);
        break;
    case IPC_CAN_GET_STATE:
        break;
    case IPC_CAN_CLEAR_ERROR:
        stm32_can_clear_error(exo);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
    ipc->param2 = exo->can.error;
    ipc->param3 = exo->can.state;
}

void stm32_can_init(EXO* exo)
{
    exo->can.error = OK;
    exo->can.state = NO_INIT;
    exo->can.tx_err_cnt = 0;
    exo->can.max_tx_err = MAX_TX_ERR_CNT;
}

