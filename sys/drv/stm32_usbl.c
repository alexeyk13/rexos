/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_usb.h"
#include "../sys.h"
#include "../usb.h"
#include "../../userspace/direct.h"
#include "stm32_gpio.h"
#include "stm32_power.h"
#if (SYS_INFO)
#include "../../userspace/lib/stdio.h"
#endif
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
    STM32_USB_FIFO_RX = USB_HAL_MAX,
    STM32_USB_FIFO_TX
}STM32_USB_IPCS;

#if (MONOLITH_USB)

#define _printd                 printd
#define get_clock               stm32_power_get_clock_inside
#define ack_gpio                stm32_gpio_request_inside
#define ack_power               stm32_power_request_inside

#else

#define _printd                 printf
#define get_clock               stm32_power_get_clock_outside
#define ack_gpio                stm32_core_request_outside
#define ack_power               stm32_core_request_outside

void stm32_usb();

const REX __STM32_USB = {
    //name
    "STM32 USB(L)",
    //size
    STM32_USB_PROCESS_SIZE,
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

void stm32_usb_open_device(SHARED_USB_DRV* drv, USB_OPEN* uo)
{
    int trdt;
    //enable DM/DP
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN_SYSTEM, USB_DM, GPIO_MODE_AF | GPIO_OT_PUSH_PULL | GPIO_SPEED_HIGH, AF0);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN_SYSTEM, USB_DM, GPIO_MODE_AF | GPIO_OT_PUSH_PULL | GPIO_SPEED_HIGH, AF0);

    //enable clock, setup prescaller
    ack_power(drv, STM32_POWER_USB_ON, 0, 0, 0);

    //power up and wait tStartup
    USB->CNTR &= ~USB_CNTR_PDWN;
    sleep_us(1);
    USB->CNTR &= ~USB_CNTR_FRES;

    //clear any spurious pending interrupts
    USB->ISTR = 0;
/*
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

void stm32_usb_open(SHARED_USB_DRV* drv, unsigned int handle, HANDLE process)
{
    union {
        USB_EP_OPEN ep_open;
        USB_OPEN uo;
    } u;

    if (handle == USB_HANDLE_DEVICE)
    {
        if (direct_read(process, (void*)&u.uo, sizeof(USB_OPEN)))
           stm32_usb_open_device(drv, &u.uo);
    }
    else
    {
///        if (direct_read(process, (void*)&u.ep_open, sizeof(USB_EP_OPEN)))
///            stm32_usb_open_ep(drv, process, handle, &u.ep_open);
    }
}

static inline void stm32_usb_close_device(SHARED_USB_DRV* drv)
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

static inline void stm32_usb_close(SHARED_USB_DRV* drv, unsigned int handle)
{
    if (handle == USB_HANDLE_DEVICE)
        stm32_usb_close_device(drv);
//    else
//        stm32_usb_close_ep(drv, handle);
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
#if (SYS_INFO)
    case IPC_GET_INFO:
///        stm32_usb_info(drv);
        need_post = true;
        break;
#endif
    case STM32_USB_FIFO_RX:
///        stm32_usb_rx(drv);
        //message from isr, no response
        break;
    case STM32_USB_FIFO_TX:
//        stm32_usb_tx(drv, HAL_ITEM(ipc->param1));
        //message from isr, no response
        break;
    case USB_GET_SPEED:
//        ipc->param1 = stm32_usb_get_speed(drv);
        need_post = true;
        break;
    case IPC_OPEN:
        stm32_usb_open(drv, HAL_ITEM(ipc->param1), ipc->process);
        need_post = true;
        break;
    case IPC_CLOSE:
        stm32_usb_close(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case USB_SET_ADDRESS:
//        stm32_usb_set_address(ipc->param1);
        need_post = true;
        break;
    case IPC_FLUSH:
//        stm32_usb_ep_flush(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case USB_EP_SET_STALL:
//        stm32_usb_ep_set_stall(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case USB_EP_CLEAR_STALL:
//        stm32_usb_ep_clear_stall(drv, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case USB_EP_IS_STALL:
//        ipc->param1 = stm32_usb_ep_is_stall(HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    case IPC_READ:
//        stm32_usb_read(drv, HAL_ITEM(ipc->param1), ipc->param2, ipc->param3, ipc->process);
        //generally posted with block, no return IPC
        break;
    case IPC_WRITE:
//        stm32_usb_write(drv, HAL_ITEM(ipc->param1), ipc->param2, ipc->param3, ipc->process);
        //generally posted with block, no return IPC
        break;
    case USB_SET_TEST_MODE:
//        stm32_usb_set_test_mode(drv, ipc->param1);
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
#if (SYS_INFO)
    open_stdout();
#endif
    for (;;)
    {
        error(ERROR_OK);
        need_post = false;
        ipc_read_ms(&ipc, 0, 0);
        switch (ipc.cmd)
        {
        case IPC_PING:
            need_post = true;
            break;
        case IPC_CALL_ERROR:
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
