/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_otg.h"
#if (MONOLITH_USB)
#include "lpc_core_private.h"
#endif
#include "../../userspace/irq.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/stdio.h"
#include <string.h>

#if (MONOLITH_USB)

#define ack_pin                 lpc_pin_request_inside
#define ack_power               lpc_power_request_inside

#else //!MONOLITH_USB

#define ack_pin                 lpc_core_request_outside
#define ack_power               lpc_core_request_outside

void lpc_otg();

const REX __LPC_OTG = {
    //name
    "LPC OTG",
    //size
    LPC_USB_PROCESS_SIZE,
    //priority - driver priority
    92,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //function
    lpc_otg
};

#endif //MONOLITH_USB

typedef enum {
    LPC_OTG_SYSTEM_ERROR = USB_HAL_MAX,
    LPC_OTG_TRANSACTION_ERROR
}LPC_OTG_IPCS;

#define EP_CTRL                             ((uint32_t*)(&(LPC_USB0->ENDPTCTRL0)))

#pragma pack(push, 1)
typedef struct
{
    uint32_t next;
    uint32_t size_flags;
    void* buf[5];
    uint32_t align[9];
} DTD;

typedef struct
{
    uint32_t capa;
    DTD* cur;
    DTD* next;
    uint32_t shadow_size_flags;
    void* shadow_buf[5];
    uint32_t reserved;
    uint32_t stp[2];
    uint32_t align[4];
} DQH;
#pragma pack(pop)

#define EP_BIT(num)                         (1 << (USB_EP_NUM(num) + (((num) & USB_EP_IN) ? 16 : 0)))

static inline EP* ep_data(SHARED_OTG_DRV* drv, unsigned int num)
{
    return (num & USB_EP_IN) ? (drv->otg.in[USB_EP_NUM(num)]) : (drv->otg.out[USB_EP_NUM(num)]);
}

static inline DQH* ep_dqh(unsigned int num)
{
    return &(((DQH*)LPC_USB0->ENDPOINTLISTADDR)[USB_EP_NUM(num) * 2 + ((num & USB_EP_IN) ? 1 : 0)]);
}

static inline DTD* ep_dtd(unsigned int num)
{
    return &(((DTD*)(LPC_USB0->ENDPOINTLISTADDR + sizeof(DQH) * USB_EP_COUNT_MAX * 2))[USB_EP_NUM(num) * 2 + ((num & USB_EP_IN) ? 1 : 0)]);
}

static void lpc_otg_bus_reset(SHARED_OTG_DRV* drv)
{
    int i;
    //clear all SETUP token semaphores
    LPC_USB0->ENDPTSETUPSTAT = LPC_USB0->ENDPTSETUPSTAT;
    //Clear all the endpoint complete status bits
    LPC_USB0->ENDPTCOMPLETE = LPC_USB0->ENDPTCOMPLETE;
    //Cancel all primed status
    while (LPC_USB0->ENDPTPRIME) {}
    //And flush all endpoints
    LPC_USB0->ENDPTFLUSH = 0xffffffff;
    while (LPC_USB0->ENDPTFLUSH) {}
    //Disable all EP (except EP0)
    for (i = 1; i < USB_EP_COUNT_MAX; ++i)
        EP_CTRL[i] = 0x0;

    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        ep_dqh(i)->capa = 0x0;
        ep_dqh(i)->next = (void*)USB0_DQH_NEXT_T_Msk;

        ep_dqh(USB_EP_IN | i)->capa = 0x0;
        ep_dqh(USB_EP_IN | i)->next = (void*)USB0_DQH_NEXT_T_Msk;
    }
    LPC_USB0->DEVICEADDR = 0;
}

static inline void lpc_otg_reset(SHARED_OTG_DRV* drv)
{
    IPC ipc;
    ipc.process = drv->otg.device;
    ipc.cmd = HAL_CMD(HAL_USB, USB_RESET);
    ipc.param1 = USB_HANDLE_DEVICE;
    ipc.param2 = drv->otg.speed;
    ipc_ipost(&ipc);
}

static inline void lpc_otg_suspend(SHARED_OTG_DRV* drv)
{
    IPC ipc;
    ipc.process = drv->otg.device;
    ipc.cmd = HAL_CMD(HAL_USB, USB_SUSPEND);
    ipc.param1 = USB_HANDLE_DEVICE;
    ipc_ipost(&ipc);
}

static inline void lpc_otg_wakeup(SHARED_OTG_DRV* drv)
{
    IPC ipc;
    ipc.process = drv->otg.device;
    ipc.cmd = HAL_CMD(HAL_USB, USB_WAKEUP);
    ipc.param1 = USB_HANDLE_DEVICE;
    ipc_ipost(&ipc);
}

static inline void lpc_otg_setup(SHARED_OTG_DRV* drv)
{
    IPC ipc;
    ipc.process = drv->otg.device;
    ipc.cmd = HAL_CMD(HAL_USB, USB_SETUP);
    ipc.param1 = 0;
    ipc.param2 = ep_dqh(0)->stp[0];
    ipc.param3 = ep_dqh(0)->stp[1];
    ipc_ipost(&ipc);
}

static inline void lpc_otg_out(SHARED_OTG_DRV* drv, int ep_num)
{
    EP* ep = drv->otg.out[ep_num];
    ep->io->data_size = drv->otg.read_size[ep_num] - ((ep_dtd(ep_num)->size_flags & USB0_DTD_SIZE_FLAGS_SIZE_Msk) >> USB0_DTD_SIZE_FLAGS_SIZE_Pos);
    iio_complete(drv->otg.device, HAL_CMD(HAL_USB, IPC_READ), ep_num, ep->io);
    ep->io = NULL;
}

static inline void lpc_otg_in(SHARED_OTG_DRV* drv, int ep_num)
{
    EP* ep = drv->otg.in[ep_num];
    iio_complete(drv->otg.device, HAL_CMD(HAL_USB, IPC_WRITE), USB_EP_IN | ep_num, ep->io);
    ep->io = NULL;
}

static void lpc_otg_on_isr(int vector, void* param)
{
    int i;
    SHARED_OTG_DRV* drv = (SHARED_OTG_DRV*)param;

    if (LPC_USB0->USBSTS_D & USB0_USBSTS_D_URI_Msk)
    {
        lpc_otg_bus_reset(drv);

        drv->otg.suspended = false;
        LPC_USB0->USBSTS_D = USB0_USBSTS_D_URI_Msk;
    }
    if (LPC_USB0->USBSTS_D & USB0_USBSTS_D_SLI_Msk)
    {
        lpc_otg_suspend(drv);
        drv->otg.suspended = true;
        LPC_USB0->USBSTS_D = USB0_USBSTS_D_SLI_Msk;
    }
    if (LPC_USB0->USBSTS_D & USB0_USBSTS_D_PCI_Msk)
    {
        drv->otg.speed = LPC_USB0->PORTSC1_D & USB0_PORTSC1_D_HSP_Msk ? USB_HIGH_SPEED : USB_FULL_SPEED;
        LPC_USB0->USBSTS_D = USB0_USBSTS_D_PCI_Msk;
        if (drv->otg.suspended)
        {
            drv->otg.suspended = false;
            lpc_otg_wakeup(drv);
        }
        else
            lpc_otg_reset(drv);
    }

    if (LPC_USB0->USBSTS_D & USB0_USBSTS_D_UI_Msk)
    {
        for (i = 0; LPC_USB0->ENDPTCOMPLETE && (i < USB_EP_COUNT_MAX); ++i )
        {
            if (LPC_USB0->ENDPTCOMPLETE & EP_BIT(i))
            {
                lpc_otg_out(drv, i);
                LPC_USB0->ENDPTCOMPLETE = EP_BIT(i);
            }
            if (LPC_USB0->ENDPTCOMPLETE & EP_BIT(USB_EP_IN | i))
            {
                lpc_otg_in(drv, i);
                LPC_USB0->ENDPTCOMPLETE = EP_BIT(USB_EP_IN | i);
            }
        }
        //Only for EP0
        if (LPC_USB0->ENDPTSETUPSTAT & (1 << 0))
        {
            lpc_otg_setup(drv);
            LPC_USB0->ENDPTSETUPSTAT = (1 << 0);
        }
        LPC_USB0->USBSTS_D = USB0_USBSTS_D_UI_Msk;
    }

#if (USB_DEBUG_ERRORS)
    IPC ipc;
    if (LPC_USB0->USBSTS_D & USB0_USBSTS_D_UEI_Msk)
    {
        ipc.process = process_iget_current();
        ipc.cmd = HAL_CMD(HAL_USB, LPC_OTG_TRANSACTION_ERROR);
        ipc.param1 = USB_HANDLE_DEVICE;
        ipc_ipost(&ipc);
        LPC_USB0->USBSTS_D = USB0_USBSTS_D_UEI_Msk;
    }
    if (LPC_USB0->USBSTS_D & USB0_USBSTS_D_SEI_Msk)
    {
        ipc.process = process_iget_current();
        ipc.cmd = HAL_CMD(HAL_USB, LPC_OTG_SYSTEM_ERROR);
        ipc.param1 = USB_HANDLE_DEVICE;
        ipc_ipost(&ipc);
        LPC_USB0->USBSTS_D = USB0_USBSTS_D_SEI_Msk;
    }
#endif
}

void lpc_otg_init(SHARED_OTG_DRV* drv)
{
    int i;
    drv->otg.device = INVALID_HANDLE;
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        drv->otg.out[i] = NULL;
        drv->otg.in[i] = NULL;
    }
}

static bool lpc_otg_ep_flush(SHARED_OTG_DRV* drv, int num)
{
    EP* ep = ep_data(drv, num);
    if (ep == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return false;
    }

    //flush
    LPC_USB0->ENDPTFLUSH = EP_BIT(num);
    while (LPC_USB0->ENDPTFLUSH & EP_BIT(num)) {}
    //reset sequence
    EP_CTRL[USB_EP_NUM(num)] |= USB0_ENDPTCTRL_R_Msk << ((num & USB_EP_IN) ? 16 : 0);
    if (ep->io != NULL)
    {
        io_complete_ex(drv->otg.device, HAL_CMD(HAL_USB, (num & USB_EP_IN) ? IPC_WRITE : IPC_READ), num, ep->io, ERROR_IO_CANCELLED);
        ep->io = NULL;
    }
    return true;
}

static inline void lpc_otg_ep_set_stall(SHARED_OTG_DRV* drv, int num)
{
    if (!lpc_otg_ep_flush(drv, num))
        return;
    EP_CTRL[USB_EP_NUM(num)] |= USB0_ENDPTCTRL_S_Msk << ((num & USB_EP_IN) ? 16 : 0);
}

static inline void lpc_otg_ep_clear_stall(SHARED_OTG_DRV* drv, int num)
{
    if (!lpc_otg_ep_flush(drv, num))
        return;
    EP_CTRL[USB_EP_NUM(num)] &= ~(USB0_ENDPTCTRL_S_Msk << ((num & USB_EP_IN) ? 16 : 0));
    EP_CTRL[USB_EP_NUM(num)] |= USB0_ENDPTCTRL_R_Msk << ((num & USB_EP_IN) ? 16 : 0);
}


static inline bool lpc_otg_ep_is_stall(int num)
{
    return (EP_CTRL[USB_EP_NUM(num)] & (USB0_ENDPTCTRL_S_Msk << ((num & USB_EP_IN) ? 16 : 0))) ? true : false;
}

static inline void lpc_otg_open_ep(SHARED_OTG_DRV* drv, int num, USB_EP_TYPE type, unsigned int size)
{
    unsigned int reg;
    DQH* dqh;
    if (ep_data(drv, num) != NULL)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }

    EP* ep = malloc(sizeof(EP));
    if (ep == NULL)
        return;
    ep->io = NULL;
    num & USB_EP_IN ? (drv->otg.in[USB_EP_NUM(num)] = ep) : (drv->otg.out[USB_EP_NUM(num)] = ep);

    dqh = ep_dqh(num);
    dqh->capa = (size << USB0_DQH_CAPA_MAX_PACKET_LENGTH_Pos) | USB0_DQH_CAPA_ZLT_Msk;
    if (USB_EP_NUM(num) == 0)
        dqh->capa |= USB0_DQH_CAPA_IOS_Msk;
    dqh->next = (void*)USB0_DQH_NEXT_T_Msk;

    if (USB_EP_NUM(num))
        reg = (type << USB0_ENDPTCTRL_T_Pos) | USB0_ENDPTCTRL_E_Msk | USB0_ENDPTCTRL_R_Msk;
    else
        reg = USB0_ENDPTCTRL_E_Msk;

    if (num & USB_EP_IN)
        EP_CTRL[USB_EP_NUM(num)] = (EP_CTRL[USB_EP_NUM(num)] & 0xffff) | (reg << 16);
    else
        EP_CTRL[USB_EP_NUM(num)] = (EP_CTRL[USB_EP_NUM(num)] & (0xffff << 16)) | reg;
}

static void lpc_otg_close_ep(SHARED_OTG_DRV* drv, int num)
{
    if (!lpc_otg_ep_flush(drv, num))
        return;

    EP* ep = ep_data(drv, num);
    free(ep);
    num & USB_EP_IN ? (drv->otg.in[USB_EP_NUM(num)] = NULL) : (drv->otg.out[USB_EP_NUM(num)] = NULL);
}

static void lpc_otg_io(SHARED_OTG_DRV* drv, IPC* ipc, bool read)
{
    unsigned int num = ipc->param1;
    DTD* dtd;
    DQH* dqh;
    EP* ep = ep_data(drv, num);
    if (ep == NULL)
    {
        io_send_error(ipc, ERROR_NOT_CONFIGURED);
        return;
    }
    if (ep->io != NULL)
    {
        io_send_error(ipc, ERROR_IN_PROGRESS);
        return;
    }
    ep->io = (IO*)ipc->param2;
    //prepare DTD
    dtd = ep_dtd(num);

    dtd->next = USB0_DQH_NEXT_T_Msk;
    if (read)
    {
        dtd->size_flags = ipc->param3 << USB0_DTD_SIZE_FLAGS_SIZE_Pos;
        drv->otg.read_size[USB_EP_NUM(num)] = ipc->param3;
    }
    else
        dtd->size_flags = ep->io->data_size << USB0_DTD_SIZE_FLAGS_SIZE_Pos;
    dtd->size_flags |= USB0_DTD_SIZE_FLAGS_IOC_Msk | USB0_DTD_SIZE_FLAGS_ACTIVE_Msk;

    dtd->buf[0] = io_data(ep->io);
    dtd->buf[1] = (void*)(((unsigned int)(dtd->buf[0]) + 0x1000) & 0xfffff000);
    dtd->buf[2] = dtd->buf[1] + 0x1000;
    dtd->buf[3] = dtd->buf[1] + 0x2000;
    dtd->buf[4] = dtd->buf[1] + 0x3000;
    //insert in EQH
    dqh = ep_dqh(num);
    dqh->next = dtd;
    dqh->shadow_size_flags &= ~USB0_DTD_SIZE_FLAGS_STATUS_Msk;

    dqh->shadow_size_flags &= ~0xc0;
    if (USB_EP_NUM(num) == 0)
        //required before EP0 transfers
        while (LPC_USB0->ENDPTSETUPSTAT & (1 << 0)) {}
    LPC_USB0->ENDPTPRIME = EP_BIT(num);
}

static inline void lpc_otg_open_device(SHARED_OTG_DRV* drv, HANDLE device)
{
    int i;
    drv->otg.device = device;
    drv->otg.suspended = false;
    drv->otg.speed = USB_FULL_SPEED;

    //power on. Turn USB0 PLL 0n
    LPC_CGU->PLL0USB_CTRL = CGU_PLL0USB_CTRL_PD_Msk;
    LPC_CGU->PLL0USB_CTRL |= CGU_PLL0USB_CTRL_DIRECTI_Msk | CGU_CLK_HSE | CGU_PLL0USB_CTRL_DIRECTO_Msk;
    LPC_CGU->PLL0USB_MDIV = USBPLL_M;
    LPC_CGU->PLL0USB_CTRL &= ~CGU_PLL0USB_CTRL_PD_Msk;

    //power on. USBPLL must be turned on even in case of SYS PLL used. Why?
    //wait for PLL lock
    for (i = 0; i < PLL_LOCK_TIMEOUT; ++i)
        if (LPC_CGU->PLL0USB_STAT & CGU_PLL0USB_STAT_LOCK_Msk)
            break;
    if ((LPC_CGU->PLL0USB_STAT & CGU_PLL0USB_STAT_LOCK_Msk) == 0)
    {
        error(ERROR_HARDWARE);
        return;
    }
    //enable PLL clock
    LPC_CGU->PLL0USB_CTRL |= CGU_PLL0USB_CTRL_CLKEN_Msk;

    //turn on USB0 PHY
    LPC_CREG->CREG0 &= ~CREG_CREG0_USB0PHY_Msk;

    //USB must be reset before mode change
    LPC_USB0->USBCMD_D = USB0_USBCMD_D_RST_Msk;
    //lowest interrupt latency
    LPC_USB0->USBCMD_D &= ~USB0_USBCMD_D_ITC_Msk;
    while (LPC_USB0->USBCMD_D & USB0_USBCMD_D_RST_Msk) {}
    //set device mode
    LPC_USB0->USBMODE_D = USB0_USBMODE_CM_DEVICE;

    LPC_USB0->ENDPOINTLISTADDR = SRAM1_BASE;

    memset((void*)LPC_USB0->ENDPOINTLISTADDR, 0, (sizeof(DQH) + sizeof(DTD)) * USB_EP_COUNT_MAX * 2);

    //clear any spurious pending interrupts
    LPC_USB0->USBSTS_D = USB0_USBSTS_D_UI_Msk | USB0_USBSTS_D_UEI_Msk | USB0_USBSTS_D_PCI_Msk | USB0_USBSTS_D_SEI_Msk | USB0_USBSTS_D_URI_Msk |
                         USB0_USBSTS_D_SLI_Msk;

    //enable interrupts
    irq_register(USB0_IRQn, lpc_otg_on_isr, drv);
    NVIC_EnableIRQ(USB0_IRQn);
    NVIC_SetPriority(USB0_IRQn, 1);

    //Unmask common interrupts
    LPC_USB0->USBINTR_D = USB0_USBINTR_D_UE_Msk | USB0_USBINTR_D_PCE_Msk | USB0_USBINTR_D_URE_Msk | USB0_USBINTR_D_SLE_Msk;
#if (USB_DEBUG_ERRORS)
    LPC_USB0->USBINTR_D |= USB0_USBINTR_D_UEE_Msk | USB0_USBINTR_D_SEE_Msk;
#endif

    //start
    LPC_USB0->USBCMD_D |= USB0_USBCMD_D_RS_Msk;
}

static inline void lpc_otg_close_device(SHARED_OTG_DRV* drv)
{
    int i;
    //disable interrupts
    NVIC_DisableIRQ(USB0_IRQn);
    irq_unregister(USB0_IRQn);
    //stop
    LPC_USB0->USBCMD_D &= ~USB0_USBCMD_D_RS_Msk;

    //close all endpoints
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        if (drv->otg.out[i] != NULL)
            lpc_otg_close_ep(drv, i);
        if (drv->otg.in[i] != NULL)
            lpc_otg_close_ep(drv, USB_EP_IN | i);
    }
    //Mask all interrupts
    LPC_USB0->USBINTR_D = 0;

    //turn off USB0 PHY
    LPC_CREG->CREG0 |= CREG_CREG0_USB0PHY_Msk;
    //disable USB0PLL
    LPC_CGU->PLL0USB_CTRL = CGU_PLL0USB_CTRL_PD_Msk;
}

static inline void lpc_otg_set_address(SHARED_OTG_DRV* drv, int addr)
{
    //according to datasheet, no special action is required if status will go in 2 ms
    LPC_USB0->DEVICEADDR = (addr << USB0_DEVICEADDR_USBADR_Pos) | USB0_DEVICEADDR_USBADRA_Msk;
}

#if (USB_TEST_MODE_SUPPORT)
static inline void lpc_otg_set_test_mode(SHARED_OTG_DRV* drv, USB_TEST_MODES test_mode)
{
    LPC_USB0->PORTSC1_D &= ~USB1_PORTSC1_H_PTC3_0_Msk;
    LPC_USB0->PORTSC1_D |= test_mode << USB1_PORTSC1_H_PTC3_0_Pos;
}
#endif //USB_TEST_MODE_SUPPORT

static inline bool lpc_otg_device_request(SHARED_OTG_DRV* drv, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case USB_GET_SPEED:
        ipc->param2 = drv->otg.speed;
        need_post = true;
        break;
    case IPC_OPEN:
        lpc_otg_open_device(drv, ipc->process);
        need_post = true;
        break;
    case IPC_CLOSE:
        lpc_otg_close_device(drv);
        need_post = true;
        break;
    case USB_SET_ADDRESS:
        lpc_otg_set_address(drv, ipc->param2);
        need_post = true;
        break;
#if (USB_TEST_MODE_SUPPORT)
    case USB_SET_TEST_MODE:
        lpc_otg_set_test_mode(drv, ipc->param2);
        need_post = true;
        break;
#endif //USB_TEST_MODE_SUPPORT
#if (USB_DEBUG_ERRORS)
    case LPC_OTG_SYSTEM_ERROR:
        printd("OTG driver system error\n");
        //posted from isr
        break;
    case LPC_OTG_TRANSACTION_ERROR:
        printd("OTG driver transaction error\n");
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

static inline bool lpc_otg_ep_request(SHARED_OTG_DRV* drv, IPC* ipc)
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
            lpc_otg_open_ep(drv, ipc->param1, ipc->param2, ipc->param3);
            need_post = true;
            break;
        case IPC_CLOSE:
            lpc_otg_close_ep(drv, ipc->param1);
            need_post = true;
            break;
        case IPC_FLUSH:
            lpc_otg_ep_flush(drv, ipc->param1);
            need_post = true;
            break;
        case USB_EP_SET_STALL:
            lpc_otg_ep_set_stall(drv, ipc->param1);
            need_post = true;
            break;
        case USB_EP_CLEAR_STALL:
            lpc_otg_ep_clear_stall(drv, ipc->param1);
            need_post = true;
            break;
        case USB_EP_IS_STALL:
            ipc->param2 = lpc_otg_ep_is_stall(ipc->param1);
            need_post = true;
            break;
        case IPC_READ:
            lpc_otg_io(drv, ipc, true);
            break;
        case IPC_WRITE:
            lpc_otg_io(drv, ipc, false);
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
            need_post = true;
            break;
        }
    return need_post;
}

bool lpc_otg_request(SHARED_OTG_DRV* drv, IPC* ipc)
{
    bool need_post = false;
    if (ipc->param1 == USB_HANDLE_DEVICE)
        need_post = lpc_otg_device_request(drv, ipc);
    else
        need_post = lpc_otg_ep_request(drv, ipc);
    return need_post;
}

#if !(MONOLITH_USB)
void lpc_otg()
{
    IPC ipc;
    SHARED_OTG_DRV drv;
    bool need_post;
    object_set_self(SYS_OBJ_USB);
    lpc_otg_init(&drv);
    for (;;)
    {
        ipc_read(&ipc);
        need_post = lpc_otg_request(&drv, &ipc);
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}
#endif //!MONOLITH_USB
