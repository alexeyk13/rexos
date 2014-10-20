/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_usb.h"
#include "../sys.h"
#include "../usb.h"

typedef struct {
  HANDLE process;
  HANDLE device;
} USB_STRUCT;


void stm32_usb();

const REX __STM32_USB = {
    //name
    "STM32 USB(L)",
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

void stm32_usb_open(USB_STRUCT* usb)
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
/*    ack(core, STM32_GPIO_ENABLE_PIN, A9, PIN_MODE_IN_FLOAT, 0);
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
    NVIC_SetPriority(OTG_FS_IRQn, 15);

    //Unmask global interrupts
    OTG_FS_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_OTGM;*/
}

static inline void stm32_usb_close(USB_STRUCT* usb)
{
/*    HANDLE core = sys_get(IPC_GET_OBJECT, SYS_OBJECT_CORE, 0, 0);
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
    ack(core, STM32_GPIO_DISABLE_PIN, A10, 0, 0);*/
}



void stm32_usb()
{
    IPC ipc;
    USB_STRUCT usb;
    usb.device = INVALID_HANDLE;
    usb.process = process_get_current();
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
/*        case STM32_USB_FIFO_RX:
            stm32_usb_rx(&usb);
            break;
        case STM32_USB_FIFO_TX:
            stm32_usb_tx(&usb, ipc.param1);
            break;*/
        case USB_GET_SPEED:
//            ipc.param1 = stm32_usb_get_speed(&usb);
//            ipc_post_or_error(&ipc);
            break;
        case IPC_OPEN:
            if (ipc.param1 == USB_HANDLE_DEVICE)
                stm32_usb_open(&usb);
            else
            {
//                if (direct_read(ipc.process, (void*)&ep_open, sizeof(USB_EP_OPEN)))
//                    stm32_usb_ep_open(&usb, ipc.process, ipc.param1, &ep_open);
//                else
                    error(ERROR_INVALID_PARAMS);
            }
            ipc_post_or_error(&ipc);
            break;
        case IPC_CLOSE:
/*            if (ipc.param1 == USB_HANDLE_DEVICE)
                stm32_usb_close(&usb);
            else
                stm32_usb_ep_close(&usb, ipc.param1);*/
            ipc_post_or_error(&ipc);
            break;
        case USB_SET_ADDRESS:
//            stm32_usb_set_address(ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        case IPC_FLUSH:
//            stm32_usb_ep_flush(&usb, ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        case USB_EP_SET_STALL:
//            stm32_usb_ep_set_stall(&usb, ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        case USB_EP_CLEAR_STALL:
//            stm32_usb_ep_clear_stall(&usb, ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        case USB_EP_IS_STALL:
//            ipc.param1 = stm32_usb_ep_is_stall(ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        case IPC_READ:
//            stm32_usb_read(&usb, &ipc);
            //generally posted with block, no return IPC
            break;
        case IPC_WRITE:
//            stm32_usb_write(&usb, &ipc);
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
//            stm32_usb_set_test_mode(&usb, ipc.param1);
            ipc_post_or_error(&ipc);
            break;
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}
