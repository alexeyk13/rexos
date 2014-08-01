/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_usb.h"
#include "stm32_gpio.h"
#include "stm32_power.h"
#include "../sys.h"
#include "../../userspace/error.h"
#include "../../userspace/irq.h"
#include "stm32_regsusb.h"

typedef struct {
  HANDLE clas;
} USB;

void stm32_usb();

const REX __STM32_USB = {
    //name
    "STM32 USB",
    //size
    2048,
    //priority - driver priority
    92,
    //flags
    PROCESS_FLAGS_ACTIVE,
    //ipc size
    10,
    //function
    stm32_usb
};

void usb_on_isr(int vector, void* param)
{
    IPC ipc;
    USB* usb = (USB*)param;
    unsigned int sta = OTG_FS_GENERAL->INTSTS;

//	if (sta & OTG_FS_GENERAL_INTSTS_OEPINT)
//		usb_on_ep_out(USB_1, EP_OUT(31 - __CLZ((OTG_FS_DEVICE->AINT) & 0xffff0000) - 16));
//	if (sta & OTG_FS_GENERAL_INTSTS_IEPINT)
//		usb_on_ep_in(USB_1, EP_IN(31 - __CLZ((OTG_FS_DEVICE->AINT) & 0x0000ffff)));
//	else
    if (sta & OTG_FS_GENERAL_INTSTS_USBRST)
    {
//		usb_on_reset(USB_1);
        ipc.process = usb->clas;
        ipc.cmd = SYS_USB_RESET;
        ipc_ipost(&ipc);
        OTG_FS_GENERAL->INTSTS |= OTG_FS_GENERAL_INTSTS_USBRST;
    }
    else if (sta & OTG_FS_GENERAL_INTSTS_WKUPINT)
    {
//		_usb_handlers[USB_1]->cb->usbd_on_resume(_usb_handlers[USB_1]->param);
        OTG_FS_GENERAL->INTSTS |= OTG_FS_GENERAL_INTSTS_WKUPINT;
    }
    else if (sta & OTG_FS_GENERAL_INTSTS_USBSUSP)
    {
//		_usb_handlers[USB_1]->cb->usbd_on_suspend(_usb_handlers[USB_1]->param);
        OTG_FS_GENERAL->INTSTS |= OTG_FS_GENERAL_INTSTS_USBSUSP;
    }
    else if (sta & OTG_FS_GENERAL_INTSTS_ESUSP)
    {
//		_usb_handlers[USB_1]->cb->usbd_on_suspend(_usb_handlers[USB_1]->param);
        OTG_FS_GENERAL->INTSTS |= OTG_FS_GENERAL_INTSTS_ESUSP;
    }
}

void stm32_usb_enable(USB* usb)
{
    HANDLE core;
    int trdt;
    //enable GPIO
    core = sys_get(SYS_GET_OBJECT, SYS_OBJECT_CORE, 0, 0);
    if (core == INVALID_HANDLE)
    {
        error(ERROR_NOT_FOUND);
        return;
    }
    if (usb->clas == INVALID_HANDLE)
    {
        error(ERRO_USB_CLASS_NOT_SET);
        return;
    }
    ack(core, STM32_GPIO_ENABLE_PIN, A9, PIN_MODE_IN_FLOAT, 0);
    ack(core, STM32_GPIO_ENABLE_PIN, A10, PIN_MODE_IN_PULLUP, 0);

    //enable clock, setup prescaller
    ack(core, STM32_POWER_USB_ON, 0, 0, 0);

    //refer to programming manual: 1.
    OTG_FS_GENERAL->AHBCFG = (OTG_FS_GENERAL_AHBCFG_GINT) | (OTG_FS_GENERAL_AHBCFG_TXFELVL);
    trdt = get(core, STM32_POWER_GET_CLOCK, STM32_CLOCK_CORE, 0, 0) == 72000000 ? 9 : 5;
    //2.
    OTG_FS_GENERAL->USBCFG = OTG_FS_GENERAL_USBCFG_FDMOD | (trdt << OTG_FS_GENERAL_USBCFG_TRDT_POS) | OTG_FS_GENERAL_USBCFG_PHYSEL | (17 << OTG_FS_GENERAL_USBCFG_TOCAL_POS);
    //3.
    OTG_FS_GENERAL->INTMSK = OTG_FS_GENERAL_INTMSK_OTGM | OTG_FS_GENERAL_INTMSK_MMISM;

    //Device init: 1.
    OTG_FS_DEVICE->CFG = OTG_FS_DEVICE_CFG_DSPD_FS;
    //2.
    OTG_FS_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_USBRSTM | OTG_FS_GENERAL_INTMSK_USBSUSPM | OTG_FS_GENERAL_INTMSK_ESUSPM | OTG_FS_GENERAL_INTMSK_ENUMDNEM;
    //3.
    OTG_FS_GENERAL->CCFG = OTG_FS_GENERAL_CCFG_VBUSBSEN;

    //enable interrupts
    irq_register(OTG_FS_IRQn, usb_on_isr, (void*)usb);
    NVIC_EnableIRQ(OTG_FS_IRQn);
    NVIC_SetPriority(OTG_FS_IRQn, 15);

}

void stm32_usb()
{
    IPC ipc;
    USB usb;
    usb.clas = INVALID_HANDLE;
    sys_ack(SYS_SET_OBJECT, SYS_OBJECT_USB, 0, 0);
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
        case SYS_GET_INFO:
//            stm32_usb_info();
            ipc_post(&ipc);
            break;
#endif
        case SYS_USB_ENABLE:
            stm32_usb_enable(&usb);
            ipc_post_or_error(&ipc);
            break;
        case SYS_USB_REGISTER_CLASS:
            if (usb.clas == INVALID_HANDLE)
                usb.clas = ipc.process;
            else
                error(ERROR_ACCESS_DENIED);
            ipc_post_or_error(&ipc);
            break;
        case SYS_USB_UNREGISTER_CLASS:
            if (usb.clas == ipc.process)
                usb.clas = INVALID_HANDLE;
            else
                error(ERROR_ACCESS_DENIED);
            ipc_post_or_error(&ipc);
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}
