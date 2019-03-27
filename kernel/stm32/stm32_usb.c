/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#include "stm32_usb.h"
#include "stm32_regsusb.h"
#include "../../userspace/sys.h"
#include "../../userspace/usb.h"
#include "../kirq.h"
#include "../kexo.h"
#include "../kerror.h"
#include "../../userspace/stm32/stm32_driver.h"
#include "stm32_power.h"
#include <string.h>
#include <stdint.h>
#include "kstdlib.h"
#include "../../userspace/stdio.h"
#include "stm32_exo_private.h"

typedef struct {
    HANDLE process;
    HANDLE device;
} USB_STRUCT;

#if defined(STM32F1)
    #define USB_IRQn                    USB_LP_CAN1_RX0_IRQn
    typedef uint32_t pma_reg;
#elif defined(STM32L1)
    #define USB_IRQn                    USB_LP_IRQn
    typedef uint32_t pma_reg;
#else
    typedef uint16_t pma_reg;
#endif //STM32F1

#pragma pack(push, 1)
typedef struct {
    pma_reg ADDR_TX;
    pma_reg COUNT_TX;
    pma_reg ADDR_RX;
    pma_reg COUNT_RX;
} USB_BUFFER_DESCRIPTOR;
#pragma pack(pop)

#define USB_BUFFER_DESCRIPTORS                      ((USB_BUFFER_DESCRIPTOR*) USB_PMAADDR)

static inline uint16_t* ep_reg_data(unsigned int num)
{
    return (uint16_t*)(USB_BASE + USB_EP_NUM(num) * 4);
}

static inline EP* ep_data(EXO* exo, unsigned int num)
{
    return (num & USB_EP_IN) ? (exo->usb.in[USB_EP_NUM(num)]) : (exo->usb.out[USB_EP_NUM(num)]);
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
#if defined(STM32F1) || defined(STM32L1)
    if(((uint32_t)dst & 0xFFFFF000)==USB_PMAADDR) dst=(void*)((uint32_t)dst + ((uint32_t)dst & 0x00000FFF));
    if(((uint32_t)src & 0xFFFFF000)==USB_PMAADDR) src=(void*)((uint32_t)src + ((uint32_t)src & 0x00000FFF));
#endif
    for (i = 0; i < size; ++i)
    {
        ((uint16_t*)dst)[i] = ((uint16_t*)src)[i];
#if defined(STM32F1) || defined(STM32L1)
        if(((uint32_t)dst & 0xFFFFF000)==USB_PMAADDR)
            dst=(void*)((uint32_t)dst + 2);
        else
            src=(void*)((uint32_t)src + 2);
#endif
    }
}

static void stm32_usb_tx(EXO* exo, unsigned int ep_num)
{
    EP* ep = exo->usb.in[ep_num];

    int size = ep->io->data_size - ep->size;
    if (size > ep->mps)
        size = ep->mps;
    USB_BUFFER_DESCRIPTORS[ep_num].COUNT_TX = size;
    memcpy16((void*)(USB_BUFFER_DESCRIPTORS[ep_num].ADDR_TX + USB_PMAADDR), io_data(ep->io) + ep->size, size);
}

bool stm32_usb_ep_flush(EXO* exo, unsigned int num)
{
    EP* ep = ep_data(exo, num);
    if (ep == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return false;
    }
    if (num & USB_EP_IN)
        ep_toggle_bits(num, USB_EPTX_STAT, USB_EP_TX_NAK);
    else
        ep_toggle_bits(num, USB_EPRX_STAT, USB_EP_RX_NAK);
    if (ep->io != NULL)
    {
        kexo_io_ex(exo->usb.device, HAL_IO_CMD(HAL_USB, (num & USB_EP_IN) ? IPC_WRITE : IPC_READ), USB_HANDLE(USB_0, num), ep->io, ERROR_IO_CANCELLED);
        ep->io = NULL;
    }
    ep->io_active = false;
    return true;
}

void stm32_usb_ep_set_stall(EXO* exo, unsigned int num)
{
    if (!stm32_usb_ep_flush(exo, num))
        return;
    if (USB_EP_IN & num)
        ep_toggle_bits(num, USB_EPTX_STAT, USB_EP_TX_STALL);
    else
        ep_toggle_bits(num, USB_EPRX_STAT, USB_EP_RX_STALL);
}

void stm32_usb_ep_clear_stall(EXO* exo, unsigned int num)
{
    if (!stm32_usb_ep_flush(exo, num))
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

USB_SPEED stm32_usb_get_speed(EXO* exo)
{
    //according to datasheet STM32L0 doesn't support low speed mode...
    return USB_FULL_SPEED;
}

static inline void stm32_usb_reset(EXO* exo)
{
    USB->CNTR |= USB_CNTR_SUSPM;

    //enable function
    USB->DADDR = USB_DADDR_EF;

    IPC ipc;
    ipc.process = exo->usb.device;
    ipc.cmd = HAL_CMD(HAL_USB, USB_RESET);
    ipc.param1 = USB_HANDLE(USB_0, USB_HANDLE_DEVICE);
    ipc.param2 = stm32_usb_get_speed(exo);
    ipc_ipost(&ipc);
}

static inline void stm32_usb_suspend(EXO* exo)
{
    IPC ipc;
    USB->CNTR &= ~USB_CNTR_SUSPM;
    ipc.process = exo->usb.device;
    ipc.param1 = USB_HANDLE(USB_0, USB_HANDLE_DEVICE);
    ipc.cmd = HAL_CMD(HAL_USB, USB_SUSPEND);
    ipc_ipost(&ipc);
}

static inline void stm32_usb_wakeup(EXO* exo)
{
    IPC ipc;
    USB->CNTR |= USB_CNTR_SUSPM;
    ipc.process = exo->usb.device;
    ipc.param1 = USB_HANDLE(USB_0, USB_HANDLE_DEVICE);
    ipc.cmd = HAL_CMD(HAL_USB, USB_WAKEUP);
    ipc_ipost(&ipc);
}

static inline void stm32_usb_tx_isr(EXO* exo, unsigned int ep_num)
{
    EP* ep = exo->usb.in[ep_num];
    //handle STATUS in for set address
    if (exo->usb.addr && USB_BUFFER_DESCRIPTORS[ep_num].COUNT_TX == 0)
    {
        USB->DADDR = USB_DADDR_EF | exo->usb.addr;
        exo->usb.addr = 0;
    }

    if (ep->io_active)
    {
        ep->size += USB_BUFFER_DESCRIPTORS[ep_num].COUNT_TX;
        if (ep->size >= ep->io->data_size)
        {
            ep_toggle_bits(ep_num, USB_EPTX_STAT, USB_EP_TX_NAK);
            iio_complete(exo->usb.device, HAL_IO_CMD(HAL_USB, IPC_WRITE), USB_HANDLE(USB_0, USB_EP_IN | ep_num), ep->io);
            ep->io = NULL;
            ep->io_active = false;
        }
        else
        {
            stm32_usb_tx(exo, ep_num);
            ep_toggle_bits(ep_num, USB_EPTX_STAT, USB_EP_TX_VALID);
        }
    }
    else
        ep_toggle_bits(ep_num, USB_EPTX_STAT, USB_EP_TX_NAK);
    *ep_reg_data(ep_num) = (*ep_reg_data(ep_num)) & USB_EPREG_MASK & ~USB_EP_CTR_TX;
}

static inline void stm32_usb_rx_isr(EXO* exo, unsigned int ep_num)
{
    EP* ep = exo->usb.out[ep_num];
    uint16_t size = USB_BUFFER_DESCRIPTORS[ep_num].COUNT_RX & 0x3ff;
    memcpy16(io_data(ep->io) + ep->io->data_size, (void*)(USB_BUFFER_DESCRIPTORS[ep_num].ADDR_RX + USB_PMAADDR), size);
    *ep_reg_data(ep_num) = (*ep_reg_data(ep_num)) & USB_EPREG_MASK & ~USB_EP_CTR_RX;
    ep->io->data_size += size;

    if (ep->io->data_size >= ep->size || size < ep->mps)
    {
        ep_toggle_bits(ep_num, USB_EPRX_STAT, USB_EP_RX_NAK);
        iio_complete(exo->usb.device, HAL_IO_CMD(HAL_USB, IPC_READ), USB_HANDLE(USB_0, ep_num), ep->io);
        ep->io = NULL;
        ep->io_active = false;
    }
    else
        ep_toggle_bits(ep_num, USB_EPRX_STAT, USB_EP_RX_VALID);
}

static inline void stm32_usb_ctr(EXO* exo)
{
    uint8_t num;
    IPC ipc;
    num = USB->ISTR & USB_ISTR_EP_ID;

    //got SETUP
    if (num == 0 && (*ep_reg_data(num)) & USB_EP_SETUP)
    {
        ipc.cmd = HAL_CMD(HAL_USB, USB_SETUP);
        ipc.process = exo->usb.device;
        ipc.param1 = 0;
        memcpy16(&ipc.param2, (void*)(USB_BUFFER_DESCRIPTORS[0].ADDR_RX + USB_PMAADDR), 4);
        memcpy16(&ipc.param3, (void*)(USB_BUFFER_DESCRIPTORS[0].ADDR_RX + USB_PMAADDR + 4), 4);
        ipc_ipost(&ipc);
        *ep_reg_data(num) = (*ep_reg_data(num)) & USB_EPREG_MASK & ~USB_EP_CTR_RX;
    }

    if ((*ep_reg_data(num)) & USB_EP_CTR_TX)
        stm32_usb_tx_isr(exo, num);

    if ((*ep_reg_data(num)) & USB_EP_CTR_RX)
        stm32_usb_rx_isr(exo, num);
}

void stm32_usb_on_isr(int vector, void* param)
{
    EXO* exo = (EXO*)param;
    uint16_t sta = USB->ISTR;

    if (sta & USB_ISTR_RESET)
    {
        stm32_usb_reset(exo);
        USB->ISTR &= ~USB_ISTR_RESET;
        return;
    }
    if ((sta & USB_ISTR_SUSP) && (USB->CNTR & USB_CNTR_SUSPM))
    {
        stm32_usb_suspend(exo);
        USB->ISTR &= ~USB_ISTR_SUSP;
        return;
    }
    if (sta & USB_ISTR_WKUP)
    {
        stm32_usb_wakeup(exo);
        USB->ISTR &= ~USB_ISTR_WKUP;
        return;
    }
    //transfer complete. Check after status
    if (sta & USB_ISTR_CTR)
    {
        stm32_usb_ctr(exo);
        return;
    }
#if (USB_DEBUG_ERRORS)
    if (sta & USB_ISTR_ERR)
    {
        printk("USB: hardware error\n");
        USB->ISTR &= ~USB_ISTR_ERR;
    }
    if (sta & USB_ISTR_PMAOVR)
    {
        printk("USB: packet memory overflow\n");
        USB->ISTR &= ~USB_ISTR_PMAOVR;
    }
#endif
}

void stm32_usb_open_device(EXO* exo, HANDLE device)
{
    int i;
    exo->usb.device = device;

#if defined(STM32F1)
    //enable clock, setup prescaller
    switch (stm32_power_get_clock_inside(exo, POWER_CORE_CLOCK))
    {
    case 72000000:
        RCC->CFGR &= ~(1 << 22);
        break;
    case 48000000:
        RCC->CFGR |= 1 << 22;
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        return;
    }
#endif

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

    //power up and wait startup
    USB->CNTR &= ~USB_CNTR_PDWN;
#ifdef EXODRIVERS
    exodriver_delay_us(1);
#else
    sleep_us(1);
#endif // EXODRIVERS
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
    kirq_register(KERNEL_HANDLE, USB_IRQn, stm32_usb_on_isr, exo);
    NVIC_EnableIRQ(USB_IRQn);
    NVIC_SetPriority(USB_IRQn, 13);

    //Unmask common interrupts
    USB->CNTR |= USB_CNTR_SUSPM | USB_CNTR_WKUPM | USB_CNTR_RESETM | USB_CNTR_CTRM;
#if (USB_DEBUG_ERRORS)
    USB->CNTR |= USB_CNTR_PMAOVRM | USB_CNTR_ERRM;
#endif

    // hardware pull up DP line
#if defined (STM32F0)
    USB->BCDR |= USB_BCDR_DPPU;
#elif defined(STM32L1)
    SYSCFG->PMC |=  SYSCFG_PMC_USB_PU;
#endif
}

static inline void stm32_usb_open_ep(EXO* exo, unsigned int num, USB_EP_TYPE type, unsigned int size)
{
    if (ep_data(exo, num) != NULL)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }

    //find free addr in FIFO
    unsigned int fifo, i;
    fifo = 0;
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        if (exo->usb.in[i])
            fifo += exo->usb.in[i]->mps;
        if (exo->usb.out[i])
            fifo += exo->usb.out[i]->mps;
    }
#if defined(STM32F1) || defined(STM32L1)
    fifo += sizeof(USB_BUFFER_DESCRIPTOR) * USB_EP_COUNT_MAX / 2;
#else
    fifo += sizeof(USB_BUFFER_DESCRIPTOR) * USB_EP_COUNT_MAX;
#endif

    EP* ep = kmalloc(sizeof(EP));
    if (ep == NULL)
        return;
    num & USB_EP_IN ? (exo->usb.in[USB_EP_NUM(num)] = ep) : (exo->usb.out[USB_EP_NUM(num)] = ep);
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
            USB_BUFFER_DESCRIPTORS[USB_EP_NUM(num)].COUNT_RX = ((((size + 31) >> 5) - 1) << 10) | (1 << 15);
    }

    ep->mps = size;

    *ep_reg_data(num) = ctl;
    //set NAK, clear DTOG
    if (num & USB_EP_IN)
        ep_toggle_bits(num, USB_EPTX_STAT | USB_EP_DTOG_TX, USB_EP_TX_NAK);
    else
        ep_toggle_bits(num, USB_EPRX_STAT | USB_EP_DTOG_RX, USB_EP_RX_NAK);
}

static inline void stm32_usb_close_ep(EXO* exo, unsigned int num)
{
    if (!stm32_usb_ep_flush(exo, num))
        return;
    if (num & USB_EP_IN)
        ep_toggle_bits(num, USB_EPTX_STAT, USB_EP_TX_DIS);
    else
        ep_toggle_bits(num, USB_EPRX_STAT, USB_EP_RX_DIS);

    EP* ep = ep_data(exo, num);
    kfree(ep);
    num & USB_EP_IN ? (exo->usb.in[USB_EP_NUM(num)] = NULL) : (exo->usb.out[USB_EP_NUM(num)] = NULL);
}

static inline void stm32_usb_close_device(EXO* exo)
{
    // hardware pull down DP line
#if defined(STM32F0)
    USB->BCDR &= ~USB_BCDR_DPPU;
#elif defined(STM32L1)
    SYSCFG->PMC &= ~SYSCFG_PMC_USB_PU;
#endif

    int i;
    //disable interrupts
    NVIC_DisableIRQ(USB_IRQn);
    kirq_unregister(KERNEL_HANDLE, USB_IRQn);

    //close all endpoints
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        if (exo->usb.out[i] != NULL)
            stm32_usb_close_ep(exo, i);
        if (exo->usb.in[i] != NULL)
            stm32_usb_close_ep(exo, USB_EP_IN | i);
    }
    exo->usb.device = INVALID_HANDLE;

    //power down, disable all interrupts
    USB->DADDR = 0;
    USB->CNTR = USB_CNTR_PDWN | USB_CNTR_FRES;

    //disable clock
    RCC->APB1ENR &= ~RCC_APB1ENR_USBEN;

}

static inline void stm32_usb_set_address(EXO* exo, int addr)
{
    //address will be set after STATUS IN packet
    if (addr)
        exo->usb.addr = addr;
    else
        USB->DADDR = USB_DADDR_EF;
}

static bool stm32_usb_io_prepare(EXO* exo, IPC* ipc)
{
    EP* ep = ep_data(exo, ipc->param1);
    if (ep == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return false;
    }
    if (ep->io_active)
    {
        kerror(ERROR_IN_PROGRESS);
        return false;
    }
    ep->io = (IO*)ipc->param2;
    return true;
}

static inline void stm32_usb_read(EXO* exo, IPC* ipc)
{
    unsigned int ep_num = USB_EP_NUM(ipc->param1);
    EP* ep = exo->usb.out[ep_num];
    if (stm32_usb_io_prepare(exo, ipc))
    {
        ep->io->data_size = 0;
        ep->size = ipc->param3;
        ep->io_active = true;
        ep_toggle_bits(ep_num, USB_EPRX_STAT, USB_EP_RX_VALID);
        kerror(ERROR_SYNC);
    }
}

static inline void stm32_usb_write(EXO* exo, IPC* ipc)
{
    unsigned int ep_num = USB_EP_NUM(ipc->param1);
    EP* ep = exo->usb.in[ep_num];
    if (stm32_usb_io_prepare(exo, ipc))
    {
        ep->size = 0;
        stm32_usb_tx(exo, ep_num);
        ep_toggle_bits(ep_num, USB_EPTX_STAT, USB_EP_TX_VALID);
        if (ep->size >= ep->io->data_size)
        {
            ipc->param3 = ep->io->data_size;
            return;
        }
        ep->io_active = true;
        kerror(ERROR_SYNC);
    }
}

void stm32_usb_init(EXO* exo)
{
    int i;
    exo->usb.device = INVALID_HANDLE;
    exo->usb.addr = 0;
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        exo->usb.out[i] = NULL;
        exo->usb.in[i] = NULL;
    }
}

static inline void stm32_usb_device_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case USB_GET_SPEED:
        ipc->param2 = stm32_usb_get_speed(exo);
        break;
    case IPC_OPEN:
        stm32_usb_open_device(exo, ipc->process);
        break;
    case IPC_CLOSE:
        stm32_usb_close_device(exo);
        break;
    case USB_SET_ADDRESS:
        stm32_usb_set_address(exo, ipc->param2);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

static inline void stm32_usb_ep_request(EXO* exo, IPC* ipc)
{
    if (USB_EP_NUM(ipc->param1) >= USB_EP_COUNT_MAX)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        stm32_usb_open_ep(exo, ipc->param1, ipc->param2, ipc->param3);
        break;
    case IPC_CLOSE:
        stm32_usb_close_ep(exo, ipc->param1);
        break;
    case IPC_FLUSH:
        stm32_usb_ep_flush(exo, ipc->param1);
        break;
    case USB_EP_SET_STALL:
        stm32_usb_ep_set_stall(exo, ipc->param1);
        break;
    case USB_EP_CLEAR_STALL:
        stm32_usb_ep_clear_stall(exo, ipc->param1);
        break;
    case USB_EP_IS_STALL:
        ipc->param2 = stm32_usb_ep_is_stall(ipc->param1);
        break;
    case IPC_READ:
        stm32_usb_read(exo, ipc);
        break;
    case IPC_WRITE:
        stm32_usb_write(exo, ipc);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

void stm32_usb_request(EXO* exo, IPC* ipc)
{
    if (ipc->param1 == USB_HANDLE_DEVICE)
        stm32_usb_device_request(exo, ipc);
    else
        stm32_usb_ep_request(exo, ipc);
}
