/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_usb.h"
#include "../../userspace/sys.h"
#include "../../userspace/usb.h"
#include "../../userspace/irq.h"
#include "../../userspace/stm32_driver.h"
#include "stm32_power.h"
#include <string.h>
#include "../../userspace/stdlib.h"
#include "../../userspace/stdio.h"
#if (MONOLITH_USB)
#include "stm32_core_private.h"
#endif

#define USB_DM                          A11
#define USB_DP                          A12

typedef struct {
  HANDLE process;
  HANDLE device;
} USB_STRUCT;

typedef enum {
    STM32_USB_ERROR = USB_HAL_MAX,
    STM32_USB_OVERFLOW
}STM32_USB_IPCS;

#if (MONOLITH_USB)

#define get_clock               stm32_power_get_clock_inside
#define ack_pin                 stm32_pin_request_inside
#define ack_power               stm32_power_request_inside

#else

#define get_clock               stm32_power_get_clock_outside
#define ack_pin                 stm32_core_request_outside
#define ack_power               stm32_core_request_outside

void stm32_usb();

const REX __STM32_USB = {
    //name
    "STM32 USB",
    //size
    STM32_USB_PROCESS_SIZE,
    //priority - driver priority
    92,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_FLAG_PERSISTENT_NAME,
    //function
    stm32_usb
};

#endif

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

static inline EP* ep_data(SHARED_USB_DRV* drv, unsigned int num)
{
    return (num & USB_EP_IN) ? (drv->usb.in[USB_EP_NUM(num)]) : (drv->usb.out[USB_EP_NUM(num)]);
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

static void stm32_usb_tx(SHARED_USB_DRV* drv, unsigned int ep_num)
{
    EP* ep = drv->usb.in[ep_num];

    int size = ep->io->data_size - ep->size;
    if (size > ep->mps)
        size = ep->mps;
    USB_BUFFER_DESCRIPTORS[ep_num].COUNT_TX = size;
    memcpy16((void*)(USB_BUFFER_DESCRIPTORS[ep_num].ADDR_TX + USB_PMAADDR), io_data(ep->io) + ep->size, size);
    ep->size += size;
}

bool stm32_usb_ep_flush(SHARED_USB_DRV* drv, unsigned int num)
{
    EP* ep = ep_data(drv, num);
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
        io_complete_ex(drv->usb.device, HAL_CMD(HAL_USB, (num & USB_EP_IN) ? IPC_WRITE : IPC_READ), num, ep->io, ERROR_IO_CANCELLED);
        ep->io = NULL;
    }
    ep->io_active = false;
    return true;
}

void stm32_usb_ep_set_stall(SHARED_USB_DRV* drv, unsigned int num)
{
    if (!stm32_usb_ep_flush(drv, num))
        return;
    if (USB_EP_IN & num)
        ep_toggle_bits(num, USB_EPTX_STAT, USB_EP_TX_STALL);
    else
        ep_toggle_bits(num, USB_EPRX_STAT, USB_EP_RX_STALL);
}

void stm32_usb_ep_clear_stall(SHARED_USB_DRV* drv, unsigned int num)
{
    if (!stm32_usb_ep_flush(drv, num))
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

USB_SPEED stm32_usb_get_speed(SHARED_USB_DRV* drv)
{
    //according to datasheet STM32L0 doesn't support low speed mode...
    return USB_FULL_SPEED;
}

static inline void stm32_usb_reset(SHARED_USB_DRV* drv)
{
    USB->CNTR |= USB_CNTR_SUSPM;

    //enable function
    USB->DADDR = USB_DADDR_EF;

    IPC ipc;
    ipc.process = drv->usb.device;
    ipc.param1 = USB_HANDLE_DEVICE;
    ipc.param2 = stm32_usb_get_speed(drv);
    ipc.cmd = HAL_CMD(HAL_USB, USB_RESET);
    ipc_ipost(&ipc);
}

static inline void stm32_usb_suspend(SHARED_USB_DRV* drv)
{
    IPC ipc;
    USB->CNTR &= ~USB_CNTR_SUSPM;
    ipc.process = drv->usb.device;
    ipc.param1 = USB_HANDLE_DEVICE;
    ipc.cmd = HAL_CMD(HAL_USB, USB_SUSPEND);
    ipc_ipost(&ipc);
}

static inline void stm32_usb_wakeup(SHARED_USB_DRV* drv)
{
    IPC ipc;
    USB->CNTR |= USB_CNTR_SUSPM;
    ipc.process = drv->usb.device;
    ipc.param1 = USB_HANDLE_DEVICE;
    ipc.cmd = HAL_CMD(HAL_USB, USB_WAKEUP);
    ipc_ipost(&ipc);
}

static inline void stm32_usb_tx_isr(SHARED_USB_DRV* drv, unsigned int ep_num)
{
    EP* ep = drv->usb.in[ep_num];
    //handle STATUS in for set address
    if (drv->usb.addr && USB_BUFFER_DESCRIPTORS[ep_num].COUNT_TX == 0)
    {
        USB->DADDR = USB_DADDR_EF | drv->usb.addr;
        drv->usb.addr = 0;
    }

    if (ep->io_active)
    {
        stm32_usb_tx(drv, ep_num);
        if (ep->size >= ep->io->data_size)
        {
            iio_complete(drv->usb.device, HAL_CMD(HAL_USB, IPC_WRITE), USB_EP_IN | ep_num, ep->io);
            ep->io = NULL;
            ep->io_active = false;
        }
        ep_toggle_bits(ep_num, USB_EPTX_STAT, USB_EP_TX_VALID);
    }
    *ep_reg_data(ep_num) = (*ep_reg_data(ep_num)) & USB_EPREG_MASK & ~USB_EP_CTR_TX;
}

static inline void stm32_usb_rx_isr(SHARED_USB_DRV* drv, unsigned int ep_num)
{
    EP* ep = drv->usb.out[ep_num];
    uint16_t size = USB_BUFFER_DESCRIPTORS[ep_num].COUNT_RX & 0x3ff;
    memcpy16(io_data(ep->io) + ep->io->data_size, (void*)(USB_BUFFER_DESCRIPTORS[ep_num].ADDR_RX + USB_PMAADDR), size);
    *ep_reg_data(ep_num) = (*ep_reg_data(ep_num)) & USB_EPREG_MASK & ~USB_EP_CTR_RX;
    ep->io->data_size += size;

    if (ep->io->data_size >= ep->size || size < ep->mps)
    {
        iio_complete(drv->usb.device, HAL_CMD(HAL_USB, IPC_READ), ep_num, ep->io);
        ep->io = NULL;
        ep->io_active = false;
    }
    else
        ep_toggle_bits(ep_num, USB_EPRX_STAT, USB_EP_RX_VALID);
}

static inline void stm32_usb_ctr(SHARED_USB_DRV* drv)
{
    uint8_t num;
    IPC ipc;
    num = USB->ISTR & USB_ISTR_EP_ID;

    //got SETUP
    if (num == 0 && (*ep_reg_data(num)) & USB_EP_SETUP)
    {
        ipc.cmd = HAL_CMD(HAL_USB, USB_SETUP);
        ipc.process = drv->usb.device;
        ipc.param1 = 0;
        memcpy16(&ipc.param2, (void*)(USB_BUFFER_DESCRIPTORS[0].ADDR_RX + USB_PMAADDR), 4);
        memcpy16(&ipc.param3, (void*)(USB_BUFFER_DESCRIPTORS[0].ADDR_RX + USB_PMAADDR + 4), 4);
        ipc_ipost(&ipc);
        *ep_reg_data(num) = (*ep_reg_data(num)) & USB_EPREG_MASK & ~USB_EP_CTR_RX;
    }

    if ((*ep_reg_data(num)) & USB_EP_CTR_TX)
        stm32_usb_tx_isr(drv, num);

    if ((*ep_reg_data(num)) & USB_EP_CTR_RX)
        stm32_usb_rx_isr(drv, num);
}

void stm32_usb_on_isr(int vector, void* param)
{
    SHARED_USB_DRV* drv = (SHARED_USB_DRV*)param;
    uint16_t sta = USB->ISTR;

    if (sta & USB_ISTR_RESET)
    {
        stm32_usb_reset(drv);
        USB->ISTR &= ~USB_ISTR_RESET;
        return;
    }
    if ((sta & USB_ISTR_SUSP) && (USB->CNTR & USB_CNTR_SUSPM))
    {
        stm32_usb_suspend(drv);
        USB->ISTR &= ~USB_ISTR_SUSP;
        return;
    }
    if (sta & USB_ISTR_WKUP)
    {
        stm32_usb_wakeup(drv);
        USB->ISTR &= ~USB_ISTR_WKUP;
        return;
    }
    //transfer complete. Check after status
    if (sta & USB_ISTR_CTR)
    {
        stm32_usb_ctr(drv);
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

void stm32_usb_open_device(SHARED_USB_DRV* drv, HANDLE device)
{
    int i;
    drv->usb.device = device;

    //enable DM/DP
    ack_pin(drv, HAL_CMD(HAL_PIN, STM32_GPIO_ENABLE_PIN), USB_DM, STM32_GPIO_MODE_AF | GPIO_OT_PUSH_PULL | GPIO_SPEED_HIGH, AF0);
    ack_pin(drv, HAL_CMD(HAL_PIN, STM32_GPIO_ENABLE_PIN), USB_DP, STM32_GPIO_MODE_AF | GPIO_OT_PUSH_PULL | GPIO_SPEED_HIGH, AF0);

    //enable clock
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;

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
    irq_register(USB_IRQn, stm32_usb_on_isr, drv);
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

static inline void stm32_usb_open_ep(SHARED_USB_DRV* drv, unsigned int num, USB_EP_TYPE type, unsigned int size)
{
    if (ep_data(drv, num) != NULL)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }

    //find free addr in FIFO
    unsigned int fifo, i;
    fifo = 0;
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        if (drv->usb.in[i])
            fifo += drv->usb.in[i]->mps;
        if (drv->usb.out[i])
            fifo += drv->usb.out[i]->mps;
    }
    fifo += sizeof(USB_BUFFER_DESCRIPTOR) * USB_EP_COUNT_MAX;

    EP* ep = malloc(sizeof(EP));
    if (ep == NULL)
        return;
    num & USB_EP_IN ? (drv->usb.in[USB_EP_NUM(num)] = ep) : (drv->usb.out[USB_EP_NUM(num)] = ep);
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

static inline void stm32_usb_close_ep(SHARED_USB_DRV* drv, unsigned int num)
{
    if (!stm32_usb_ep_flush(drv, num))
        return;
    if (num & USB_EP_IN)
        ep_toggle_bits(num, USB_EPTX_STAT, USB_EP_TX_DIS);
    else
        ep_toggle_bits(num, USB_EPRX_STAT, USB_EP_RX_DIS);

    EP* ep = ep_data(drv, num);
    free(ep);
    num & USB_EP_IN ? (drv->usb.in[USB_EP_NUM(num)] = NULL) : (drv->usb.out[USB_EP_NUM(num)] = NULL);
}

static inline void stm32_usb_close_device(SHARED_USB_DRV* drv)
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
        if (drv->usb.out[i] != NULL)
            stm32_usb_close_ep(drv, i);
        if (drv->usb.in[i] != NULL)
            stm32_usb_close_ep(drv, USB_EP_IN | i);
    }
    drv->usb.device = INVALID_HANDLE;

    //power down, disable all interrupts
    USB->DADDR = 0;
    USB->CNTR = USB_CNTR_PDWN | USB_CNTR_FRES;

    //disable clock
    RCC->APB1ENR &= ~RCC_APB1ENR_USBEN;

    //disable pins
    ack_pin(drv, HAL_CMD(HAL_PIN, STM32_GPIO_DISABLE_PIN), USB_DM, 0, 0);
    ack_pin(drv, HAL_CMD(HAL_PIN, STM32_GPIO_DISABLE_PIN), USB_DP, 0, 0);
}

static inline void stm32_usb_set_address(SHARED_USB_DRV* drv, int addr)
{
    //address will be set after STATUS IN packet
    if (addr)
        drv->usb.addr = addr;
    else
        USB->DADDR = USB_DADDR_EF;
}

static bool stm32_usb_io_prepare(SHARED_USB_DRV* drv, IPC* ipc)
{
    EP* ep = ep_data(drv, ipc->param1);
    if (ep == NULL)
    {
        io_send_error(ipc, ERROR_NOT_CONFIGURED);
        return false;
    }
    if (ep->io_active)
    {
        io_send_error(ipc, ERROR_IN_PROGRESS);
        return false;
    }
    ep->io = (IO*)ipc->param2;
    return true;
}

static inline void stm32_usb_read(SHARED_USB_DRV* drv, IPC* ipc)
{
    unsigned int ep_num = USB_EP_NUM(ipc->param1);
    EP* ep = drv->usb.out[ep_num];
    if (stm32_usb_io_prepare(drv, ipc))
    {
        ep->io->data_size = 0;
        ep->size = ipc->param3;
        ep->io_active = true;
        ep_toggle_bits(ep_num, USB_EPRX_STAT, USB_EP_RX_VALID);
    }
}

static inline void stm32_usb_write(SHARED_USB_DRV* drv, IPC* ipc)
{
    unsigned int ep_num = USB_EP_NUM(ipc->param1);
    EP* ep = drv->usb.in[ep_num];
    if (stm32_usb_io_prepare(drv, ipc))
    {
        ep->size = 0;
        stm32_usb_tx(drv, ep_num);
        if (ep->size >= ep->io->data_size)
        {
            ipc->param3 = ep->io->data_size;
            io_send(ipc);
        }
        else
            ep->io_active = true;
        ep_toggle_bits(ep_num, USB_EPTX_STAT, USB_EP_TX_VALID);
    }
}

void stm32_usb_init(SHARED_USB_DRV* drv)
{
    int i;
    drv->usb.device = INVALID_HANDLE;
    drv->usb.addr = 0;
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        drv->usb.out[i] = NULL;
        drv->usb.in[i] = NULL;
    }
}

static inline bool stm32_usb_device_request(SHARED_USB_DRV* drv, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case USB_GET_SPEED:
        ipc->param2 = stm32_usb_get_speed(drv);
        need_post = true;
        break;
    case IPC_OPEN:
        stm32_usb_open_device(drv, ipc->process);
        need_post = true;
        break;
    case IPC_CLOSE:
        stm32_usb_close_device(drv);
        need_post = true;
        break;
    case USB_SET_ADDRESS:
        stm32_usb_set_address(drv, ipc->param2);
        need_post = true;
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
        need_post = true;
        break;
    }
    return need_post;
}

static inline bool stm32_usb_ep_request(SHARED_USB_DRV* drv, IPC* ipc)
{
    bool need_post = false;
    if (USB_EP_NUM(ipc->param1) >= USB_EP_COUNT_MAX)
    {
        switch (HAL_ITEM(ipc->cmd))
        {
        case IPC_READ:
        case IPC_WRITE:
            io_send_error(ipc, ERROR_INVALID_PARAMS);
            break;
        default:
            error(ERROR_INVALID_PARAMS);
            need_post = true;
        }
    }
    else
        switch (HAL_ITEM(ipc->cmd))
        {
        case IPC_OPEN:
            stm32_usb_open_ep(drv, ipc->param1, ipc->param2, ipc->param3);
            need_post = true;
            break;
        case IPC_CLOSE:
            stm32_usb_close_ep(drv, ipc->param1);
            need_post = true;
            break;
        case IPC_FLUSH:
            stm32_usb_ep_flush(drv, ipc->param1);
            need_post = true;
            break;
        case USB_EP_SET_STALL:
            stm32_usb_ep_set_stall(drv, ipc->param1);
            need_post = true;
            break;
        case USB_EP_CLEAR_STALL:
            stm32_usb_ep_clear_stall(drv, ipc->param1);
            need_post = true;
            break;
        case USB_EP_IS_STALL:
            ipc->param2 = stm32_usb_ep_is_stall(ipc->param1);
            need_post = true;
            break;
        case IPC_READ:
            stm32_usb_read(drv, ipc);
            //posted with io, no return IPC
            break;
        case IPC_WRITE:
            stm32_usb_write(drv, ipc);
            //posted with io, no return IPC
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
            need_post = true;
            break;
        }
    return need_post;
}

bool stm32_usb_request(SHARED_USB_DRV* drv, IPC* ipc)
{
    bool need_post = false;
    if (ipc->param1 == USB_HANDLE_DEVICE)
        need_post = stm32_usb_device_request(drv, ipc);
    else
        need_post = stm32_usb_ep_request(drv, ipc);
    return need_post;
}

#if !(MONOLITH_USB)
void stm32_usb()
{
    IPC ipc;
    SHARED_USB_DRV drv;
    bool need_post;
    stm32_usb_init(&drv);
    object_set_self(SYS_OBJ_USB);
    for (;;)
    {
        ipc_read(&ipc);
        need_post = stm32_usb_request(&drv, &ipc);
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}
#endif
