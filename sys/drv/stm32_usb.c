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
#include "../../userspace/lib/stdio.h"
#include <string.h>

#define ALIGN_SIZE                                          (sizeof(int))
#define ALIGN(var)                                          (((var) + (ALIGN_SIZE - 1)) & ~(ALIGN_SIZE - 1))

#define USB_TX_EP0_FIFO_SIZE                                64
#define EP_COUNT_MAX                                        4

typedef enum {
    STM32_USB_FIFO_RX = USB_LAST + 1,
    STM32_USB_FIFO_TX
}STM32_USB_IPCS;

typedef struct {
    HANDLE block;
    void* ptr;
    int size, processed;
    int mps;
    bool io_active;
    HANDLE process;
} EP;

typedef struct {
  HANDLE process;
  HANDLE device;
  EP out[4];
  EP in[4];
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
    return (num & USB_EP_IN) ? &(OTG_FS_DEVICE->INEP[USB_EP_NUM(num)]) : &(OTG_FS_DEVICE->INEP[USB_EP_NUM(num)]);
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
        block_return(ep->block);
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
            OTG_FS_GENERAL->DIEPTXF[USB_EP_NUM(num) - 1] = ((ep_open->size / 4 * 2)  << OTG_FS_GENERAL_TX0FSIZ_TX0FD_POS) | (fifo_used | OTG_FS_GENERAL_TX0FSIZ_TX0FSA_POS);
        else
            OTG_FS_GENERAL->TX0FSIZ = ((ep_open->size / 4)  << OTG_FS_GENERAL_TX0FSIZ_TX0FD_POS) | (fifo_used | OTG_FS_GENERAL_TX0FSIZ_TX0FSA_POS);
        ctl |= USB_EP_NUM(num) << OTG_FS_DEVICE_ENDPOINT_CTL_TXFNUM_POS;
        //enable interrupts for FIFO empty
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

static inline void stm32_usb_return_block(USB* usb, int num, int cmd, int size)
{
    EP* ep = ep_data(usb, num);
    HANDLE block = ep->block;
    ep->block = INVALID_HANDLE;
    block_send_ipc_inline(block, ep->process, cmd, block, num, size);
}

static inline void stm32_usb_read_chunk(USB* usb, int num)
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

static inline void stm32_usb_rx_fifo(USB* usb)
{
    IPC ipc;
    int bcnt, num;
    EP* ep;
    int sta = OTG_FS_GENERAL->RXSTSP;
    int pktsts = sta & OTG_FS_GENERAL_RXSTSR_PKTSTS;

    if (pktsts == OTG_FS_GENERAL_RXSTSR_PKTSTS_SETUP_RX || pktsts == OTG_FS_GENERAL_RXSTSR_PKTSTS_OUT_RX)
    {
        bcnt = (sta & OTG_FS_GENERAL_RXSTSR_BCNT) >> OTG_FS_GENERAL_RXSTSR_BCNT_POS;
        num = (sta & OTG_FS_GENERAL_RXSTSR_EPNUM) >> OTG_FS_GENERAL_RXSTSR_EPNUM_POS;
        ep = &usb->out[num];
        if (pktsts == OTG_FS_GENERAL_RXSTSR_PKTSTS_SETUP_RX)
        //ignore all data on setup packet
        {
            ipc.process = ep->process;
            ipc.cmd = USB_SETUP;
            ipc.param1 = ((uint32_t*)(OTG_FS_FIFO_BASE + num * 0x1000))[0];
            ipc.param2 = ((uint32_t*)(OTG_FS_FIFO_BASE + num * 0x1000))[1];
            ipc_post(&ipc);
        }
        else if (ep->size)
        {
            memcpy(ep->ptr + ep->processed, (void*)(OTG_FS_FIFO_BASE + num * 0x1000), ALIGN(bcnt));
            ep->processed += bcnt;
            if (ep->processed >= ep->size)
            {
                ep->io_active = false;
                stm32_usb_return_block(usb, num, IPC_READ_COMPLETE, ep->processed);
            }
            else
                stm32_usb_read_chunk(usb, num);
        }
        //ZLP rx
        else
        {
            ep->io_active = false;
            ipc.process = ep->process;
            ipc.cmd = IPC_READ_COMPLETE;
            ipc.param1 = INVALID_HANDLE;
            ipc.param2 = num;
            ipc.param3 = 0;
            ipc_post(&ipc);
        }
    }
    OTG_FS_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_RXFLVLM;
}

static inline void stm32_usb_write_chunk(USB* usb, int num)
{
    IPC ipc;
    EP* ep = &usb->in[USB_EP_NUM(num)];
    int size = ep->size - ep->processed;
    if (size > ep->mps)
        size = ep->mps;
    OTG_FS_DEVICE->INEP[USB_EP_NUM(num)].TSIZ = (1 << OTG_FS_DEVICE_ENDPOINT_TSIZ_PKTCNT_POS) | (size << OTG_FS_DEVICE_ENDPOINT_TSIZ_XFRSIZ_POS);
    OTG_FS_DEVICE->INEP[USB_EP_NUM(num)].CTL |= OTG_FS_DEVICE_ENDPOINT_CTL_EPENA | OTG_FS_DEVICE_ENDPOINT_CTL_CNAK;

    if (size)
    {
        memcpy((void*)(OTG_FS_FIFO_BASE + USB_EP_NUM(num) * 0x1000), ep->ptr +  ep->processed, ALIGN(size));
        ep->processed += size;

        if (ep->processed >= ep->size)
        {
            ep->io_active = false;
            stm32_usb_return_block(usb, num, IPC_WRITE_COMPLETE, ep->processed);
        }
        else
            OTG_FS_DEVICE->EIPEMPMSK |= 1 << USB_EP_NUM(num);
    }
    //ZLP tx
    else
    {
        ep->io_active = false;
        ipc.process = ep->process;
        ipc.cmd = IPC_WRITE_COMPLETE;
        ipc.param1 = INVALID_HANDLE;
        ipc.param2 = num;
        ipc.param3 = 0;
        ipc_post(&ipc);
    }
}

static inline void usb_enumdne(USB* usb)
{
    OTG_FS_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_USBSUSPM;

    IPC ipc;
    ipc.process = usb->device;
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
        if (usb->in[i].io_active && (OTG_FS_DEVICE->INEP[i].INT & OTG_FS_DEVICE_ENDPOINT_INT_TXFE))
        {
            OTG_FS_DEVICE->EIPEMPMSK &= ~(1 << i);
            ipc.process = usb->process;
            ipc.cmd = STM32_USB_FIFO_TX;
            ipc.param1 = USB_EP_IN | i;
            ipc_ipost(&ipc);
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
    core = sys_get(IPC_GET_OBJECT, SYS_OBJECT_CORE, 0, 0);
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
    //disable endpoint interrupts
    OTG_FS_DEVICE->IEPMSK = 0;
    OTG_FS_DEVICE->OEPMSK = 0;
    OTG_FS_DEVICE->CTL = OTG_FS_DEVICE_CTL_POPRGDNE;

    //Setup data FIFO
    OTG_FS_GENERAL->RXFSIZ = ((STM32_USB_MPS / 4) + 1) * 2 + 10 + 1;

    //enable interrupts
    irq_register(OTG_FS_IRQn, usb_on_isr, (void*)usb);
    NVIC_EnableIRQ(OTG_FS_IRQn);
    NVIC_SetPriority(OTG_FS_IRQn, 15);

    //Unmask global interrupts
    OTG_FS_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_OTGM;
}

static inline void stm32_usb_set_address(int addr)
{
    OTG_FS_DEVICE->CFG &= OTG_FS_DEVICE_CFG_DAD;
    OTG_FS_DEVICE->CFG |= addr << OTG_FS_DEVICE_CFG_DAD_POS;
}

static inline void stm32_usb_read(USB* usb, int num, HANDLE block, int size)
{
    if (USB_EP_NUM(num) >= EP_COUNT_MAX)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    EP* ep = &usb->out[USB_EP_NUM(num)];
    if (ep->io_active)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    //no blocks for ZLP
    if (size)
    {
        ep->block = block;
        ep->ptr = block_open(block);
    }
    ep->size = size;
    ep->processed = 0;
    ep->io_active = true;
    stm32_usb_read_chunk(usb, num);
}

static inline void stm32_usb_write(USB* usb, int num, HANDLE block, int size)
{
    int cnt;
    if (USB_EP_NUM(num) >= EP_COUNT_MAX)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    EP* ep = &usb->in[USB_EP_NUM(num)];
    if (ep->io_active)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    //no blocks for ZLP
    if (size)
    {
        ep->block = block;
        ep->ptr = block_open(block);
    }
    ep->size = size;
    ep->processed = 0;
    ep->io_active = true;

    //if there are pending packet(s) to send, wait for interrupt FIFO empty
    if (USB_EP_NUM(num))
        cnt = (OTG_FS_DEVICE->INEP[USB_EP_NUM(num)].TSIZ & OTG_FS_DEVICE_ENDPOINT_TSIZ_PKTCNT) >> OTG_FS_DEVICE_ENDPOINT_TSIZ_PKTCNT_POS;
    else
        cnt = (OTG_FS_DEVICE->INEP[USB_EP_NUM(num)].TSIZ & OTG_FS_DEVICE_ENDPOINT_TSIZ_PKTCNT_IN0) >> OTG_FS_DEVICE_ENDPOINT_TSIZ_PKTCNT_POS;
    if (cnt)
        OTG_FS_DEVICE->EIPEMPMSK |= 1 << (USB_EP_NUM(num));
    else
        stm32_usb_write_chunk(usb, num);
}

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
    open_stdout();
    for (;;)
    {
        ipc_read_ms(&ipc, 0, 0);
        error(ERROR_OK);
        switch (ipc.cmd)
        {
        case IPC_PING:
            ipc_post(&ipc);
            break;
#if (SYS_INFO)
        case IPC_GET_INFO:
//            stm32_usb_info();
            ipc_post(&ipc);
            break;
#endif
        case STM32_USB_FIFO_RX:
            stm32_usb_rx_fifo(&usb);
            break;
        case STM32_USB_FIFO_TX:
            stm32_usb_write_chunk(&usb, ipc.param1);
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
            {
                //TODO
                error(ERROR_NOT_SUPPORTED);
            }
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
            stm32_usb_read(&usb, ipc.param1, (HANDLE)ipc.param2, ipc.param3);
            //generally posted with block, no return IPC
            break;
        case IPC_WRITE:
            stm32_usb_write(&usb, ipc.param1, (HANDLE)ipc.param2, ipc.param3);
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
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}
