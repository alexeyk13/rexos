/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_usb.h"
#include "stm32_config.h"
#include "stm32_regsusb.h"
#include "stm32_gpio.h"
#include "stm32_power.h"
#include "../sys.h"
#include "../usb.h"
#include "../../userspace/error.h"
#include "../../userspace/irq.h"
#include "../../userspace/block.h"
#include "../../userspace/direct.h"
#include "../../userspace/object.h"
#include "../../userspace/lib/stdio.h"
#include <string.h>

#define ALIGN_SIZE                                          (sizeof(int))
#define ALIGN(var)                                          (((var) + (ALIGN_SIZE - 1)) & ~(ALIGN_SIZE - 1))

#define USB_TX_EP0_FIFO_SIZE                                64
#define EP_COUNT_MAX                                        4


#if (SYS_INFO)
const char* const EP_TYPES[] =                              {"CONTROL", "ISOCHRON", "BULK", "INTERRUPT"};
#endif

typedef enum {
    STM32_USB_FIFO_RX = USB_LAST + 1,
    STM32_USB_FIFO_TX
}STM32_USB_IPCS;

typedef struct {
    HANDLE block;
    void* ptr;
    unsigned int size, processed;
    unsigned int mps;
    bool io_active;
    HANDLE process;
} EP;

typedef struct {
  HANDLE process;
  HANDLE device;
  EP out[EP_COUNT_MAX];
  EP in[EP_COUNT_MAX];
} USB;

void stm32_usb();

const REX __STM32_USB = {
    //name
    "STM32 USB",
    //size
    1024,
    //priority - driver priority
    92,
    //flags
    PROCESS_FLAGS_ACTIVE,
    //ipc size
    10,
    //function
    stm32_usb
};

static inline OTG_FS_DEVICE_ENDPOINT_TypeDef* ep_reg_data(int num)
{
    return (num & USB_EP_IN) ? &(OTG_FS_DEVICE->INEP[USB_EP_NUM(num)]) : &(OTG_FS_DEVICE->OUTEP[USB_EP_NUM(num)]);
}

static inline EP* ep_data(USB* usb, int num)
{
    return (num & USB_EP_IN) ? &(usb->in[USB_EP_NUM(num)]) : &(usb->out[USB_EP_NUM(num)]);
}

bool stm32_usb_ep_flush(USB* usb, int num)
{
    if (USB_EP_NUM(num) >= EP_COUNT_MAX)
    {
        error(ERROR_INVALID_PARAMS);
        return false;
    }
    EP* ep = ep_data(usb, num);
    if (ep->block != INVALID_HANDLE)
    {
        ep_reg_data(num)->CTL |= OTG_FS_DEVICE_ENDPOINT_CTL_SNAK;
        block_send(ep->block, ep->process);
        ep->block = INVALID_HANDLE;
    }
    ep->io_active = false;
    return true;
}

static inline void stm32_usb_ep_close(USB* usb, int num)
{
    if (!stm32_usb_ep_flush(usb, num))
        return;
    ep_reg_data(num)->CTL &= ~OTG_FS_DEVICE_ENDPOINT_CTL_USBAEP;
    ep_data(usb, num)->mps = 0;
    if (num & USB_EP_IN)
    {
        OTG_FS_DEVICE->AINTMSK &= ~(1 << USB_EP_NUM(num));
        OTG_FS_DEVICE->EIPEMPMSK &= ~(1 << USB_EP_NUM(num));
    }
}

static inline void stm32_usb_ep_open(USB* usb, HANDLE process, int num, USB_EP_OPEN* ep_open)
{
    EP* ep = ep_data(usb, num);
    if (!stm32_usb_ep_flush(usb, num))
        return;
    int fifo_used, i;
    //enable, NAK
    uint32_t ctl = OTG_FS_DEVICE_ENDPOINT_CTL_USBAEP | OTG_FS_DEVICE_ENDPOINT_CTL_SNAK;
    //setup ep type, DATA0 for bulk/interrupt, EVEN frame for isochronous endpoint
    switch (ep_open->type)
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
            fifo_used += usb->in[i].mps / 4;
        if (USB_EP_NUM(num))
            OTG_FS_GENERAL->DIEPTXF[USB_EP_NUM(num) - 1] = ((ep_open->size / 4)  << OTG_FS_GENERAL_TX0FSIZ_TX0FD_POS) | ((fifo_used * 4) | OTG_FS_GENERAL_TX0FSIZ_TX0FSA_POS);
        else
            OTG_FS_GENERAL->TX0FSIZ = ((ep_open->size / 4)  << OTG_FS_GENERAL_TX0FSIZ_TX0FD_POS) | ((fifo_used * 4) | OTG_FS_GENERAL_TX0FSIZ_TX0FSA_POS);
        ctl |= USB_EP_NUM(num) << OTG_FS_DEVICE_ENDPOINT_CTL_TXFNUM_POS;
        //enable interrupts for XFRCM
        OTG_FS_DEVICE->AINTMSK |= 1 << USB_EP_NUM(num);
    }

    //EP_OUT0 has differrent mps structure
    if (USB_EP_NUM(num) == 0)
    {
        switch (ep_open->size)
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
        ctl |= ep_open->size;

    ep->mps = ep_open->size;
    ep->process = process;
    ep_reg_data(num)->CTL = ctl;
}

void stm32_usb_ep_set_stall(USB* usb, int num)
{
    if (!stm32_usb_ep_flush(usb, num))
        return;
    ep_reg_data(num)->CTL |= OTG_FS_DEVICE_ENDPOINT_CTL_STALL;
}

void stm32_usb_ep_clear_stall(USB* usb, int num)
{
    if (!stm32_usb_ep_flush(usb, num))
        return;
    ep_reg_data(num)->CTL &= ~OTG_FS_DEVICE_ENDPOINT_CTL_STALL;
}


bool stm32_usb_ep_is_stall(int num)
{
    if (USB_EP_NUM(num) >= EP_COUNT_MAX)
    {
        error(ERROR_INVALID_PARAMS);
        return false;
    }
    return ep_reg_data(num)->CTL & OTG_FS_DEVICE_ENDPOINT_CTL_STALL ? true : false;
}

static inline void usb_suspend(USB* usb)
{
    IPC ipc;
    OTG_FS_GENERAL->INTMSK &= ~OTG_FS_GENERAL_INTMSK_USBSUSPM;
    ipc.process = usb->device;
    ipc.cmd = USB_SUSPEND;
    ipc_ipost(&ipc);
}

static inline void usb_wakeup(USB* usb)
{
    IPC ipc;
    OTG_FS_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_USBSUSPM;
    ipc.process = usb->device;
    ipc.cmd = USB_WAKEUP;
    ipc_ipost(&ipc);
}

static inline void stm32_usb_rx_prepare(USB* usb, int num)
{
    EP* ep = &usb->out[USB_EP_NUM(num)];
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

static inline void stm32_usb_rx(USB* usb)
{
    IPC ipc;
    unsigned int sta = OTG_FS_GENERAL->RXSTSP;
    unsigned int pktsts = sta & OTG_FS_GENERAL_RXSTSR_PKTSTS;
    int bcnt = (sta & OTG_FS_GENERAL_RXSTSR_BCNT) >> OTG_FS_GENERAL_RXSTSR_BCNT_POS;
    unsigned int num = (sta & OTG_FS_GENERAL_RXSTSR_EPNUM) >> OTG_FS_GENERAL_RXSTSR_EPNUM_POS;
    EP* ep = &usb->out[num];

    if (pktsts == OTG_FS_GENERAL_RXSTSR_PKTSTS_SETUP_RX)
    {
        //ignore all data on setup packet
        ipc.process = ep->process;
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
            ipc.process = ep->process;
            ipc.cmd = IPC_READ_COMPLETE;
            ipc.param1 = num;
            ipc.param2 = ep->block;
            ipc.param3 = ep->processed;

            if (ep->block != INVALID_HANDLE)
            {
                block_send_ipc(ep->block, ep->process, &ipc);
                ep->block = INVALID_HANDLE;
            }
            else
                ipc_post(&ipc);
            ep->io_active = false;
        }
        else
            stm32_usb_rx_prepare(usb, num);
    }
    OTG_FS_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_RXFLVLM;
}

static inline void stm32_usb_tx(USB* usb, int num)
{
    EP* ep = &usb->in[USB_EP_NUM(num)];

    int size = ep->size - ep->processed;
    if (size > ep->mps)
        size = ep->mps;
    OTG_FS_DEVICE->INEP[USB_EP_NUM(num)].TSIZ = (1 << OTG_FS_DEVICE_ENDPOINT_TSIZ_PKTCNT_POS) | (size << OTG_FS_DEVICE_ENDPOINT_TSIZ_XFRSIZ_POS);
    OTG_FS_DEVICE->INEP[USB_EP_NUM(num)].CTL |= OTG_FS_DEVICE_ENDPOINT_CTL_EPENA | OTG_FS_DEVICE_ENDPOINT_CTL_CNAK;

    memcpy((void*)(OTG_FS_FIFO_BASE + USB_EP_NUM(num) * 0x1000), ep->ptr +  ep->processed, ALIGN(size));
    ep->processed += size;
}

USB_SPEED stm32_usb_get_speed(USB* usb)
{
    //according to datasheet STM32F1_CL doesn't support low speed mode...
    return USB_FULL_SPEED;
}

static inline void usb_enumdne(USB* usb)
{
    OTG_FS_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_USBSUSPM;

    IPC ipc;
    ipc.process = usb->device;
    ipc.param1 = stm32_usb_get_speed(usb);
    ipc.cmd = USB_RESET;
    ipc_ipost(&ipc);
}

void usb_on_isr(int vector, void* param)
{
    IPC ipc;
    int i;
    USB* usb = (USB*)param;
    unsigned int sta = OTG_FS_GENERAL->INTSTS;

    //first two most often called
    if ((OTG_FS_GENERAL->INTMSK & OTG_FS_GENERAL_INTMSK_RXFLVLM) && (sta & OTG_FS_GENERAL_INTSTS_RXFLVL))
    {
        //mask interrupts, will be umasked by process after FIFO read
        OTG_FS_GENERAL->INTMSK &= ~OTG_FS_GENERAL_INTMSK_RXFLVLM;
        ipc.process = usb->process;
        ipc.cmd = STM32_USB_FIFO_RX;
        ipc_ipost(&ipc);
        return;
    }
    for (i = 0; i < EP_COUNT_MAX; ++i)
        if (usb->in[i].io_active && (OTG_FS_DEVICE->INEP[i].INT & OTG_FS_DEVICE_ENDPOINT_INT_XFRC))
        {
            OTG_FS_DEVICE->INEP[i].INT = OTG_FS_DEVICE_ENDPOINT_INT_XFRC;
            if (usb->in[i].processed >= usb->in[i].size)
            {
                ipc.process = usb->in[i].process;
                ipc.cmd = IPC_WRITE_COMPLETE;
                ipc.param1 = USB_EP_IN | i;
                ipc.param2 = usb->in[i].block;
                ipc.param3 = usb->in[i].processed;
                if (usb->in[i].block != INVALID_HANDLE)
                {
                    block_isend_ipc(usb->in[i].block, usb->in[i].process, &ipc);
                    usb->in[i].block = INVALID_HANDLE;
                }
                else
                    ipc_ipost(&ipc);
                usb->in[i].io_active = false;

            }
            else
            {
                ipc.process = usb->process;
                ipc.cmd = STM32_USB_FIFO_TX;
                ipc.param1 = USB_EP_IN | i;
                ipc_ipost(&ipc);
            }
            return;
        }
    //rarely called
    if (sta & OTG_FS_GENERAL_INTSTS_ENUMDNE)
    {
        usb_enumdne(usb);
        OTG_FS_GENERAL->INTSTS |= OTG_FS_GENERAL_INTSTS_ENUMDNE;
        return;
    }
    if ((sta & OTG_FS_GENERAL_INTSTS_USBSUSP) && (OTG_FS_GENERAL->INTMSK & OTG_FS_GENERAL_INTMSK_USBSUSPM))
    {
        usb_suspend(usb);
        OTG_FS_GENERAL->INTSTS |= OTG_FS_GENERAL_INTSTS_USBSUSP;
        return;
    }
    if (sta & OTG_FS_GENERAL_INTSTS_WKUPINT)
    {
        usb_wakeup(usb);
        OTG_FS_GENERAL->INTSTS |= OTG_FS_GENERAL_INTSTS_WKUPINT | OTG_FS_GENERAL_INTSTS_USBSUSP;
    }
}

void stm32_usb_open(USB* usb)
{
    HANDLE core;
    int trdt;
    //enable GPIO
    core = object_get(SYS_OBJ_CORE);
    if (core == INVALID_HANDLE)
    {
        error(ERROR_NOT_FOUND);
        return;
    }
    if (usb->device == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    ack(core, STM32_GPIO_ENABLE_PIN, A9, PIN_MODE_IN_FLOAT, 0);
    ack(core, STM32_GPIO_ENABLE_PIN, A10, PIN_MODE_IN_PULLUP, 0);

    //enable clock, setup prescaller
    ack(core, STM32_POWER_USB_ON, 0, 0, 0);

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
    trdt = get(core, STM32_POWER_GET_CLOCK, STM32_CLOCK_CORE, 0, 0) == 72000000 ? 9 : 5;
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
    irq_register(OTG_FS_IRQn, usb_on_isr, (void*)usb);
    NVIC_EnableIRQ(OTG_FS_IRQn);
    NVIC_SetPriority(OTG_FS_IRQn, 13);

    //Unmask global interrupts
    OTG_FS_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_OTGM;
}

static inline void stm32_usb_close(USB* usb)
{
    HANDLE core = object_get(SYS_OBJ_CORE);
    if (core == INVALID_HANDLE)
    {
        error(ERROR_NOT_FOUND);
        return;
    }
    int i;
    //Mask global interrupts
    OTG_FS_GENERAL->INTMSK &= ~OTG_FS_GENERAL_INTMSK_OTGM;

    //disable interrupts
    NVIC_DisableIRQ(OTG_FS_IRQn);
    irq_unregister(OTG_FS_IRQn);

    //close all endpoints
    for (i = 0; i < EP_COUNT_MAX; ++i)
    {
        stm32_usb_ep_close(usb, i);
        stm32_usb_ep_close(usb, USB_EP_IN | i);
    }

    //power down
    ack(core, STM32_POWER_USB_OFF, 0, 0, 0);

    //disable pins
    ack(core, STM32_GPIO_DISABLE_PIN, A9, 0, 0);
    ack(core, STM32_GPIO_DISABLE_PIN, A10, 0, 0);
}

static inline void stm32_usb_set_address(int addr)
{
    OTG_FS_DEVICE->CFG &= OTG_FS_DEVICE_CFG_DAD;
    OTG_FS_DEVICE->CFG |= addr << OTG_FS_DEVICE_CFG_DAD_POS;
}

static inline void stm32_usb_read(USB* usb, IPC* ipc)
{
    if (USB_EP_NUM(ipc->param1) >= EP_COUNT_MAX)
    {
        block_send((HANDLE)ipc->param2, ipc->process);
        ipc_post_error(ipc->process, ERROR_INVALID_PARAMS);
        return;
    }
    EP* ep = &usb->out[USB_EP_NUM(ipc->param1)];
    if (ep->io_active)
    {
        block_send((HANDLE)ipc->param2, ipc->process);
        ipc_post_error(ipc->process, ERROR_IN_PROGRESS);
        return;
    }
    //no blocks for ZLP
    ep->size = ipc->param3;
    if (ep->size)
    {
        ep->block = (HANDLE)ipc->param2;
        if ((ep->ptr = block_open(ep->block)) == NULL)
        {
            block_send((HANDLE)ipc->param2, ipc->process);
            ipc_post_error(ipc->process, get_last_error());
            return;
        }
    }
    ep->processed = 0;
    ep->io_active = true;
    stm32_usb_rx_prepare(usb, ipc->param1);
}

static inline void stm32_usb_write(USB* usb, IPC* ipc)
{
    if (USB_EP_NUM(ipc->param1) >= EP_COUNT_MAX || ((USB_EP_IN & ipc->param1) == 0))
    {
        block_send((HANDLE)ipc->param2, ipc->process);
        ipc_post_error(ipc->process, ERROR_INVALID_PARAMS);
        return;
    }
    EP* ep = &usb->in[USB_EP_NUM(ipc->param1)];
    if (ep->io_active)
    {
        block_send((HANDLE)ipc->param2, ipc->process);
        ipc_post_error(ipc->process, ERROR_IN_PROGRESS);
        return;
    }
    ep->size = ipc->param3;
    //no blocks for ZLP
    if (ep->size)
    {
        ep->block = ipc->param2;
        if ((ep->ptr = block_open(ep->block)) == NULL)
        {
            block_send((HANDLE)ipc->param2, ipc->process);
            ipc_post_error(ipc->process, get_last_error());
            return;
        }
    }
    ep->processed = 0;
    ep->io_active = true;

    stm32_usb_tx(usb, ipc->param1);
}

static inline void stm32_usb_set_test_mode(USB* usb, USB_TEST_MODES test_mode)
{
    OTG_FS_DEVICE->CTL &= ~OTG_FS_DEVICE_CTL_TCTL;
    OTG_FS_DEVICE->CTL |= test_mode << OTG_FS_DEVICE_CTL_TCTL_POS;
}

#if (SYS_INFO)
static inline void stm32_usb_info(USB* usb)
{
    int i;
    printf("STM32 USB driver info\n\r\n\r");
    printf("State: ");
    if (OTG_FS_DEVICE->STS & 1)
    {
        printf("suspended\n\r");
        return;
    }
    printf("device\n\r");
    printf("Speed: FULL SPEED, address: %d\n\r", (OTG_FS_DEVICE->CFG & OTG_FS_DEVICE_CFG_DAD) >> OTG_FS_DEVICE_CFG_DAD_POS);
    printf("Active endpoints:\n\r");
    for (i = 0; i < EP_COUNT_MAX; ++i)
    {
        if (OTG_FS_DEVICE->OUTEP[i].CTL & OTG_FS_DEVICE_ENDPOINT_CTL_USBAEP)
            printf("OUT%d: %s %d bytes\n\r", i, EP_TYPES[(OTG_FS_DEVICE->OUTEP[i].CTL & OTG_FS_DEVICE_ENDPOINT_CTL_EPTYP) >> OTG_FS_DEVICE_ENDPOINT_CTL_EPTYP_POS], usb->out[i].mps);
        if (OTG_FS_DEVICE->INEP[i].CTL & OTG_FS_DEVICE_ENDPOINT_CTL_USBAEP)
            printf("IN%d: %s %d bytes\n\r", i, EP_TYPES[(OTG_FS_DEVICE->INEP[i].CTL & OTG_FS_DEVICE_ENDPOINT_CTL_EPTYP) >> OTG_FS_DEVICE_ENDPOINT_CTL_EPTYP_POS], usb->in[i].mps);
    }
}
#endif

void stm32_usb()
{
    IPC ipc;
    USB usb;
    int i;
    USB_EP_OPEN ep_open;
    usb.device = INVALID_HANDLE;
    usb.process = process_get_current();
    for (i = 0; i < 4; ++i)
    {
        usb.out[i].block = INVALID_HANDLE;
        usb.out[i].mps = 0;
        usb.out[i].io_active = false;
        usb.in[i].block = INVALID_HANDLE;
        usb.in[i].mps = 0;
        usb.in[i].io_active = false;
    }
#if (SYS_INFO)
    open_stdout();
#endif
    for (;;)
    {
        error(ERROR_OK);
        ipc_read_ms(&ipc, 0, 0);
        switch (ipc.cmd)
        {
        case IPC_PING:
            ipc_post(&ipc);
            break;
#if (SYS_INFO)
        case IPC_GET_INFO:
            stm32_usb_info(&usb);
            ipc_post(&ipc);
            break;
#endif
        case STM32_USB_FIFO_RX:
            stm32_usb_rx(&usb);
            break;
        case STM32_USB_FIFO_TX:
            stm32_usb_tx(&usb, ipc.param1);
            break;
        case USB_GET_SPEED:
            ipc.param1 = stm32_usb_get_speed(&usb);
            ipc_post_or_error(&ipc);
            break;
        case IPC_OPEN:
            if (ipc.param1 == USB_HANDLE_DEVICE)
                stm32_usb_open(&usb);
            else
            {
                if (direct_read(ipc.process, (void*)&ep_open, sizeof(USB_EP_OPEN)))
                    stm32_usb_ep_open(&usb, ipc.process, ipc.param1, &ep_open);
                else
                    error(ERROR_INVALID_PARAMS);
            }
            ipc_post_or_error(&ipc);
            break;
        case IPC_CLOSE:
            if (ipc.param1 == USB_HANDLE_DEVICE)
                stm32_usb_close(&usb);
            else
                stm32_usb_ep_close(&usb, ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        case USB_SET_ADDRESS:
            stm32_usb_set_address(ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        case IPC_FLUSH:
            stm32_usb_ep_flush(&usb, ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        case USB_EP_SET_STALL:
            stm32_usb_ep_set_stall(&usb, ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        case USB_EP_CLEAR_STALL:
            stm32_usb_ep_clear_stall(&usb, ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        case USB_EP_IS_STALL:
            ipc.param1 = stm32_usb_ep_is_stall(ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        case IPC_READ:
            stm32_usb_read(&usb, &ipc);
            //generally posted with block, no return IPC
            break;
        case IPC_WRITE:
            stm32_usb_write(&usb, &ipc);
            //generally posted with block, no return IPC
            break;
        case USB_REGISTER_DEVICE:
            if (usb.device == INVALID_HANDLE)
                usb.device = ipc.process;
            else
                error(ERROR_ACCESS_DENIED);
            ipc_post_or_error(&ipc);
            break;
        case USB_UNREGISTER_DEVICE:
            if (usb.device == ipc.process)
                usb.device = INVALID_HANDLE;
            else
                error(ERROR_ACCESS_DENIED);
            ipc_post_or_error(&ipc);
            break;
        case USB_SET_TEST_MODE:
            stm32_usb_set_test_mode(&usb, ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}
