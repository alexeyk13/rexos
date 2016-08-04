/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_usb.h"
#include "../../userspace/sys.h"
#include "../../userspace/usb.h"
#include "../../userspace/irq.h"
#include "../../userspace/stm32/stm32_driver.h"
#include "stm32_power.h"
#include <string.h>
#include "../../userspace/stdlib.h"
#include "../../userspace/stdio.h"
#include "stm32_core_private.h"

typedef struct {
  HANDLE process;
  HANDLE device;
} USB_STRUCT;

typedef enum {
    STM32_USB_ERROR = USB_HAL_MAX,
    STM32_USB_OVERFLOW
}STM32_USB_IPCS;

#pragma pack(push, 1)

typedef struct {
    uint16_t ADDR_TX;
    uint16_t COUNT_TX;
    uint16_t ADDR_RX;
    uint16_t COUNT_RX;
} USB_BUFFER_DESCRIPTOR;

#define USB_BUFFER_DESCRIPTORS                      ((USB_BUFFER_DESCRIPTOR*) USB_PMAADDR)

#pragma pack(pop)

static inline uint16_t* ep_reg_data(unsigned int num)
{
    return (uint16_t*)(USB_BASE + USB_EP_NUM(num) * 4);
}

static inline EP* ep_data(CORE* core, unsigned int num)
{
    return (num & USB_EP_IN) ? (core->usb.in[USB_EP_NUM(num)]) : (core->usb.out[USB_EP_NUM(num)]);
}

static inline void ep_toggle_bits(unsigned int num, int mask, int value)
{
    __disable_irq();
    *ep_reg_data(num) = ((*ep_reg_data(num)) & USB_EPREG_MASK) | (((*ep_reg_data(num)) & mask) ^ value);
    __enable_irq();
}

static inline void memcpy16(void* dst, void* src, unsigned int size)
{
    int i;
    size = (size + 1) / 2;
    for (i = 0; i < size; ++i)
        ((uint16_t*)dst)[i] = ((uint16_t*)src)[i];
}

static void stm32_usb_tx(CORE* core, unsigned int ep_num)
{
    EP* ep = core->usb.in[ep_num];

    int size = ep->io->data_size - ep->size;
    if (size > ep->mps)
        size = ep->mps;
    USB_BUFFER_DESCRIPTORS[ep_num].COUNT_TX = size;
    memcpy16((void*)(USB_BUFFER_DESCRIPTORS[ep_num].ADDR_TX + USB_PMAADDR), io_data(ep->io) + ep->size, size);
}

bool stm32_usb_ep_flush(CORE* core, unsigned int num)
{
    EP* ep = ep_data(core, num);
    if (ep == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return false;
    }
    if (num & USB_EP_IN)
        ep_toggle_bits(num, USB_EPTX_STAT, USB_EP_TX_NAK);
    else
        ep_toggle_bits(num, USB_EPRX_STAT, USB_EP_RX_NAK);
    if (ep->io != NULL)
    {
        io_complete_ex(core->usb.device, HAL_IO_CMD(HAL_USB, (num & USB_EP_IN) ? IPC_WRITE : IPC_READ), USB_HANDLE(USB_0, num), ep->io, ERROR_IO_CANCELLED);
        ep->io = NULL;
    }
    ep->io_active = false;
    return true;
}

void stm32_usb_ep_set_stall(CORE* core, unsigned int num)
{
    if (!stm32_usb_ep_flush(core, num))
        return;
    if (USB_EP_IN & num)
        ep_toggle_bits(num, USB_EPTX_STAT, USB_EP_TX_STALL);
    else
        ep_toggle_bits(num, USB_EPRX_STAT, USB_EP_RX_STALL);
}

void stm32_usb_ep_clear_stall(CORE* core, unsigned int num)
{
    if (!stm32_usb_ep_flush(core, num))
        return;
    if (USB_EP_IN & num)
        ep_toggle_bits(num, USB_EPTX_STAT, USB_EP_TX_NAK);
    else
        ep_toggle_bits(num, USB_EPRX_STAT, USB_EP_RX_NAK);
}


bool stm32_usb_ep_is_stall(unsigned int num)
{
    if (USB_EP_IN & num)
        return ((*ep_reg_data(num)) & USB_EPTX_STAT) == USB_EP_TX_STALL;
    else
        return ((*ep_reg_data(num)) & USB_EPRX_STAT) == USB_EP_RX_STALL;
}

USB_SPEED stm32_usb_get_speed(CORE* core)
{
    //according to datasheet STM32L0 doesn't support low speed mode...
    return USB_FULL_SPEED;
}

static inline void stm32_usb_reset(CORE* core)
{
    USB->CNTR |= USB_CNTR_SUSPM;

    //enable function
    USB->DADDR = USB_DADDR_EF;

    IPC ipc;
    ipc.process = core->usb.device;
    ipc.cmd = HAL_CMD(HAL_USB, USB_RESET);
    ipc.param1 = USB_HANDLE(USB_0, USB_HANDLE_DEVICE);
    ipc.param2 = stm32_usb_get_speed(core);
    ipc_ipost(&ipc);
}

static inline void stm32_usb_suspend(CORE* core)
{
    IPC ipc;
    USB->CNTR &= ~USB_CNTR_SUSPM;
    ipc.process = core->usb.device;
    ipc.param1 = USB_HANDLE(USB_0, USB_HANDLE_DEVICE);
    ipc.cmd = HAL_CMD(HAL_USB, USB_SUSPEND);
    ipc_ipost(&ipc);
}

static inline void stm32_usb_wakeup(CORE* core)
{
    IPC ipc;
    USB->CNTR |= USB_CNTR_SUSPM;
    ipc.process = core->usb.device;
    ipc.param1 = USB_HANDLE(USB_0, USB_HANDLE_DEVICE);
    ipc.cmd = HAL_CMD(HAL_USB, USB_WAKEUP);
    ipc_ipost(&ipc);
}

static inline void stm32_usb_tx_isr(CORE* core, unsigned int ep_num)
{
    EP* ep = core->usb.in[ep_num];
    //handle STATUS in for set address
    if (core->usb.addr && USB_BUFFER_DESCRIPTORS[ep_num].COUNT_TX == 0)
    {
        USB->DADDR = USB_DADDR_EF | core->usb.addr;
        core->usb.addr = 0;
    }

    if (ep->io_active)
    {
        ep->size += USB_BUFFER_DESCRIPTORS[ep_num].COUNT_TX;
        if (ep->size >= ep->io->data_size)
        {
            ep_toggle_bits(ep_num, USB_EPTX_STAT, USB_EP_TX_NAK);
            iio_complete(core->usb.device, HAL_IO_CMD(HAL_USB, IPC_WRITE), USB_HANDLE(USB_0, USB_EP_IN | ep_num), ep->io);
            ep->io = NULL;
            ep->io_active = false;
        }
        else
        {
            stm32_usb_tx(core, ep_num);
            ep_toggle_bits(ep_num, USB_EPTX_STAT, USB_EP_TX_VALID);
        }
    }
    else
        ep_toggle_bits(ep_num, USB_EPTX_STAT, USB_EP_TX_NAK);
    *ep_reg_data(ep_num) = (*ep_reg_data(ep_num)) & USB_EPREG_MASK & ~USB_EP_CTR_TX;
}

static inline void stm32_usb_rx_isr(CORE* core, unsigned int ep_num)
{
    EP* ep = core->usb.out[ep_num];
    uint16_t size = USB_BUFFER_DESCRIPTORS[ep_num].COUNT_RX & 0x3ff;
    memcpy16(io_data(ep->io) + ep->io->data_size, (void*)(USB_BUFFER_DESCRIPTORS[ep_num].ADDR_RX + USB_PMAADDR), size);
    *ep_reg_data(ep_num) = (*ep_reg_data(ep_num)) & USB_EPREG_MASK & ~USB_EP_CTR_RX;
    ep->io->data_size += size;

    if (ep->io->data_size >= ep->size || size < ep->mps)
    {
        ep_toggle_bits(ep_num, USB_EPRX_STAT, USB_EP_RX_NAK);
        iio_complete(core->usb.device, HAL_IO_CMD(HAL_USB, IPC_READ), USB_HANDLE(USB_0, ep_num), ep->io);
        ep->io = NULL;
        ep->io_active = false;
    }
    else
        ep_toggle_bits(ep_num, USB_EPRX_STAT, USB_EP_RX_VALID);
}

static inline void stm32_usb_ctr(CORE* core)
{
    uint8_t num;
    IPC ipc;
    num = USB->ISTR & USB_ISTR_EP_ID;

    //got SETUP
    if (num == 0 && (*ep_reg_data(num)) & USB_EP_SETUP)
    {
        ipc.cmd = HAL_CMD(HAL_USB, USB_SETUP);
        ipc.process = core->usb.device;
        ipc.param1 = 0;
        memcpy16(&ipc.param2, (void*)(USB_BUFFER_DESCRIPTORS[0].ADDR_RX + USB_PMAADDR), 4);
        memcpy16(&ipc.param3, (void*)(USB_BUFFER_DESCRIPTORS[0].ADDR_RX + USB_PMAADDR + 4), 4);
        ipc_ipost(&ipc);
        *ep_reg_data(num) = (*ep_reg_data(num)) & USB_EPREG_MASK & ~USB_EP_CTR_RX;
    }

    if ((*ep_reg_data(num)) & USB_EP_CTR_TX)
        stm32_usb_tx_isr(core, num);

    if ((*ep_reg_data(num)) & USB_EP_CTR_RX)
        stm32_usb_rx_isr(core, num);
}

void stm32_usb_on_isr(int vector, void* param)
{
    CORE* core = (CORE*)param;
    uint16_t sta = USB->ISTR;

    if (sta & USB_ISTR_RESET)
    {
        stm32_usb_reset(core);
        USB->ISTR &= ~USB_ISTR_RESET;
        return;
    }
    if ((sta & USB_ISTR_SUSP) && (USB->CNTR & USB_CNTR_SUSPM))
    {
        stm32_usb_suspend(core);
        USB->ISTR &= ~USB_ISTR_SUSP;
        return;
    }
    if (sta & USB_ISTR_WKUP)
    {
        stm32_usb_wakeup(core);
        USB->ISTR &= ~USB_ISTR_WKUP;
        return;
    }
    //transfer complete. Check after status
    if (sta & USB_ISTR_CTR)
    {
        stm32_usb_ctr(core);
        return;
    }
#if (USB_DEBUG_ERRORS)
    IPC ipc;
    if (sta & USB_ISTR_ERR)
    {
        ipc.process = process_iget_current();
        ipc.param1 = USB_HANDLE_DEVICE;
        ipc.cmd = HAL_CMD(HAL_USB, STM32_USB_ERROR);
        ipc_ipost(&ipc);
        USB->ISTR &= ~USB_ISTR_ERR;
    }
    if (sta & USB_ISTR_PMAOVR)
    {
        ipc.process = process_iget_current();
        ipc.param1 = USB_HANDLE_DEVICE;
        ipc.cmd = HAL_CMD(HAL_USB, STM32_USB_OVERFLOW);
        ipc_ipost(&ipc);
        USB->ISTR &= ~USB_ISTR_PMAOVR;
    }
#endif
}

void stm32_usb_open_device(CORE* core, HANDLE device)
{
    int i;
    core->usb.device = device;

    //enable clock
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;

#if defined(STM32F0)
#if (HSE_VALUE)
    RCC->CFGR3 |= RCC_CFGR3_USBSW;
#else
    //Turn USB HSI On (crystall less)
    RCC->CR2 |= RCC_CR2_HSI48ON;
    while ((RCC->CR2 & RCC_CR2_HSI48RDY) == 0) {}
    //setup CRS
    RCC->APB1ENR |= RCC_APB1ENR_CRSEN;
    CRS->CR |= CRS_CR_AUTOTRIMEN;
    CRS->CR |= CRS_CR_CEN;
#endif //HSE_VALUE
#endif //STM32F0

    //power up and wait tStartup
    USB->CNTR &= ~USB_CNTR_PDWN;
    sleep_us(1);
    USB->CNTR &= ~USB_CNTR_FRES;

    //clear any spurious pending interrupts
    USB->ISTR = 0;

    //buffer descriptor table at top
    USB->BTABLE = 0;

    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        USB_BUFFER_DESCRIPTORS[i].ADDR_TX = 0;
        USB_BUFFER_DESCRIPTORS[i].COUNT_TX = 0;
        USB_BUFFER_DESCRIPTORS[i].ADDR_RX = 0;
        USB_BUFFER_DESCRIPTORS[i].COUNT_RX = 0;
    }

    //enable interrupts
    irq_register(USB_IRQn, stm32_usb_on_isr, core);
    NVIC_EnableIRQ(USB_IRQn);
    NVIC_SetPriority(USB_IRQn, 13);

    //Unmask common interrupts
    USB->CNTR |= USB_CNTR_SUSPM | USB_CNTR_RESETM | USB_CNTR_CTRM;
#if (USB_DEBUG_ERRORS)
    USB->CNTR |= USB_CNTR_PMAOVRM | USB_CNTR_ERRM;
#endif

    //pullup device
    USB->BCDR |= USB_BCDR_DPPU;
}

static inline void stm32_usb_open_ep(CORE* core, unsigned int num, USB_EP_TYPE type, unsigned int size)
{
    if (ep_data(core, num) != NULL)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }

    //find free addr in FIFO
    unsigned int fifo, i;
    fifo = 0;
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        if (core->usb.in[i])
            fifo += core->usb.in[i]->mps;
        if (core->usb.out[i])
            fifo += core->usb.out[i]->mps;
    }
    fifo += sizeof(USB_BUFFER_DESCRIPTOR) * USB_EP_COUNT_MAX;

    EP* ep = malloc(sizeof(EP));
    if (ep == NULL)
        return;
    num & USB_EP_IN ? (core->usb.in[USB_EP_NUM(num)] = ep) : (core->usb.out[USB_EP_NUM(num)] = ep);
    ep->io = NULL;
    ep->mps = 0;
    ep->io_active = false;

    uint16_t ctl = USB_EP_NUM(num);
    //setup ep type
    switch (type)
    {
    case USB_EP_CONTROL:
        ctl |= 1 << 9;
        break;
    case USB_EP_BULK:
        ctl |= 0 << 9;
        break;
    case USB_EP_INTERRUPT:
        ctl |= 3 << 9;
        break;
    case USB_EP_ISOCHRON:
        ctl |= 2 << 9;
        break;
    }

    //setup FIFO
    if (num & USB_EP_IN)
    {
        USB_BUFFER_DESCRIPTORS[USB_EP_NUM(num)].ADDR_TX = fifo;
        USB_BUFFER_DESCRIPTORS[USB_EP_NUM(num)].COUNT_TX = 0;
    }
    else
    {
        USB_BUFFER_DESCRIPTORS[USB_EP_NUM(num)].ADDR_RX = fifo;
        if (size <= 62)
            USB_BUFFER_DESCRIPTORS[USB_EP_NUM(num)].COUNT_RX = ((size + 1) >> 1) << 10;
        else
            USB_BUFFER_DESCRIPTORS[USB_EP_NUM(num)].COUNT_RX = ((((size + 3) >> 2) - 1) << 10) | (1 << 15);
    }

    ep->mps = size;

    *ep_reg_data(num) = ctl;
    //set NAK, clear DTOG
    if (num & USB_EP_IN)
        ep_toggle_bits(num, USB_EPTX_STAT | USB_EP_DTOG_TX, USB_EP_TX_NAK);
    else
        ep_toggle_bits(num, USB_EPRX_STAT | USB_EP_DTOG_RX, USB_EP_RX_NAK);
}

static inline void stm32_usb_close_ep(CORE* core, unsigned int num)
{
    if (!stm32_usb_ep_flush(core, num))
        return;
    if (num & USB_EP_IN)
        ep_toggle_bits(num, USB_EPTX_STAT, USB_EP_TX_DIS);
    else
        ep_toggle_bits(num, USB_EPRX_STAT, USB_EP_RX_DIS);

    EP* ep = ep_data(core, num);
    free(ep);
    num & USB_EP_IN ? (core->usb.in[USB_EP_NUM(num)] = NULL) : (core->usb.out[USB_EP_NUM(num)] = NULL);
}

static inline void stm32_usb_close_device(CORE* core)
{
    //disable pullup
    USB->BCDR &= ~USB_BCDR_DPPU;

    int i;
    //disable interrupts
    NVIC_DisableIRQ(USB_IRQn);
    irq_unregister(USB_IRQn);

    //close all endpoints
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        if (core->usb.out[i] != NULL)
            stm32_usb_close_ep(core, i);
        if (core->usb.in[i] != NULL)
            stm32_usb_close_ep(core, USB_EP_IN | i);
    }
    core->usb.device = INVALID_HANDLE;

    //power down, disable all interrupts
    USB->DADDR = 0;
    USB->CNTR = USB_CNTR_PDWN | USB_CNTR_FRES;

    //disable clock
    RCC->APB1ENR &= ~RCC_APB1ENR_USBEN;

}

static inline void stm32_usb_set_address(CORE* core, int addr)
{
    //address will be set after STATUS IN packet
    if (addr)
        core->usb.addr = addr;
    else
        USB->DADDR = USB_DADDR_EF;
}

static bool stm32_usb_io_prepare(CORE* core, IPC* ipc)
{
    EP* ep = ep_data(core, ipc->param1);
    if (ep == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return false;
    }
    if (ep->io_active)
    {
        error(ERROR_IN_PROGRESS);
        return false;
    }
    ep->io = (IO*)ipc->param2;
    return true;
}

static inline void stm32_usb_read(CORE* core, IPC* ipc)
{
    unsigned int ep_num = USB_EP_NUM(ipc->param1);
    EP* ep = core->usb.out[ep_num];
    if (stm32_usb_io_prepare(core, ipc))
    {
        ep->io->data_size = 0;
        ep->size = ipc->param3;
        ep->io_active = true;
        ep_toggle_bits(ep_num, USB_EPRX_STAT, USB_EP_RX_VALID);
        error(ERROR_SYNC);
    }
}

static inline void stm32_usb_write(CORE* core, IPC* ipc)
{
    unsigned int ep_num = USB_EP_NUM(ipc->param1);
    EP* ep = core->usb.in[ep_num];
    if (stm32_usb_io_prepare(core, ipc))
    {
        ep->size = 0;
        stm32_usb_tx(core, ep_num);
        ep_toggle_bits(ep_num, USB_EPTX_STAT, USB_EP_TX_VALID);
        if (ep->size >= ep->io->data_size)
        {
            ipc->param3 = ep->io->data_size;
            return;
        }
        ep->io_active = true;
        error(ERROR_SYNC);
    }
}

void stm32_usb_init(CORE* core)
{
    int i;
    core->usb.device = INVALID_HANDLE;
    core->usb.addr = 0;
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        core->usb.out[i] = NULL;
        core->usb.in[i] = NULL;
    }
}

static inline void stm32_usb_device_request(CORE* core, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case USB_GET_SPEED:
        ipc->param2 = stm32_usb_get_speed(core);
        break;
    case IPC_OPEN:
        stm32_usb_open_device(core, ipc->process);
        break;
    case IPC_CLOSE:
        stm32_usb_close_device(core);
        break;
    case USB_SET_ADDRESS:
        stm32_usb_set_address(core, ipc->param2);
        break;
#if (USB_DEBUG_ERRORS)
    case STM32_USB_ERROR:
        printd("USB: hardware error\n");
        //posted from isr
        break;
     case STM32_USB_OVERFLOW:
        printd("USB: packet memory overflow\n");
        //posted from isr
        break;
#endif
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

static inline void stm32_usb_ep_request(CORE* core, IPC* ipc)
{
    if (USB_EP_NUM(ipc->param1) >= USB_EP_COUNT_MAX)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        stm32_usb_open_ep(core, ipc->param1, ipc->param2, ipc->param3);
        break;
    case IPC_CLOSE:
        stm32_usb_close_ep(core, ipc->param1);
        break;
    case IPC_FLUSH:
        stm32_usb_ep_flush(core, ipc->param1);
        break;
    case USB_EP_SET_STALL:
        stm32_usb_ep_set_stall(core, ipc->param1);
        break;
    case USB_EP_CLEAR_STALL:
        stm32_usb_ep_clear_stall(core, ipc->param1);
        break;
    case USB_EP_IS_STALL:
        ipc->param2 = stm32_usb_ep_is_stall(ipc->param1);
        break;
    case IPC_READ:
        stm32_usb_read(core, ipc);
        break;
    case IPC_WRITE:
        stm32_usb_write(core, ipc);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

void stm32_usb_request(CORE* core, IPC* ipc)
{
    if (ipc->param1 == USB_HANDLE_DEVICE)
        stm32_usb_device_request(core, ipc);
    else
        stm32_usb_ep_request(core, ipc);
}
