/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_usb.h"
#include "../../userspace/usb.h"
#include "../kirq.h"
#include "../kipc.h"
#include "../kstdlib.h"
#include "../kerror.h"
#include "../../userspace/lpc/lpc_driver.h"
#include "lpc_pin.h"
#include "lpc_power.h"
#include <string.h>
#include "../../userspace/stdio.h"
#include "sys_config.h"

#define VBUS                            PIO0_3
#define SCONNECT                        PIO0_6

#define USB_RAM_BASE                    0x20004000
#define USB_HW_EP_COUNT                 5

//status requiring 0x50 bytes, align is 0x40
#define USB_BUF_BASE                    ((USB_RAM_BASE) + 0x80)
//just first
#define USB_SETUP_BUF_BASE              USB_BUF_BASE
#define USB_FREE_BUF_BASE               (USB_SETUP_BUF_BASE + 0x40)

#define USB_EP_RESET_TIMEOUT            1000

#define USB_EP_INT_BIT(num)                         (1 << (((num) & USB_EP_IN) ? ((USB_EP_NUM(num) << 1) + 1) : (USB_EP_NUM(num) << 1)))
#define USB_EP_LISTSTS(num, buf)                    ((uint32_t*)(((num) & USB_EP_IN) ? (USB_RAM_BASE + (((USB_EP_NUM(num) << 2) + 2 + (buf)) << 2)) : \
                                                                                       (USB_RAM_BASE + (((USB_EP_NUM(num) << 2) + 0 + (buf)) << 2))))

static inline EP* ep_data(EXO* exo, unsigned int num)
{
    return (num & USB_EP_IN) ? (exo->usb.in[USB_EP_NUM(num)]) : (exo->usb.out[USB_EP_NUM(num)]);
}

static inline void lpc_usb_tx(EXO* exo, int num)
{
    EP* ep = exo->usb.in[USB_EP_NUM(num)];

    int size = ep->io->data_size - ep->size;
    if (size > ep->mps)
        size = ep->mps;

    memcpy(ep->fifo, io_data(ep->io) + ep->size, size);
    ep->size += size;

    *USB_EP_LISTSTS(num, 0) &= ~(USB_EP_LISTST_OFFSET_MASK | USB_EP_LISTST_NBYTES_MASK);
    *USB_EP_LISTSTS(num, 0) |= USB_EP_LISTST_OFFSET_SET(ep->fifo) | USB_EP_LISTST_NBYTES_SET(size);
    *USB_EP_LISTSTS(num, 0) |= USB_EP_LISTST_A;
}

void lpc_usb_rx_prepare(EXO* exo, int num)
{
    EP* ep = exo->usb.out[USB_EP_NUM(num)];

    *USB_EP_LISTSTS(num, 0) &= ~(USB_EP_LISTST_OFFSET_MASK | USB_EP_LISTST_NBYTES_MASK);
    *USB_EP_LISTSTS(num, 0) |= USB_EP_LISTST_OFFSET_SET(ep->fifo) | USB_EP_LISTST_NBYTES_SET(ep->mps);
    *USB_EP_LISTSTS(num, 0) |= USB_EP_LISTST_A;
}

static void lpc_usb_ep_reset(EXO* exo, int num)
{
    int i;
    //disabling IO is required co access S, TR bits
    //active bit can't be reset directly
    if ((*USB_EP_LISTSTS(num, 0)) & USB_EP_LISTST_A)
    {
        //ignore this interrupt source
        LPC_USB->INTEN &= ~USB_EP_INT_BIT(num);
        LPC_USB->EPSKIP |= USB_EP_INT_BIT(num);
        for (i = 0; i < USB_EP_RESET_TIMEOUT; ++i)
            if (LPC_USB->EPSKIP & USB_EP_INT_BIT(num))
                break;
        //clear and unmask int
        LPC_USB->INTSTAT = USB_EP_INT_BIT(num);
        LPC_USB->INTEN |= USB_EP_INT_BIT(num);
    }
    *USB_EP_LISTSTS(num, 0) &= ~(USB_EP_LISTST_TV | USB_EP_LISTST_S);
    *USB_EP_LISTSTS(num, 0) |= USB_EP_LISTST_TR;
    *USB_EP_LISTSTS(num, 0) &= ~USB_EP_LISTST_TR;
}


bool lpc_usb_ep_flush(EXO* exo, int num)
{
    EP* ep = ep_data(exo, num);
    if (ep == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return false;
    }

    ep->io_active = false;
    lpc_usb_ep_reset(exo, num);
    if (ep->io != NULL)
    {
        io_complete_ex(exo->usb.device, HAL_IO_CMD(HAL_USB, (num & USB_EP_IN) ? IPC_WRITE : IPC_READ), num, ep->io, ERROR_IO_CANCELLED);
        ep->io = NULL;
    }
    return true;
}

void lpc_usb_ep_set_stall(EXO* exo, int num)
{
    if (!lpc_usb_ep_flush(exo, num))
        return;
    *USB_EP_LISTSTS(num, 0) |= USB_EP_LISTST_S;
}

void lpc_usb_ep_clear_stall(EXO* exo, int num)
{
    if (!lpc_usb_ep_flush(exo, num))
        return;
    *USB_EP_LISTSTS(num, 0) &= ~(USB_EP_LISTST_TV | USB_EP_LISTST_S);
    *USB_EP_LISTSTS(num, 0) |= USB_EP_LISTST_TR;
    *USB_EP_LISTSTS(num, 0) &= ~USB_EP_LISTST_TR;
}


bool lpc_usb_ep_is_stall(int num)
{
    return (*USB_EP_LISTSTS(num, 0)) & USB_EP_LISTST_S;
}

USB_SPEED lpc_usb_get_speed(EXO* exo)
{
    //according to datasheet LPC11Uxx doesn't support low speed mode...
    return USB_FULL_SPEED;
}

static inline void lpc_usb_reset(EXO* exo)
{
    //enable device
    LPC_USB->DEVCMDSTAT |= USB_DEVCMDSTAT_DEV_EN;
    if (exo->usb.device_ready)
    {
        ipc_ipost_inline(exo->usb.device, HAL_CMD(HAL_USB, USB_RESET), USB_HANDLE(USB_0, USB_HANDLE_DEVICE), USB_FULL_SPEED, 0);
        exo->usb.device_ready = false;
    }
    else
    {
        exo->usb.device_pending = true;
        exo->usb.pending_ipc = HAL_CMD(HAL_USB, USB_RESET);
    }
}

static inline void lpc_usb_suspend(EXO* exo)
{
    if (exo->usb.device_ready)
    {
        ipc_ipost_inline(exo->usb.device, HAL_CMD(HAL_USB, USB_SUSPEND), USB_HANDLE(USB_0, USB_HANDLE_DEVICE), 0, 0);
        exo->usb.device_ready = false;
    }
    else
    {
        exo->usb.device_pending = true;
        exo->usb.pending_ipc = HAL_CMD(HAL_USB, USB_SUSPEND);
    }
}

static inline void lpc_usb_wakeup(EXO* exo)
{
    if (exo->usb.device_ready)
    {
        ipc_ipost_inline(exo->usb.device, HAL_CMD(HAL_USB, USB_WAKEUP), USB_HANDLE(USB_0, USB_HANDLE_DEVICE), 0, 0);
        exo->usb.device_ready = false;
    }
    else
    {
        exo->usb.device_pending = true;
        exo->usb.pending_ipc = HAL_CMD(HAL_USB, USB_WAKEUP);
    }
}

static inline void lpc_usb_setup(EXO* exo)
{
    IPC ipc;
    ipc.process = exo->usb.device;
    ipc.cmd = HAL_CMD(HAL_USB, USB_SETUP);
    ipc.param1 = 0;
    ipc.param2 = *((uint32_t*)(USB_SETUP_BUF_BASE));
    ipc.param3 = *((uint32_t*)(USB_SETUP_BUF_BASE + 4));
    ipc_ipost(&ipc);
}

static inline void lpc_usb_out(EXO* exo, int num)
{
    EP* ep = exo->usb.out[USB_EP_NUM(num)];
    unsigned int cnt = ep->mps - (((*USB_EP_LISTSTS(num, 0)) & USB_EP_LISTST_NBYTES_MASK) >> USB_EP_LISTST_NBYTES_POS);

    io_data_append(ep->io, ep->fifo, cnt);
    if (ep->io->data_size >= ep->size || cnt < ep->mps)
    {
        ep->io_active = false;
        iio_complete(exo->usb.device, HAL_IO_CMD(HAL_USB, IPC_READ), num, ep->io);
        ep->io = NULL;
    }
    else
        lpc_usb_rx_prepare(exo, num);
}

static inline void lpc_usb_in(EXO* exo, int num)
{
    EP* ep = exo->usb.in[USB_EP_NUM(num)];
    //handle STATUS in for set address
    if (exo->usb.addr && ep->size == 0)
    {
        LPC_USB->DEVCMDSTAT |= exo->usb.addr;
        exo->usb.addr = 0;
    }

    if (ep->size >= ep->io->data_size)
    {
        ep->io_active = false;
        iio_complete(exo->usb.device, HAL_IO_CMD(HAL_USB, IPC_WRITE), USB_EP_IN | num, ep->io);
        ep->io = NULL;
    }
    else
        lpc_usb_tx(exo, num);
}

void lpc_usb_on_isr(int vector, void* param)
{
    int i;
    EXO* exo = (SHARED_USB_DRV*)param;
    uint32_t sta = LPC_USB->INTSTAT;
    EP* ep;

#if (USB_DEBUG_ERRORS)
    switch (LPC_USB->INFO & USB_INFO_ERR_CODE_MASK)
    {
    case USB_INFO_ERR_CODE_NO_ERROR:
    case USB_INFO_ERR_CODE_IONAK:
        //no error
        break;
    default:
        printk("USB: Error %d\n", (LPC_USB->INFO & USB_INFO_ERR_CODE_MASK) >> USB_INFO_ERR_CODE_POS);
        LPC_USB->INFO &= ~USB_INFO_ERR_CODE_MASK;
    }
#endif

    if (sta & USB_INTSTAT_DEV_INT)
    {
        sta = LPC_USB->DEVCMDSTAT;
        //Don't care on connection change, just clear pending bit
        if (sta & USB_DEVCMDSTAT_DCON_C)
            LPC_USB->DEVCMDSTAT |= USB_DEVCMDSTAT_DCON_C;
        if (sta & USB_DEVCMDSTAT_DSUS_C)
        {
            if (sta & USB_DEVCMDSTAT_DSUS)
                lpc_usb_suspend(exo);
            else
                lpc_usb_wakeup(exo);
            LPC_USB->DEVCMDSTAT |= USB_DEVCMDSTAT_DSUS_C;
        }
        if (sta & USB_DEVCMDSTAT_DRES_C)
        {
            lpc_usb_reset(exo);
            LPC_USB->DEVCMDSTAT |= USB_DEVCMDSTAT_DRES_C;
        }
        LPC_USB->INTSTAT = USB_INTSTAT_DEV_INT;
        return;
    }
    if ((sta & USB_INTSTAT_EP0OUT) && (LPC_USB->DEVCMDSTAT & USB_DEVCMDSTAT_SETUP))
    {
        lpc_usb_setup(exo);
        LPC_USB->DEVCMDSTAT |= USB_DEVCMDSTAT_SETUP;
        LPC_USB->INTSTAT = USB_INTSTAT_EP0OUT;
        return;
    }

    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        if (sta & USB_EP_INT_BIT(i))
        {
            ep = exo->usb.out[i];
            if (ep && ep->io_active)
                lpc_usb_out(exo, i);
            LPC_USB->INTSTAT = USB_EP_INT_BIT(i);
        }
        if (sta & USB_EP_INT_BIT(USB_EP_IN | i))
        {
            ep = exo->usb.in[i];
            if (ep && ep->io_active)
                lpc_usb_in(exo, i | USB_EP_IN);
            LPC_USB->INTSTAT = USB_EP_INT_BIT(USB_EP_IN | i);
        }
    }
}

void lpc_usb_open_device(EXO* exo, HANDLE device)
{
    int i;
    exo->usb.device = device;

    exo->usb.device_pending = false;
    exo->usb.device_ready = true;

    lpc_pin_enable(VBUS, PIO0_3_VBUS);
#if (USB_SOFT_CONNECT)
    lpc_pin_enable(SCONNECT, PIO0_6_USB_CONNECT);
#endif

    //enable clock, power up
    //power on. USBPLL must be turned on even in case of SYS PLL used. Why?
    LPC_SYSCON->PDRUNCFG &= ~(SYSCON_PDRUNCFG_USBPLL_PD | SYSCON_PDRUNCFG_USBPAD_PD);
#if (USB_DEDICATED_PLL)

    int i;
    //enable and lock PLL
    LPC_SYSCON->USBPLLCTRL = ((USBPLL_M - 1) << SYSCON_USBPLLCTRL_MSEL_POS) | ((32 - __builtin_clz(USBPLL_P)) << SYSCON_USBPLLCTRL_PSEL_POS);
    LPC_SYSCON->USBPLLCLKSEL = SYSCON_USBPLLCLKSEL_SYSOSC;

    LPC_SYSCON->USBPLLCLKUEN = 0;
    LPC_SYSCON->USBPLLCLKUEN = SYSCON_SYSPLLCLKUEN_ENA;
    //wait for PLL lock
    for (i = 0; i < PLL_LOCK_TIMEOUT; ++i)
    {
        if (LPC_SYSCON->USBPLLSTAT & SYSCON_USBPLLSTAT_LOCK)
            break;
    }

    LPC_SYSCON->USBCLKSEL = SYSCON_USBCLKSEL_PLL;
#else
    LPC_SYSCON->USBCLKSEL = SYSCON_USBCLKSEL_MAIN;
#endif
    //switch to clock source
    LPC_SYSCON->USBCLKUEN = 0;
    LPC_SYSCON->USBCLKUEN = SYSCON_USBCLKUEN_ENA;
    //turn clock on
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << SYSCON_SYSAHBCLKCTRL_USB_POS) | (1 << SYSCON_SYSAHBCLKCTRL_USBRAM_POS);

    //clear any spurious pending interrupts
    LPC_USB->DEVCMDSTAT = USB_DEVCMDSTAT_DCON_C | USB_DEVCMDSTAT_DSUS_C | USB_DEVCMDSTAT_DRES_C | USB_DEVCMDSTAT_SETUP;
    LPC_USB->INTSTAT = USB_INTSTAT_EP0OUT | USB_INTSTAT_EP0IN | USB_INTSTAT_EP1OUT | USB_INTSTAT_EP1IN |
                       USB_INTSTAT_EP2OUT | USB_INTSTAT_EP2IN | USB_INTSTAT_EP3OUT | USB_INTSTAT_EP3IN |
                       USB_INTSTAT_EP4OUT | USB_INTSTAT_EP4IN | USB_INTSTAT_FRAME_INT | USB_INTSTAT_DEV_INT;
#if (USB_DEBUG_ERRORS)
    LPC_USB->INFO &= ~USB_INFO_ERR_CODE_MASK;
#endif

    //setup buffer descriptor table
    LPC_USB->EPLISTSTART = USB_RAM_BASE & ~0xff;
    LPC_USB->DATABUFSTART = USB_BUF_BASE & ~0x3fffff;

    //clear descriptor table data
    for (i = 0; i < USB_HW_EP_COUNT; ++i)
    {
        *USB_EP_LISTSTS(i, 0) = *USB_EP_LISTSTS(i, 1) = 0;
        *USB_EP_LISTSTS(i | USB_EP_IN, 0) = *USB_EP_LISTSTS(i | USB_EP_IN, 1) = 0;
    }
    //SETUP buffer offset is not incremented
    *USB_EP_LISTSTS(0, 1) = USB_EP_LISTST_OFFSET_SET(USB_SETUP_BUF_BASE);

    //enable interrupts
    kirq_register(KERNEL_HANDLE, USB_IRQn, lpc_usb_on_isr, exo);
    NVIC_EnableIRQ(USB_IRQn);
    NVIC_SetPriority(USB_IRQn, 1);

    //Unmask common interrupts
    LPC_USB->INTEN = USB_INTSTAT_DEV_INT;

#if (USB_SOFT_CONNECT)
    //pullap
    LPC_USB->DEVCMDSTAT |= USB_DEVCMDSTAT_DCON;
#endif
}

static inline void lpc_usb_open_ep(EXO* exo, int num, USB_EP_TYPE type, unsigned int size)
{
    unsigned int i;
    if (ep_data(exo, num) != NULL)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }

    EP* ep = kmalloc(sizeof(EP));
    if (ep == NULL)
        return;
    ep->io = NULL;
    ep->fifo = (void*)USB_FREE_BUF_BASE;
    ep->io_active = false;
    ep->mps = size;

    //find free addr in FIFO
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        if (exo->usb.in[i])
            ep->fifo += ((exo->usb.in[i]->mps + 63) & ~63);
        if (exo->usb.out[i])
            ep->fifo += ((exo->usb.out[i]->mps + 63) & ~63);
    }

    num & USB_EP_IN ? (exo->usb.in[USB_EP_NUM(num)] = ep) : (exo->usb.out[USB_EP_NUM(num)] = ep);
    if (type == USB_EP_ISOCHRON)
        *USB_EP_LISTSTS(num, 0) |= USB_EP_LISTST_T;
    if (USB_EP_NUM(num))
        *USB_EP_LISTSTS(num, 0) &= ~USB_EP_LISTST_D;
    LPC_USB->INTEN |= USB_EP_INT_BIT(num);
}

static inline void lpc_usb_close_ep(EXO* exo, int num)
{
    if (!lpc_usb_ep_flush(exo, num))
        return;
    *USB_EP_LISTSTS(num, 0) |= USB_EP_LISTST_D;
    LPC_USB->INTEN &= ~USB_EP_INT_BIT(num);

    EP* ep = ep_data(exo, num);
    kfree(ep);
    num & USB_EP_IN ? (exo->usb.in[USB_EP_NUM(num)] = NULL) : (exo->usb.out[USB_EP_NUM(num)] = NULL);
}

static inline void lpc_usb_close_device(EXO* exo)
{
    int i;
    //disable interrupts
    NVIC_DisableIRQ(USB_IRQn);
    kirq_unregister(KERNEL_HANDLE, USB_IRQn);
#if (USB_SOFT_CONNECT)
    //disable pullap
    LPC_USB->DEVCMDSTAT &= ~USB_DEVCMDSTAT_DCON;
#endif
    //close all endpoints
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        if (exo->usb.out[i] != NULL)
            lpc_usb_close_ep(exo, i);
        if (exo->usb.in[i] != NULL)
            lpc_usb_close_ep(exo, USB_EP_IN | i);
    }
    //Mask all interrupts
    LPC_USB->INTEN = 0;
    //disable device
    LPC_USB->DEVCMDSTAT &= ~USB_DEVCMDSTAT_DEV_EN;

    //power down
    //turn clock off
    LPC_SYSCON->SYSAHBCLKCTRL &= ~((1 << SYSCON_SYSAHBCLKCTRL_USB_POS) | (1 << SYSCON_SYSAHBCLKCTRL_USBRAM_POS));
    //power down
    LPC_SYSCON->PDRUNCFG |= SYSCON_PDRUNCFG_USBPAD_PD;
#if (USB_DEDICATED_PLL)
    LPC_SYSCON->PDRUNCFG |= SYSCON_PDRUNCFG_USBPLL_PD;
#endif

    //disable pins
    lpc_pin_disable(VBUS);
#if (USB_SOFT_CONNECT)
    lpc_pin_disable(SCONNECT);
#endif
}

static inline void lpc_usb_set_address(EXO* exo, int addr)
{
    //address will be set after STATUS IN packet
    if (addr)
        exo->usb.addr = addr;
    else
        LPC_USB->DEVCMDSTAT &= ~USB_DEVCMDSTAT_DEV_ADDR_MASK;
}

static inline void lpc_usb_sync(EXO* exo)
{
    if (exo->usb.device == INVALID_HANDLE)
        continue;
    if (exo->usb.device_pending)
    {
        switch (HAL_ITEM(exo->usb.pending_ipc))
        {
        case USB_RESET:
            ipc_ipost_inline(exo->usb.device, exo->usb.pending_ipc, USB_HANDLE(USB_0, USB_HANDLE_DEVICE), USB_FULL_SPEED, 0);
            break;
        case USB_SUSPEND:
        case USB_WAKEUP:
            ipc_ipost_inline(exo->usb.device, exo->usb.pending_ipc, USB_HANDLE(USB_0, USB_HANDLE_DEVICE), 0, 0);
            break;
        default:
            break;
        }
        exo->usb.device_pending = false;
    }
    else
        exo->usb.device_ready = true;
}

static bool lpc_usb_io_prepare(EXO* exo, IPC* ipc)
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
    kerror(ERROR_SYNC);
    return true;
}


static inline void lpc_usb_read(EXO* exo, IPC* ipc)
{
    unsigned int num = ipc->param1;
    EP* ep = exo->usb.out[USB_EP_NUM(num)];
    if (lpc_usb_io_prepare(exo, ipc))
    {
        ep->io->data_size = 0;
        ep->size = ipc->param3;
        ep->io_active = true;
        lpc_usb_rx_prepare(exo, num);
    }
}

static inline void lpc_usb_write(EXO* exo, IPC* ipc)
{
    unsigned int num = ipc->param1;
    EP* ep = exo->usb.in[USB_EP_NUM(num)];
    if (lpc_usb_io_prepare(exo, ipc))
    {
        ep->size = 0;
        ep->io_active = true;
        lpc_usb_tx(exo, num);
    }
}

void lpc_usb_init(EXO* exo)
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

static inline void lpc_usb_device_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case USB_GET_SPEED:
        ipc->param2 = lpc_usb_get_speed(exo);
        break;
    case IPC_OPEN:
        lpc_usb_open_device(exo, ipc->process);
        break;
    case IPC_CLOSE:
        lpc_usb_close_device(exo);
        break;
    case USB_SET_ADDRESS:
        lpc_usb_set_address(exo, ipc->param2);
        break;
    case IPC_SYNC:
        lpc_otg_sync(exo);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

static inline void lpc_usb_ep_request(EXO* exo, IPC* ipc)
{
    if (USB_EP_NUM(ipc->param1) >= USB_EP_COUNT_MAX)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        lpc_usb_open_ep(exo, ipc->param1, ipc->param2, ipc->param3);
        break;
    case IPC_CLOSE:
        lpc_usb_close_ep(exo, ipc->param1);
        break;
    case IPC_FLUSH:
        lpc_usb_ep_flush(exo, ipc->param1);
        break;
    case USB_EP_SET_STALL:
        lpc_usb_ep_set_stall(exo, ipc->param1);
        break;
    case USB_EP_CLEAR_STALL:
        lpc_usb_ep_clear_stall(exo, ipc->param1);
        break;
    case USB_EP_IS_STALL:
        ipc->param2 = lpc_usb_ep_is_stall(ipc->param1);
        break;
    case IPC_READ:
        lpc_usb_read(exo, ipc);
        break;
    case IPC_WRITE:
        lpc_usb_write(exo, ipc);
        //posted with io, no return IPC
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

void lpc_usb_request(EXO* exo, IPC* ipc)
{
    if (ipc->param1 == USB_HANDLE_DEVICE)
        lpc_usb_device_request(exo, ipc);
    else
        lpc_usb_ep_request(exo, ipc);
}
