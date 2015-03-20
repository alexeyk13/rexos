/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_usb.h"
#include "stm32_regsusb.h"
#include "../../userspace/stm32_driver.h"
#include "stm32_power.h"
#include "../../userspace/sys.h"
#include "../../userspace/usb.h"
#include "../../userspace/error.h"
#include "../../userspace/irq.h"
#include "../../userspace/block.h"
#include "../../userspace/direct.h"
#include "../../userspace/object.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/file.h"
#include <string.h>
#if (MONOLITH_USB)
#include "stm32_core_private.h"
#endif

#define ALIGN_SIZE                                          (sizeof(int))
#define ALIGN(var)                                          (((var) + (ALIGN_SIZE - 1)) & ~(ALIGN_SIZE - 1))

#define USB_TX_EP0_FIFO_SIZE                                64


typedef enum {
    STM32_USB_FIFO_RX = USB_HAL_MAX,
    STM32_USB_FIFO_TX
}STM32_USB_IPCS;

#if (MONOLITH_USB)

#define get_clock               stm32_power_get_clock_inside
#define ack_gpio                stm32_gpio_request_inside
#define ack_power               stm32_power_request_inside

#else

#define get_clock               stm32_power_get_clock_outside
#define ack_gpio                stm32_core_request_outside
#define ack_power               stm32_core_request_outside

void stm32_usb();

const REX __STM32_USB = {
    //name
    "STM32 USB",
    //size
    STM32_USB_STACK_SIZE,
    //priority - driver priority
    92,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    STM32_DRIVERS_IPC_COUNT,
    //function
    stm32_usb
};

#endif

static inline OTG_FS_DEVICE_ENDPOINT_TypeDef* ep_reg_data(int num)
{
    return (num & USB_EP_IN) ? &(OTG_FS_DEVICE->INEP[USB_EP_NUM(num)]) : &(OTG_FS_DEVICE->OUTEP[USB_EP_NUM(num)]);
}

static inline EP* ep_data(SHARED_USB_DRV* drv, int num)
{
    return (num & USB_EP_IN) ? (drv->usb.in[USB_EP_NUM(num)]) : (drv->usb.out[USB_EP_NUM(num)]);
}

bool stm32_usb_ep_flush(SHARED_USB_DRV* drv, int num)
{
    if (USB_EP_NUM(num) >= USB_EP_COUNT_MAX)
    {
        error(ERROR_INVALID_PARAMS);
        return false;
    }
    EP* ep = ep_data(drv, num);
    if (ep == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return false;
    }
    if (ep->block != INVALID_HANDLE)
    {
        ep_reg_data(num)->CTL |= OTG_FS_DEVICE_ENDPOINT_CTL_SNAK;
        block_send(ep->block, drv->usb.device);
        ep->block = INVALID_HANDLE;
    }
    ep->io_active = false;
    return true;
}

void stm32_usb_ep_set_stall(SHARED_USB_DRV* drv, int num)
{
    if (!stm32_usb_ep_flush(drv, num))
        return;
    ep_reg_data(num)->CTL |= OTG_FS_DEVICE_ENDPOINT_CTL_STALL;
}

void stm32_usb_ep_clear_stall(SHARED_USB_DRV* drv, int num)
{
    if (!stm32_usb_ep_flush(drv, num))
        return;
    ep_reg_data(num)->CTL &= ~OTG_FS_DEVICE_ENDPOINT_CTL_STALL;
}


bool stm32_usb_ep_is_stall(int num)
{
    if (USB_EP_NUM(num) >= USB_EP_COUNT_MAX)
    {
        error(ERROR_INVALID_PARAMS);
        return false;
    }
    return ep_reg_data(num)->CTL & OTG_FS_DEVICE_ENDPOINT_CTL_STALL ? true : false;
}

static inline void usb_suspend(SHARED_USB_DRV* drv)
{
    IPC ipc;
    OTG_FS_GENERAL->INTMSK &= ~OTG_FS_GENERAL_INTMSK_USBSUSPM;
    ipc.process = drv->usb.device;
    ipc.cmd = USB_SUSPEND;
    ipc_ipost(&ipc);
}

static inline void usb_wakeup(SHARED_USB_DRV* drv)
{
    IPC ipc;
    OTG_FS_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_USBSUSPM;
    ipc.process = drv->usb.device;
    ipc.cmd = USB_WAKEUP;
    ipc_ipost(&ipc);
}

static inline void stm32_usb_rx_prepare(SHARED_USB_DRV* drv, int num)
{
    EP* ep = drv->usb.out[USB_EP_NUM(num)];
    int size = ep->size - ep->processed;
    if (size > ep->mps)
        size = ep->mps;
    if (USB_EP_NUM(num))
        OTG_FS_DEVICE->OUTEP[USB_EP_NUM(num)].TSIZ = (1 << OTG_FS_DEVICE_ENDPOINT_TSIZ_PKTCNT_POS) | (size << OTG_FS_DEVICE_ENDPOINT_TSIZ_XFRSIZ_POS);
    else //EP0
        OTG_FS_DEVICE->OUTEP[0].TSIZ = (1 << OTG_FS_DEVICE_ENDPOINT_TSIZ_PKTCNT_POS) | (size << OTG_FS_DEVICE_ENDPOINT_TSIZ_XFRSIZ_POS) |
                                       (3 << OTG_FS_DEVICE_ENDPOINT_TSIZ_STUPCNT_OUT0_POS);
    OTG_FS_DEVICE->OUTEP[USB_EP_NUM(num)].CTL |= OTG_FS_DEVICE_ENDPOINT_CTL_EPENA | OTG_FS_DEVICE_ENDPOINT_CTL_CNAK;
}

static inline void stm32_usb_rx(SHARED_USB_DRV* drv)
{
    IPC ipc;
    unsigned int sta = OTG_FS_GENERAL->RXSTSP;
    unsigned int pktsts = sta & OTG_FS_GENERAL_RXSTSR_PKTSTS;
    int bcnt = (sta & OTG_FS_GENERAL_RXSTSR_BCNT) >> OTG_FS_GENERAL_RXSTSR_BCNT_POS;
    unsigned int num = (sta & OTG_FS_GENERAL_RXSTSR_EPNUM) >> OTG_FS_GENERAL_RXSTSR_EPNUM_POS;
    EP* ep = drv->usb.out[num];

    if (pktsts == OTG_FS_GENERAL_RXSTSR_PKTSTS_SETUP_RX)
    {
        //ignore all data on setup packet
        ipc.process = drv->usb.device;
        ipc.cmd = USB_SETUP;
        ipc.param1 = ((uint32_t*)(OTG_FS_FIFO_BASE + num * 0x1000))[0];
        ipc.param2 = ((uint32_t*)(OTG_FS_FIFO_BASE + num * 0x1000))[1];
        ipc_post(&ipc);
    }
    else if (pktsts == OTG_FS_GENERAL_RXSTSR_PKTSTS_OUT_RX)
    {
        memcpy(ep->ptr + ep->processed, (void*)(OTG_FS_FIFO_BASE + num * 0x1000), ALIGN(bcnt));
        ep->processed += bcnt;

        if (ep->processed >= ep->size)
        {
            ep->io_active = false;
            firead_complete(drv->usb.device, HAL_HANDLE(HAL_USB, num), ep->block, ep->processed);
            ep->block = INVALID_HANDLE;
        }
        else
            stm32_usb_rx_prepare(drv, num);
    }
    OTG_FS_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_RXFLVLM;
}

static inline void stm32_usb_tx(SHARED_USB_DRV* drv, int num)
{
    EP* ep = drv->usb.in[USB_EP_NUM(num)];

    int size = ep->size - ep->processed;
    if (size > ep->mps)
        size = ep->mps;
    OTG_FS_DEVICE->INEP[USB_EP_NUM(num)].TSIZ = (1 << OTG_FS_DEVICE_ENDPOINT_TSIZ_PKTCNT_POS) | (size << OTG_FS_DEVICE_ENDPOINT_TSIZ_XFRSIZ_POS);
    OTG_FS_DEVICE->INEP[USB_EP_NUM(num)].CTL |= OTG_FS_DEVICE_ENDPOINT_CTL_EPENA | OTG_FS_DEVICE_ENDPOINT_CTL_CNAK;

    memcpy((void*)(OTG_FS_FIFO_BASE + USB_EP_NUM(num) * 0x1000), ep->ptr +  ep->processed, ALIGN(size));
    ep->processed += size;
}

USB_SPEED stm32_usb_get_speed(SHARED_USB_DRV* drv)
{
    //according to datasheet STM32F1_CL doesn't support low speed mode...
    return USB_FULL_SPEED;
}

static inline void usb_enumdne(SHARED_USB_DRV* drv)
{
    OTG_FS_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_USBSUSPM;

    IPC ipc;
    ipc.process = drv->usb.device;
    ipc.param1 = stm32_usb_get_speed(drv);
    ipc.cmd = USB_RESET;
    ipc_ipost(&ipc);
}

void usb_on_isr(int vector, void* param)
{
    IPC ipc;
    int i;
    SHARED_USB_DRV* drv = (SHARED_USB_DRV*)param;
    unsigned int sta = OTG_FS_GENERAL->INTSTS;

    //first two most often called
    if ((OTG_FS_GENERAL->INTMSK & OTG_FS_GENERAL_INTMSK_RXFLVLM) && (sta & OTG_FS_GENERAL_INTSTS_RXFLVL))
    {
        //mask interrupts, will be umasked by process after FIFO read
        OTG_FS_GENERAL->INTMSK &= ~OTG_FS_GENERAL_INTMSK_RXFLVLM;
        ipc.process = process_iget_current();
        ipc.cmd = STM32_USB_FIFO_RX;
        ipc_ipost(&ipc);
        return;
    }
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
        if (drv->usb.in[i] != NULL && drv->usb.in[i]->io_active && (OTG_FS_DEVICE->INEP[i].INT & OTG_FS_DEVICE_ENDPOINT_INT_XFRC))
        {
            OTG_FS_DEVICE->INEP[i].INT = OTG_FS_DEVICE_ENDPOINT_INT_XFRC;
            if (drv->usb.in[i]->processed >= drv->usb.in[i]->size)
            {
                drv->usb.in[i]->io_active = false;
                fiwrite_complete(drv->usb.device, HAL_HANDLE(HAL_USB, USB_EP_IN | i), drv->usb.in[i]->block, drv->usb.in[i]->processed);
                drv->usb.in[i]->block = INVALID_HANDLE;
            }
            else
            {
                ipc.process = process_iget_current();
                ipc.cmd = STM32_USB_FIFO_TX;
                ipc.param1 = HAL_HANDLE(HAL_USB, USB_EP_IN | i);
                ipc_ipost(&ipc);
            }
            return;
        }
    //rarely called
    if (sta & OTG_FS_GENERAL_INTSTS_ENUMDNE)
    {
        usb_enumdne(drv);
        OTG_FS_GENERAL->INTSTS |= OTG_FS_GENERAL_INTSTS_ENUMDNE;
        return;
    }
    if ((sta & OTG_FS_GENERAL_INTSTS_USBSUSP) && (OTG_FS_GENERAL->INTMSK & OTG_FS_GENERAL_INTMSK_USBSUSPM))
    {
        usb_suspend(drv);
        OTG_FS_GENERAL->INTSTS |= OTG_FS_GENERAL_INTSTS_USBSUSP;
        return;
    }
    if (sta & OTG_FS_GENERAL_INTSTS_WKUPINT)
    {
        usb_wakeup(drv);
        OTG_FS_GENERAL->INTSTS |= OTG_FS_GENERAL_INTSTS_WKUPINT | OTG_FS_GENERAL_INTSTS_USBSUSP;
    }
}

void stm32_usb_open_device(SHARED_USB_DRV* drv, HANDLE device)
{
    int trdt;
    drv->usb.device = device;

    //enable GPIO
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, A9, STM32_GPIO_MODE_INPUT_FLOAT, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, A10, STM32_GPIO_MODE_INPUT_PULL, true);

    //enable clock, setup prescaller
    ack_power(drv, STM32_POWER_USB_ON, 0, 0, 0);

    OTG_FS_GENERAL->CCFG = OTG_FS_GENERAL_CCFG_PWRDWN;

    //reset core
    while ((OTG_FS_GENERAL->RSTCTL & OTG_FS_GENERAL_RSTCTL_AHBIDL) == 0) {}
    OTG_FS_GENERAL->RSTCTL |= OTG_FS_GENERAL_RSTCTL_CSRST;
    while (OTG_FS_GENERAL->RSTCTL & OTG_FS_GENERAL_RSTCTL_CSRST) {};
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();

    //disable HNP/SRP
    OTG_FS_GENERAL->OTGCTL = 0;

    //refer to programming manual: 1. Setup AHB
    OTG_FS_GENERAL->AHBCFG = (OTG_FS_GENERAL_AHBCFG_GINT) | (OTG_FS_GENERAL_AHBCFG_TXFELVL);
    trdt = get_clock(drv, STM32_CLOCK_CORE) == 72000000 ? 9 : 5;
    //2. Setup USB turn-around time
    OTG_FS_GENERAL->USBCFG = OTG_FS_GENERAL_USBCFG_FDMOD | (trdt << OTG_FS_GENERAL_USBCFG_TRDT_POS) | OTG_FS_GENERAL_USBCFG_PHYSEL | (17 << OTG_FS_GENERAL_USBCFG_TOCAL_POS);

    //Device init: 1.
    OTG_FS_DEVICE->CFG = OTG_FS_DEVICE_CFG_DSPD_FS;
    //2.
    OTG_FS_GENERAL->INTMSK = OTG_FS_GENERAL_INTMSK_USBSUSPM | OTG_FS_GENERAL_INTMSK_ENUMDNEM | OTG_FS_GENERAL_INTMSK_IEPM | OTG_FS_GENERAL_INTMSK_RXFLVLM;
    //3.
    OTG_FS_GENERAL->CCFG |= OTG_FS_GENERAL_CCFG_VBUSBSEN;
    //disable endpoint interrupts, except IN XFRC
    OTG_FS_DEVICE->IEPMSK = OTG_FS_DEVICE_IEPMSK_XFRCM;
    OTG_FS_DEVICE->OEPMSK = 0;
    OTG_FS_DEVICE->CTL = OTG_FS_DEVICE_CTL_POPRGDNE;

    //Setup data FIFO
    OTG_FS_GENERAL->RXFSIZ = ((STM32_USB_MPS / 4) + 1) * 2 + 10 + 1;

    //enable interrupts
    irq_register(OTG_FS_IRQn, usb_on_isr, (void*)drv);
    NVIC_EnableIRQ(OTG_FS_IRQn);
    NVIC_SetPriority(OTG_FS_IRQn, 13);

    //Unmask global interrupts
    OTG_FS_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_OTGM;
}

static inline void stm32_usb_open_ep(SHARED_USB_DRV* drv, int num, USB_EP_TYPE type, unsigned int size)
{
    if (USB_EP_NUM(num) >=  USB_EP_COUNT_MAX)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if (ep_data(drv, num) != NULL)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    EP* ep = malloc(sizeof(EP));
    if (ep == NULL)
        return;
    num & USB_EP_IN ? (drv->usb.in[USB_EP_NUM(num)] = ep) : (drv->usb.out[USB_EP_NUM(num)] = ep);
    ep->block = INVALID_HANDLE;
    ep->mps = 0;
    ep->io_active = false;

    int fifo_used, i;
    //enable, NAK
    uint32_t ctl = OTG_FS_DEVICE_ENDPOINT_CTL_USBAEP | OTG_FS_DEVICE_ENDPOINT_CTL_SNAK;
    //setup ep type, DATA0 for bulk/interrupt, EVEN frame for isochronous endpoint
    switch (type)
    {
    case USB_EP_CONTROL:
        ctl |= OTG_FS_DEVICE_ENDPOINT_CTL_EPTYP_CONTROL;
        break;
    case USB_EP_BULK:
        ctl |= OTG_FS_DEVICE_ENDPOINT_CTL_EPTYP_BULK | OTG_FS_DEVICE_ENDPOINT_CTL_DPID;
        break;
    case USB_EP_INTERRUPT:
        ctl |= OTG_FS_DEVICE_ENDPOINT_CTL_EPTYP_INTERRUPT | OTG_FS_DEVICE_ENDPOINT_CTL_DPID;
        break;
    case USB_EP_ISOCHRON:
        ctl |= OTG_FS_DEVICE_ENDPOINT_CTL_EPTYP_ISOCHRONOUS | OTG_FS_DEVICE_ENDPOINT_CTL_SEVNFRM;
        break;
    }

    if (num & USB_EP_IN)
    {
        //setup TX FIFO num for IN endpoint
        fifo_used = OTG_FS_GENERAL->RXFSIZ & 0xffff;
        for (i = 0; i < USB_EP_NUM(num); ++i)
            fifo_used += drv->usb.in[i]->mps / 4;
        if (USB_EP_NUM(num))
            OTG_FS_GENERAL->DIEPTXF[USB_EP_NUM(num) - 1] = ((size / 4)  << OTG_FS_GENERAL_TX0FSIZ_TX0FD_POS) | ((fifo_used * 4) | OTG_FS_GENERAL_TX0FSIZ_TX0FSA_POS);
        else
            OTG_FS_GENERAL->TX0FSIZ = ((size / 4)  << OTG_FS_GENERAL_TX0FSIZ_TX0FD_POS) | ((fifo_used * 4) | OTG_FS_GENERAL_TX0FSIZ_TX0FSA_POS);
        ctl |= USB_EP_NUM(num) << OTG_FS_DEVICE_ENDPOINT_CTL_TXFNUM_POS;
        //enable interrupts for XFRCM
        OTG_FS_DEVICE->AINTMSK |= 1 << USB_EP_NUM(num);
    }

    //EP_OUT0 has differrent mps structure
    if (USB_EP_NUM(num) == 0)
    {
        switch (size)
        {
        case 8:
            ctl |= OTG_FS_DEVICE_ENDPOINT_CTL_MPSIZ0_8;
            break;
        case 16:
            ctl |= OTG_FS_DEVICE_ENDPOINT_CTL_MPSIZ0_16;
            break;
        case 32:
            ctl |= OTG_FS_DEVICE_ENDPOINT_CTL_MPSIZ0_32;
            break;
        default:
            ctl |= OTG_FS_DEVICE_ENDPOINT_CTL_MPSIZ0_64;
        }
    }
    else
        ctl |= size;

    ep->mps = size;
    ep_reg_data(num)->CTL = ctl;
}

static inline void stm32_usb_close_ep(SHARED_USB_DRV* drv, int num)
{
    if (!stm32_usb_ep_flush(drv, num))
        return;
    ep_reg_data(num)->CTL &= ~OTG_FS_DEVICE_ENDPOINT_CTL_USBAEP;
    EP* ep = ep_data(drv, num);

    ep->mps = 0;
    if (num & USB_EP_IN)
    {
        OTG_FS_DEVICE->AINTMSK &= ~(1 << USB_EP_NUM(num));
        OTG_FS_DEVICE->EIPEMPMSK &= ~(1 << USB_EP_NUM(num));
    }
    free(ep);
    num & USB_EP_IN ? (drv->usb.in[USB_EP_NUM(num)] = NULL) : (drv->usb.out[USB_EP_NUM(num)] = NULL);
}

static inline void stm32_usb_close_device(SHARED_USB_DRV* drv)
{
    int i;
    //Mask global interrupts
    OTG_FS_GENERAL->INTMSK &= ~OTG_FS_GENERAL_INTMSK_OTGM;

    //disable interrupts
    NVIC_DisableIRQ(OTG_FS_IRQn);
    irq_unregister(OTG_FS_IRQn);

    //close all endpoints
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        if (drv->usb.out[i] != NULL)
            stm32_usb_close_ep(drv, i);
        if (drv->usb.in[i] != NULL)
            stm32_usb_close_ep(drv, USB_EP_IN | i);
    }
    drv->usb.device = INVALID_HANDLE;

    //power down
    ack_power(drv, STM32_POWER_USB_OFF, 0, 0, 0);

    //disable pins
    ack_gpio(drv, STM32_GPIO_DISABLE_PIN, A9, 0, 0);
    ack_gpio(drv, STM32_GPIO_DISABLE_PIN, A10, 0, 0);
}

static inline void stm32_usb_set_address(int addr)
{
    OTG_FS_DEVICE->CFG &= OTG_FS_DEVICE_CFG_DAD;
    OTG_FS_DEVICE->CFG |= addr << OTG_FS_DEVICE_CFG_DAD_POS;
}

static inline void stm32_usb_read(SHARED_USB_DRV* drv, unsigned int num, HANDLE block, unsigned int size, HANDLE process)
{
    if (USB_EP_NUM(num) >= USB_EP_COUNT_MAX)
    {
        fread_complete(process, HAL_HANDLE(HAL_USB, num), block, ERROR_INVALID_PARAMS);
        return;
    }
    EP* ep = drv->usb.out[USB_EP_NUM(num)];
    if (ep == NULL)
    {
        fread_complete(process, HAL_HANDLE(HAL_USB, num), block, ERROR_NOT_CONFIGURED);
        return;
    }
    if (ep->io_active)
    {
        fread_complete(process, HAL_HANDLE(HAL_USB, num), block, ERROR_IN_PROGRESS);
        return;
    }
    //no blocks for ZLP
    if (size)
    {
        if ((ep->ptr = block_open(block)) == NULL)
        {
            fread_complete(process, HAL_HANDLE(HAL_USB, num), block, get_last_error());
            return;
        }
    }
    ep->size = size;
    ep->block = block;
    ep->processed = 0;
    ep->io_active = true;
    stm32_usb_rx_prepare(drv, num);
}

static inline void stm32_usb_write(SHARED_USB_DRV* drv, unsigned int num, HANDLE block, unsigned int size, HANDLE process)
{
    if (USB_EP_NUM(num) >= USB_EP_COUNT_MAX)
    {
        fwrite_complete(process, HAL_HANDLE(HAL_USB, num), block, ERROR_INVALID_PARAMS);
        return;
    }
    EP* ep = drv->usb.in[USB_EP_NUM(num)];
    if (ep == NULL)
    {
        fwrite_complete(process, HAL_HANDLE(HAL_USB, num), block, ERROR_NOT_CONFIGURED);
        return;
    }
    if (ep->io_active)
    {
        fwrite_complete(process, HAL_HANDLE(HAL_USB, num), block, ERROR_IN_PROGRESS);
        return;
    }
    ep->size = size;
    //no blocks for ZLP
    if (ep->size)
    {
        if ((ep->ptr = block_open(block)) == NULL)
        {
            fwrite_complete(process, HAL_HANDLE(HAL_USB, num), block, get_last_error());
            return;
        }
    }
    ep->block = block;
    ep->processed = 0;
    ep->io_active = true;

    stm32_usb_tx(drv, num);
}

static inline void stm32_usb_set_test_mode(SHARED_USB_DRV* drv, USB_TEST_MODES test_mode)
{
    OTG_FS_DEVICE->CTL &= ~OTG_FS_DEVICE_CTL_TCTL;
    OTG_FS_DEVICE->CTL |= test_mode << OTG_FS_DEVICE_CTL_TCTL_POS;
}

void stm32_usb_init(SHARED_USB_DRV* drv)
{
    int i;
    drv->usb.device = INVALID_HANDLE;
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        drv->usb.out[i] = NULL;
        drv->usb.in[i] = NULL;
    }
}

bool stm32_usb_request(SHARED_USB_DRV* drv, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
    case STM32_USB_FIFO_RX:
        stm32_usb_rx(drv);
        //message from isr, no response
        break;
    case STM32_USB_FIFO_TX:
        stm32_usb_tx(drv, HAL_ITEM(ipc->param1));
        //message from isr, no response
        break;
    case USB_GET_SPEED:
        ipc->param2 = stm32_usb_get_speed(drv);
        need_post = true;
        break;
    case IPC_OPEN:
        if (HAL_ITEM(ipc->param1) == USB_HANDLE_DEVICE)
            stm32_usb_open_device(drv, ipc->process);
        else
            stm32_usb_open_ep(drv, HAL_ITEM(ipc->param1), ipc->param2, ipc->param3);
        need_post = true;
        break;
    case IPC_CLOSE:
        if (HAL_ITEM(ipc->param1) == USB_HANDLE_DEVICE)
            stm32_usb_close_device(drv);
        else
            stm32_usb_close_ep(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case USB_SET_ADDRESS:
        stm32_usb_set_address(ipc->param1);
        need_post = true;
        break;
    case IPC_FLUSH:
        stm32_usb_ep_flush(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case USB_EP_SET_STALL:
        stm32_usb_ep_set_stall(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case USB_EP_CLEAR_STALL:
        stm32_usb_ep_clear_stall(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case USB_EP_IS_STALL:
        ipc->param2 = stm32_usb_ep_is_stall(HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_READ:
        if ((int)ipc->param3 < 0)
            break;
        stm32_usb_read(drv, HAL_ITEM(ipc->param1), ipc->param2, ipc->param3, ipc->process);
        //generally posted with block, no return IPC
        break;
    case IPC_WRITE:
        if ((int)ipc->param3 < 0)
            break;
        stm32_usb_write(drv, HAL_ITEM(ipc->param1), ipc->param2, ipc->param3, ipc->process);
        //generally posted with block, no return IPC
        break;
    case USB_SET_TEST_MODE:
        stm32_usb_set_test_mode(drv, ipc->param1);
        need_post = true;
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
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
        error(ERROR_OK);
        need_post = false;
        ipc_read_ms(&ipc, 0, ANY_HANDLE);
        switch (ipc.cmd)
        {
        case IPC_PING:
            need_post = true;
            break;
        default:
            need_post = stm32_usb_request(&drv, &ipc);
            break;
        }
        if (need_post)
            ipc_post(&ipc);
    }
}
#endif
