/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_usb.h"
#include "../../userspace/sys.h"
#include "../../userspace/usb.h"
#include "../../userspace/irq.h"
#include "../../userspace/lpc/lpc_driver.h"
#include "lpc_pin.h"
#include "lpc_power.h"
#include <string.h>
#include "../../userspace/stdlib.h"
#include "../../userspace/stdio.h"
#include "sys_config.h"
#if (MONOLITH_USB)
#include "lpc_core_private.h"
#endif

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

typedef enum {
    LPC_USB_ERROR = USB_HAL_MAX,
    LPC_USB_OVERFLOW
}LPC_USB_IPCS;

#if (MONOLITH_USB)

#define ack_pin                 lpc_pin_request_inside

#else

#define ack_pin                 lpc_core_request_outside

void lpc_usb();

const REX __LPC_USB = {
    //name
    "LPC USB",
    //size
    LPC_USB_PROCESS_SIZE,
    //priority - driver priority
    92,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_FLAG_PERSISTENT_NAME,
    //function
    lpc_usb
};

#endif

#define USB_EP_INT_BIT(num)                         (1 << (((num) & USB_EP_IN) ? ((USB_EP_NUM(num) << 1) + 1) : (USB_EP_NUM(num) << 1)))
#define USB_EP_LISTSTS(num, buf)                    ((uint32_t*)(((num) & USB_EP_IN) ? (USB_RAM_BASE + (((USB_EP_NUM(num) << 2) + 2 + (buf)) << 2)) : \
                                                                                       (USB_RAM_BASE + (((USB_EP_NUM(num) << 2) + 0 + (buf)) << 2))))

static inline EP* ep_data(SHARED_USB_DRV* drv, unsigned int num)
{
    return (num & USB_EP_IN) ? (drv->usb.in[USB_EP_NUM(num)]) : (drv->usb.out[USB_EP_NUM(num)]);
}

static inline void lpc_usb_tx(SHARED_USB_DRV* drv, int num)
{
    EP* ep = drv->usb.in[USB_EP_NUM(num)];

    int size = ep->io->data_size - ep->size;
    if (size > ep->mps)
        size = ep->mps;

    memcpy(ep->fifo, io_data(ep->io) + ep->size, size);
    ep->size += size;

    *USB_EP_LISTSTS(num, 0) &= ~(USB_EP_LISTST_OFFSET_MASK | USB_EP_LISTST_NBYTES_MASK);
    *USB_EP_LISTSTS(num, 0) |= USB_EP_LISTST_OFFSET_SET(ep->fifo) | USB_EP_LISTST_NBYTES_SET(size);
    *USB_EP_LISTSTS(num, 0) |= USB_EP_LISTST_A;
}

void lpc_usb_rx_prepare(SHARED_USB_DRV* drv, int num)
{
    EP* ep = drv->usb.out[USB_EP_NUM(num)];

    *USB_EP_LISTSTS(num, 0) &= ~(USB_EP_LISTST_OFFSET_MASK | USB_EP_LISTST_NBYTES_MASK);
    *USB_EP_LISTSTS(num, 0) |= USB_EP_LISTST_OFFSET_SET(ep->fifo) | USB_EP_LISTST_NBYTES_SET(ep->mps);
    *USB_EP_LISTSTS(num, 0) |= USB_EP_LISTST_A;
}

static void lpc_usb_ep_reset(SHARED_USB_DRV* drv, int num)
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


bool lpc_usb_ep_flush(SHARED_USB_DRV* drv, int num)
{
    EP* ep = ep_data(drv, num);
    if (ep == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return false;
    }

    ep->io_active = false;
    lpc_usb_ep_reset(drv, num);
    if (ep->io != NULL)
    {
        io_complete_ex(drv->usb.device, HAL_IO_CMD(HAL_USB, (num & USB_EP_IN) ? IPC_WRITE : IPC_READ), num, ep->io, ERROR_IO_CANCELLED);
        ep->io = NULL;
    }
    return true;
}

void lpc_usb_ep_set_stall(SHARED_USB_DRV* drv, int num)
{
    if (!lpc_usb_ep_flush(drv, num))
        return;
    *USB_EP_LISTSTS(num, 0) |= USB_EP_LISTST_S;
}

void lpc_usb_ep_clear_stall(SHARED_USB_DRV* drv, int num)
{
    if (!lpc_usb_ep_flush(drv, num))
        return;
    *USB_EP_LISTSTS(num, 0) &= ~(USB_EP_LISTST_TV | USB_EP_LISTST_S);
    *USB_EP_LISTSTS(num, 0) |= USB_EP_LISTST_TR;
    *USB_EP_LISTSTS(num, 0) &= ~USB_EP_LISTST_TR;
}


bool lpc_usb_ep_is_stall(int num)
{
    return (*USB_EP_LISTSTS(num, 0)) & USB_EP_LISTST_S;
}

USB_SPEED lpc_usb_get_speed(SHARED_USB_DRV* drv)
{
    //according to datasheet LPC11Uxx doesn't support low speed mode...
    return USB_FULL_SPEED;
}

static inline void lpc_usb_reset(SHARED_USB_DRV* drv)
{
    //enable device
    LPC_USB->DEVCMDSTAT |= USB_DEVCMDSTAT_DEV_EN;
    IPC ipc;
    ipc.process = drv->usb.device;
    ipc.cmd = HAL_CMD(HAL_USB, USB_RESET);
    ipc.param1 = USB_HANDLE_DEVICE;
    ipc.param2 = lpc_usb_get_speed(drv);
    ipc_ipost(&ipc);

}

static inline void lpc_usb_suspend(SHARED_USB_DRV* drv)
{
    IPC ipc;
    ipc.process = drv->usb.device;
    ipc.cmd = HAL_CMD(HAL_USB, USB_SUSPEND);
    ipc.param1 = USB_HANDLE_DEVICE;
    ipc_ipost(&ipc);
}

static inline void lpc_usb_wakeup(SHARED_USB_DRV* drv)
{
    IPC ipc;
    ipc.process = drv->usb.device;
    ipc.cmd = HAL_CMD(HAL_USB, USB_WAKEUP);
    ipc.param1 = USB_HANDLE_DEVICE;
    ipc_ipost(&ipc);
}

static inline void lpc_usb_setup(SHARED_USB_DRV* drv)
{
    IPC ipc;
    ipc.process = drv->usb.device;
    ipc.cmd = HAL_CMD(HAL_USB, USB_SETUP);
    ipc.param1 = 0;
    ipc.param2 = *((uint32_t*)(USB_SETUP_BUF_BASE));
    ipc.param3 = *((uint32_t*)(USB_SETUP_BUF_BASE + 4));
    ipc_ipost(&ipc);
}

static inline void lpc_usb_out(SHARED_USB_DRV* drv, int num)
{
    EP* ep = drv->usb.out[USB_EP_NUM(num)];
    unsigned int cnt = ep->mps - (((*USB_EP_LISTSTS(num, 0)) & USB_EP_LISTST_NBYTES_MASK) >> USB_EP_LISTST_NBYTES_POS);

    io_data_append(ep->io, ep->fifo, cnt);
    if (ep->io->data_size >= ep->size || cnt < ep->mps)
    {
        ep->io_active = false;
        iio_complete(drv->usb.device, HAL_IO_CMD(HAL_USB, IPC_READ), num, ep->io);
        ep->io = NULL;
    }
    else
        lpc_usb_rx_prepare(drv, num);
}

static inline void lpc_usb_in(SHARED_USB_DRV* drv, int num)
{
    EP* ep = drv->usb.in[USB_EP_NUM(num)];
    //handle STATUS in for set address
    if (drv->usb.addr && ep->size == 0)
    {
        LPC_USB->DEVCMDSTAT |= drv->usb.addr;
        drv->usb.addr = 0;
    }

    if (ep->size >= ep->io->data_size)
    {
        ep->io_active = false;
        iio_complete(drv->usb.device, HAL_IO_CMD(HAL_USB, IPC_WRITE), USB_EP_IN | num, ep->io);
        ep->io = NULL;
    }
    else
        lpc_usb_tx(drv, num);
}

void lpc_usb_on_isr(int vector, void* param)
{
    int i;
    SHARED_USB_DRV* drv = (SHARED_USB_DRV*)param;
    uint32_t sta = LPC_USB->INTSTAT;
    EP* ep;

#if (USB_DEBUG_ERRORS)
    IPC ipc;
    switch (LPC_USB->INFO & USB_INFO_ERR_CODE_MASK)
    {
    case USB_INFO_ERR_CODE_NO_ERROR:
    case USB_INFO_ERR_CODE_IONAK:
        //no error
        break;
    default:
        ipc.process = process_iget_current();
        ipc.cmd = HAL_CMD(HAL_USB, LPC_USB_ERROR);
        ipc.param1 = USB_HANDLE_DEVICE;
        ipc.param2 = (LPC_USB->INFO & USB_INFO_ERR_CODE_MASK) >> USB_INFO_ERR_CODE_POS;
        ipc_ipost(&ipc);
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
                lpc_usb_suspend(drv);
            else
                lpc_usb_wakeup(drv);
            LPC_USB->DEVCMDSTAT |= USB_DEVCMDSTAT_DSUS_C;
        }
        if (sta & USB_DEVCMDSTAT_DRES_C)
        {
            lpc_usb_reset(drv);
            LPC_USB->DEVCMDSTAT |= USB_DEVCMDSTAT_DRES_C;
        }
        LPC_USB->INTSTAT = USB_INTSTAT_DEV_INT;
        return;
    }
    if ((sta & USB_INTSTAT_EP0OUT) && (LPC_USB->DEVCMDSTAT & USB_DEVCMDSTAT_SETUP))
    {
        lpc_usb_setup(drv);
        LPC_USB->DEVCMDSTAT |= USB_DEVCMDSTAT_SETUP;
        LPC_USB->INTSTAT = USB_INTSTAT_EP0OUT;
        return;
    }

    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        if (sta & USB_EP_INT_BIT(i))
        {
            ep = drv->usb.out[i];
            if (ep && ep->io_active)
                lpc_usb_out(drv, i);
            LPC_USB->INTSTAT = USB_EP_INT_BIT(i);
        }
        if (sta & USB_EP_INT_BIT(USB_EP_IN | i))
        {
            ep = drv->usb.in[i];
            if (ep && ep->io_active)
                lpc_usb_in(drv, i | USB_EP_IN);
            LPC_USB->INTSTAT = USB_EP_INT_BIT(USB_EP_IN | i);
        }
    }
}

void lpc_usb_open_device(SHARED_USB_DRV* drv, HANDLE device)
{
    int i;
    drv->usb.device = device;

    ack_pin(drv, HAL_REQ(HAL_PIN, LPC_PIN_ENABLE), VBUS, PIO0_3_VBUS, 0);
#if (USB_SOFT_CONNECT)
    ack_pin(drv, HAL_REQ(HAL_PIN, LPC_PIN_ENABLE), SCONNECT, PIO0_6_USB_CONNECT, 0);
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
    irq_register(USB_IRQn, lpc_usb_on_isr, drv);
    NVIC_EnableIRQ(USB_IRQn);
    NVIC_SetPriority(USB_IRQn, 1);

    //Unmask common interrupts
    LPC_USB->INTEN = USB_INTSTAT_DEV_INT;

#if (USB_SOFT_CONNECT)
    //pullap
    LPC_USB->DEVCMDSTAT |= USB_DEVCMDSTAT_DCON;
#endif
}

static inline void lpc_usb_open_ep(SHARED_USB_DRV* drv, int num, USB_EP_TYPE type, unsigned int size)
{
    unsigned int i;
    if (ep_data(drv, num) != NULL)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }

    EP* ep = malloc(sizeof(EP));
    if (ep == NULL)
        return;
    ep->io = NULL;
    ep->fifo = (void*)USB_FREE_BUF_BASE;
    ep->io_active = false;
    ep->mps = size;

    //find free addr in FIFO
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        if (drv->usb.in[i])
            ep->fifo += ((drv->usb.in[i]->mps + 63) & ~63);
        if (drv->usb.out[i])
            ep->fifo += ((drv->usb.out[i]->mps + 63) & ~63);
    }

    num & USB_EP_IN ? (drv->usb.in[USB_EP_NUM(num)] = ep) : (drv->usb.out[USB_EP_NUM(num)] = ep);
    if (type == USB_EP_ISOCHRON)
        *USB_EP_LISTSTS(num, 0) |= USB_EP_LISTST_T;
    if (USB_EP_NUM(num))
        *USB_EP_LISTSTS(num, 0) &= ~USB_EP_LISTST_D;
    LPC_USB->INTEN |= USB_EP_INT_BIT(num);
}

static inline void lpc_usb_close_ep(SHARED_USB_DRV* drv, int num)
{
    if (!lpc_usb_ep_flush(drv, num))
        return;
    *USB_EP_LISTSTS(num, 0) |= USB_EP_LISTST_D;
    LPC_USB->INTEN &= ~USB_EP_INT_BIT(num);

    EP* ep = ep_data(drv, num);
    free(ep);
    num & USB_EP_IN ? (drv->usb.in[USB_EP_NUM(num)] = NULL) : (drv->usb.out[USB_EP_NUM(num)] = NULL);
}

static inline void lpc_usb_close_device(SHARED_USB_DRV* drv)
{
    int i;
    //disable interrupts
    NVIC_DisableIRQ(USB_IRQn);
    irq_unregister(USB_IRQn);
#if (USB_SOFT_CONNECT)
    //disable pullap
    LPC_USB->DEVCMDSTAT &= ~USB_DEVCMDSTAT_DCON;
#endif
    //close all endpoints
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        if (drv->usb.out[i] != NULL)
            lpc_usb_close_ep(drv, i);
        if (drv->usb.in[i] != NULL)
            lpc_usb_close_ep(drv, USB_EP_IN | i);
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
    ack_pin(drv, HAL_REQ(HAL_PIN, LPC_PIN_DISABLE), VBUS, 0, 0);
#if (USB_SOFT_CONNECT)
    ack_pin(drv, HAL_REQ(HAL_PIN, LPC_PIN_DISABLE), SCONNECT, 0, 0);
#endif
}

static inline void lpc_usb_set_address(SHARED_USB_DRV* drv, int addr)
{
    //address will be set after STATUS IN packet
    if (addr)
        drv->usb.addr = addr;
    else
        LPC_USB->DEVCMDSTAT &= ~USB_DEVCMDSTAT_DEV_ADDR_MASK;
}

static bool lpc_usb_io_prepare(SHARED_USB_DRV* drv, IPC* ipc)
{
    EP* ep = ep_data(drv, ipc->param1);
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
    error(ERROR_SYNC);
    return true;
}


static inline void lpc_usb_read(SHARED_USB_DRV* drv, IPC* ipc)
{
    unsigned int num = ipc->param1;
    EP* ep = drv->usb.out[USB_EP_NUM(num)];
    if (lpc_usb_io_prepare(drv, ipc))
    {
        ep->io->data_size = 0;
        ep->size = ipc->param3;
        ep->io_active = true;
        lpc_usb_rx_prepare(drv, num);
    }
}

static inline void lpc_usb_write(SHARED_USB_DRV* drv, IPC* ipc)
{
    unsigned int num = ipc->param1;
    EP* ep = drv->usb.in[USB_EP_NUM(num)];
    if (lpc_usb_io_prepare(drv, ipc))
    {
        ep->size = 0;
        ep->io_active = true;
        lpc_usb_tx(drv, num);
    }
}

void lpc_usb_init(SHARED_USB_DRV* drv)
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

static inline void lpc_usb_device_request(SHARED_USB_DRV* drv, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case USB_GET_SPEED:
        ipc->param2 = lpc_usb_get_speed(drv);
        break;
    case IPC_OPEN:
        lpc_usb_open_device(drv, ipc->process);
        break;
    case IPC_CLOSE:
        lpc_usb_close_device(drv);
        break;
    case USB_SET_ADDRESS:
        lpc_usb_set_address(drv, ipc->param2);
        break;
#if (USB_DEBUG_ERRORS)
    case LPC_USB_ERROR:
        printd("USB driver error: %#x\n", ipc->param2);
        //posted from isr
        break;
#endif
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

static inline void lpc_usb_ep_request(SHARED_USB_DRV* drv, IPC* ipc)
{
    if (USB_EP_NUM(ipc->param1) >= USB_EP_COUNT_MAX)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        lpc_usb_open_ep(drv, ipc->param1, ipc->param2, ipc->param3);
        break;
    case IPC_CLOSE:
        lpc_usb_close_ep(drv, ipc->param1);
        break;
    case IPC_FLUSH:
        lpc_usb_ep_flush(drv, ipc->param1);
        break;
    case USB_EP_SET_STALL:
        lpc_usb_ep_set_stall(drv, ipc->param1);
        break;
    case USB_EP_CLEAR_STALL:
        lpc_usb_ep_clear_stall(drv, ipc->param1);
        break;
    case USB_EP_IS_STALL:
        ipc->param2 = lpc_usb_ep_is_stall(ipc->param1);
        break;
    case IPC_READ:
        lpc_usb_read(drv, ipc);
        break;
    case IPC_WRITE:
        lpc_usb_write(drv, ipc);
        //posted with io, no return IPC
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

void lpc_usb_request(SHARED_USB_DRV* drv, IPC* ipc)
{
    if (ipc->param1 == USB_HANDLE_DEVICE)
        lpc_usb_device_request(drv, ipc);
    else
        lpc_usb_ep_request(drv, ipc);
}

#if !(MONOLITH_USB)
void lpc_usb()
{
    IPC ipc;
    SHARED_USB_DRV drv;
    object_set_self(SYS_OBJ_USB);
    lpc_usb_init(&drv);
    for (;;)
    {
        ipc_read(&ipc);
        lpc_usb_request(&drv, &ipc);
        ipc_write(&ipc);
    }
}
#endif
