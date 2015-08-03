/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_otg.h"
#include "stm32_regsusb.h"
#include "../../userspace/stm32_driver.h"
#include "stm32_power.h"
#include "../../userspace/sys.h"
#include "../../userspace/usb.h"
#include "../../userspace/error.h"
#include "../../userspace/irq.h"
#include "../../userspace/object.h"
#include "../../userspace/stdlib.h"
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
#define ack_pin                 stm32_pin_request_inside
#define ack_power               stm32_power_request_inside

#else

#define get_clock               stm32_power_get_clock_outside
#define ack_pin                 stm32_core_request_outside
#define ack_power               stm32_core_request_outside

void stm32_otg();

const REX __STM32_OTG = {
    //name
    "STM32 OTG",
    //size
    STM32_USB_PROCESS_SIZE,
    //priority - driver priority
    92,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    STM32_DRIVERS_IPC_COUNT,
    //function
    stm32_otg
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

bool stm32_otg_ep_flush(SHARED_USB_DRV* drv, int num)
{
    EP* ep = ep_data(drv, num);
    if (ep == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return false;
    }
    if (ep->io != NULL)
    {
        io_complete_ex(drv->usb.device, HAL_CMD(HAL_USB, (num & USB_EP_IN) ? IPC_WRITE : IPC_READ), num, ep->io, ERROR_IO_CANCELLED);
        ep->io = NULL;
        ep_reg_data(num)->CTL |= OTG_FS_DEVICE_ENDPOINT_CTL_SNAK;
    }
    ep->io_active = false;
    return true;
}

static inline void stm32_otg_ep_set_stall(SHARED_USB_DRV* drv, int num)
{
    if (!stm32_otg_ep_flush(drv, num))
        return;
    ep_reg_data(num)->CTL |= OTG_FS_DEVICE_ENDPOINT_CTL_STALL;
}

static inline void stm32_otg_ep_clear_stall(SHARED_USB_DRV* drv, int num)
{
    if (!stm32_otg_ep_flush(drv, num))
        return;
    ep_reg_data(num)->CTL &= ~OTG_FS_DEVICE_ENDPOINT_CTL_STALL;
}

static inline bool stm32_otg_ep_is_stall(int num)
{
    return ep_reg_data(num)->CTL & OTG_FS_DEVICE_ENDPOINT_CTL_STALL ? true : false;
}

static inline void usb_suspend(SHARED_USB_DRV* drv)
{
    IPC ipc;
    OTG_FS_GENERAL->INTMSK &= ~OTG_FS_GENERAL_INTMSK_USBSUSPM;
    ipc.process = drv->usb.device;
    ipc.param1 = USB_HANDLE_DEVICE;
    ipc.cmd = HAL_CMD(HAL_USB, USB_SUSPEND);
    ipc_ipost(&ipc);
}

static inline void usb_wakeup(SHARED_USB_DRV* drv)
{
    IPC ipc;
    OTG_FS_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_USBSUSPM;
    ipc.process = drv->usb.device;
    ipc.param1 = USB_HANDLE_DEVICE;
    ipc.cmd = HAL_CMD(HAL_USB, USB_WAKEUP);
    ipc_ipost(&ipc);
}

static void stm32_otg_rx_prepare(SHARED_USB_DRV* drv, unsigned int ep_num)
{
    EP* ep = drv->usb.out[ep_num];
    int size = ep->size - ep->io->data_size;
    if (size > ep->mps)
        size = ep->mps;
    if (ep_num)
        OTG_FS_DEVICE->OUTEP[ep_num].TSIZ = (1 << OTG_FS_DEVICE_ENDPOINT_TSIZ_PKTCNT_POS) | (size << OTG_FS_DEVICE_ENDPOINT_TSIZ_XFRSIZ_POS);
    else //EP0
        OTG_FS_DEVICE->OUTEP[0].TSIZ = (1 << OTG_FS_DEVICE_ENDPOINT_TSIZ_PKTCNT_POS) | (size << OTG_FS_DEVICE_ENDPOINT_TSIZ_XFRSIZ_POS) |
                                       (3 << OTG_FS_DEVICE_ENDPOINT_TSIZ_STUPCNT_OUT0_POS);
    OTG_FS_DEVICE->OUTEP[ep_num].CTL |= OTG_FS_DEVICE_ENDPOINT_CTL_EPENA | OTG_FS_DEVICE_ENDPOINT_CTL_CNAK;
}

static inline void stm32_otg_rx(SHARED_USB_DRV* drv)
{
    IPC ipc;
    unsigned int sta = OTG_FS_GENERAL->RXSTSP;
    unsigned int pktsts = sta & OTG_FS_GENERAL_RXSTSR_PKTSTS;
    int bcnt = (sta & OTG_FS_GENERAL_RXSTSR_BCNT) >> OTG_FS_GENERAL_RXSTSR_BCNT_POS;
    unsigned int ep_num = (sta & OTG_FS_GENERAL_RXSTSR_EPNUM) >> OTG_FS_GENERAL_RXSTSR_EPNUM_POS;
    EP* ep = drv->usb.out[ep_num];

    if (pktsts == OTG_FS_GENERAL_RXSTSR_PKTSTS_SETUP_RX)
    {
        //ignore all data on setup packet
        ipc.process = drv->usb.device;
        ipc.cmd = HAL_CMD(HAL_USB, USB_SETUP);
        ipc.param1 = 0;
        ipc.param2 = ((uint32_t*)(OTG_FS_FIFO_BASE + ep_num * 0x1000))[0];
        ipc.param3 = ((uint32_t*)(OTG_FS_FIFO_BASE + ep_num * 0x1000))[1];
        ipc_post(&ipc);
    }
    else if (pktsts == OTG_FS_GENERAL_RXSTSR_PKTSTS_OUT_RX)
    {
        memcpy(io_data(ep->io) + ep->io->data_size, (void*)(OTG_FS_FIFO_BASE + ep_num * 0x1000), ALIGN(bcnt));
        ep->io->data_size += bcnt;

        if (ep->io->data_size >= ep->size)
        {
            iio_complete(drv->usb.device, HAL_CMD(HAL_USB, IPC_READ), ep_num, ep->io);
            ep->io_active = false;
            ep->io = NULL;
        }
        else
            stm32_otg_rx_prepare(drv, ep_num);
    }
    OTG_FS_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_RXFLVLM;
}

static inline void stm32_otg_tx(SHARED_USB_DRV* drv, int num)
{
    EP* ep = drv->usb.in[USB_EP_NUM(num)];

    int size = ep->io->data_size - ep->size;
    if (size > ep->mps)
        size = ep->mps;
    OTG_FS_DEVICE->INEP[USB_EP_NUM(num)].TSIZ = (1 << OTG_FS_DEVICE_ENDPOINT_TSIZ_PKTCNT_POS) | (size << OTG_FS_DEVICE_ENDPOINT_TSIZ_XFRSIZ_POS);
    OTG_FS_DEVICE->INEP[USB_EP_NUM(num)].CTL |= OTG_FS_DEVICE_ENDPOINT_CTL_EPENA | OTG_FS_DEVICE_ENDPOINT_CTL_CNAK;

    memcpy((void*)(OTG_FS_FIFO_BASE + USB_EP_NUM(num) * 0x1000), io_data(ep->io) +  ep->size, ALIGN(size));
    ep->size += size;
}

USB_SPEED stm32_otg_get_speed(SHARED_USB_DRV* drv)
{
    //according to datasheet STM32F1_CL doesn't support low speed mode...
    return USB_FULL_SPEED;
}

static inline void usb_enumdne(SHARED_USB_DRV* drv)
{
    OTG_FS_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_USBSUSPM;

    IPC ipc;
    ipc.process = drv->usb.device;
    ipc.param1 = USB_HANDLE_DEVICE;
    ipc.param2 = stm32_otg_get_speed(drv);
    ipc.cmd = HAL_CMD(HAL_USB, USB_RESET);
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
        ipc.cmd = HAL_CMD(HAL_USB, STM32_USB_FIFO_RX);
        ipc.param1 = USB_HANDLE_DEVICE;
        ipc_ipost(&ipc);
        return;
    }
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
        if (drv->usb.in[i] != NULL && drv->usb.in[i]->io_active && (OTG_FS_DEVICE->INEP[i].INT & OTG_FS_DEVICE_ENDPOINT_INT_XFRC))
        {
            OTG_FS_DEVICE->INEP[i].INT = OTG_FS_DEVICE_ENDPOINT_INT_XFRC;
            if (drv->usb.in[i]->size >= drv->usb.in[i]->io->data_size)
            {
                drv->usb.in[i]->io_active = false;
                iio_complete(drv->usb.device, HAL_CMD(HAL_USB, IPC_WRITE), USB_EP_IN | i, drv->usb.in[i]->io);
                drv->usb.in[i]->io = NULL;
            }
            else
            {
                ipc.process = process_iget_current();
                ipc.cmd = HAL_CMD(HAL_USB, STM32_USB_FIFO_TX);
                ipc.param1 = USB_EP_IN | i;
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

void stm32_otg_open_device(SHARED_USB_DRV* drv, HANDLE device)
{
    int trdt;
    drv->usb.device = device;

    //enable GPIO
    ack_pin(drv, HAL_CMD(HAL_PIN, STM32_GPIO_ENABLE_PIN), A9, STM32_GPIO_MODE_INPUT_FLOAT, false);
    ack_pin(drv, HAL_CMD(HAL_PIN, STM32_GPIO_ENABLE_PIN), A10, STM32_GPIO_MODE_INPUT_PULL, true);

    //enable clock, setup prescaller
    switch (get_clock(drv, STM32_CLOCK_CORE))
    {
    case 72000000:
        RCC->CFGR &= ~(1 << 22);
        break;
    case 48000000:
        RCC->CFGR |= 1 << 22;
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        return;
    }
#if defined(STM32F10X_CL)
    //enable clock
    RCC->AHBENR |= RCC_AHBENR_OTGFSEN;
    //F100, F101, F103
#elif defined(STM32F1)
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;
#endif

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

static inline void stm32_otg_open_ep(SHARED_USB_DRV* drv, int num, USB_EP_TYPE type, unsigned int size)
{
    if (ep_data(drv, num) != NULL)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    EP* ep = malloc(sizeof(EP));
    if (ep == NULL)
        return;
    num & USB_EP_IN ? (drv->usb.in[USB_EP_NUM(num)] = ep) : (drv->usb.out[USB_EP_NUM(num)] = ep);
    ep->io = NULL;
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

static inline void stm32_otg_close_ep(SHARED_USB_DRV* drv, int num)
{
    if (!stm32_otg_ep_flush(drv, num))
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

static inline void stm32_otg_close_device(SHARED_USB_DRV* drv)
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
            stm32_otg_close_ep(drv, i);
        if (drv->usb.in[i] != NULL)
            stm32_otg_close_ep(drv, USB_EP_IN | i);
    }
    drv->usb.device = INVALID_HANDLE;

    //power down
#if defined(STM32F10X_CL)
    //disable clock
    RCC->AHBENR &= ~RCC_AHBENR_OTGFSEN;
    //F100, F101, F103
#elif defined(STM32F1)
    RCC->APB1ENR &= ~RCC_APB1ENR_USBEN;
#endif

    //disable pins
    ack_pin(drv, HAL_CMD(HAL_PIN, STM32_GPIO_DISABLE_PIN), A9, 0, 0);
    ack_pin(drv, HAL_CMD(HAL_PIN, STM32_GPIO_DISABLE_PIN), A10, 0, 0);
}

static inline void stm32_otg_set_address(int addr)
{
    OTG_FS_DEVICE->CFG &= OTG_FS_DEVICE_CFG_DAD;
    OTG_FS_DEVICE->CFG |= addr << OTG_FS_DEVICE_CFG_DAD_POS;
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

static inline void stm32_otg_read(SHARED_USB_DRV* drv, IPC* ipc)
{
    unsigned int ep_num = USB_EP_NUM(ipc->param1);
    EP* ep = drv->usb.out[ep_num];
    if (stm32_usb_io_prepare(drv, ipc))
    {
        ep->io->data_size = 0;
        ep->size = ipc->param3;
        ep->io_active = true;
        stm32_otg_rx_prepare(drv, ep_num);
    }
}

static inline void stm32_otg_write(SHARED_USB_DRV* drv, IPC* ipc)
{
    unsigned int ep_num = USB_EP_NUM(ipc->param1);
    EP* ep = drv->usb.in[ep_num];
    if (stm32_usb_io_prepare(drv, ipc))
    {
        ep->size = 0;
        ep->io_active = true;
        stm32_otg_tx(drv, USB_EP_IN | ep_num);
    }
}

#if (USB_TEST_MODE_SUPPORT)
static inline void stm32_otg_set_test_mode(SHARED_USB_DRV* drv, USB_TEST_MODES test_mode)
{
    OTG_FS_DEVICE->CTL &= ~OTG_FS_DEVICE_CTL_TCTL;
    OTG_FS_DEVICE->CTL |= test_mode << OTG_FS_DEVICE_CTL_TCTL_POS;
}
#endif //USB_TEST_MODE_SUPPORT

void stm32_otg_init(SHARED_USB_DRV* drv)
{
    int i;
    drv->usb.device = INVALID_HANDLE;
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        drv->usb.out[i] = NULL;
        drv->usb.in[i] = NULL;
    }
}

static inline bool stm32_otg_device_request(SHARED_USB_DRV* drv, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case USB_GET_SPEED:
        ipc->param2 = stm32_otg_get_speed(drv);
        need_post = true;
        break;
    case IPC_OPEN:
        stm32_otg_open_device(drv, ipc->process);
        need_post = true;
        break;
    case IPC_CLOSE:
        stm32_otg_close_device(drv);
        need_post = true;
        break;
    case USB_SET_ADDRESS:
        stm32_otg_set_address(ipc->param2);
        need_post = true;
        break;
    case STM32_USB_FIFO_RX:
        stm32_otg_rx(drv);
        //message from isr, no response
        break;
#if (USB_TEST_MODE_SUPPORT)
    case USB_SET_TEST_MODE:
        stm32_otg_set_test_mode(drv, ipc->param2);
        need_post = true;
        break;
#endif //USB_TEST_MODE_SUPPORT
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}


bool stm32_otg_ep_request(SHARED_USB_DRV* drv, IPC* ipc)
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
            stm32_otg_open_ep(drv, ipc->param1, ipc->param2, ipc->param3);
            need_post = true;
            break;
        case IPC_CLOSE:
            stm32_otg_close_ep(drv, ipc->param1);
            need_post = true;
            break;
        case IPC_FLUSH:
            stm32_otg_ep_flush(drv, ipc->param1);
            need_post = true;
            break;
        case USB_EP_SET_STALL:
            stm32_otg_ep_set_stall(drv, ipc->param1);
            need_post = true;
            break;
        case USB_EP_CLEAR_STALL:
            stm32_otg_ep_clear_stall(drv, ipc->param1);
            need_post = true;
            break;
        case USB_EP_IS_STALL:
            ipc->param2 = stm32_otg_ep_is_stall(ipc->param1);
            need_post = true;
            break;
        case IPC_READ:
            stm32_otg_read(drv, ipc);
            break;
        case IPC_WRITE:
            stm32_otg_write(drv, ipc);
            break;
        case STM32_USB_FIFO_TX:
            stm32_otg_tx(drv, ipc->param1);
            //message from isr, no response
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
            need_post = true;
            break;
        }
    return need_post;
}

bool stm32_otg_request(SHARED_USB_DRV* drv, IPC* ipc)
{
    bool need_post = false;
    if (ipc->param1 == USB_HANDLE_DEVICE)
        need_post = stm32_otg_device_request(drv, ipc);
    else
        need_post = stm32_otg_ep_request(drv, ipc);
    return need_post;
}

#if !(MONOLITH_USB)
void stm32_otg()
{
    IPC ipc;
    SHARED_USB_DRV drv;
    bool need_post;
    stm32_otg_init(&drv);
    object_set_self(SYS_OBJ_USB);
    for (;;)
    {
        ipc_read(&ipc);
        need_post = stm32_otg_request(&drv, &ipc);
        if (need_post)
            ipc_post(&ipc);
    }
}
#endif
