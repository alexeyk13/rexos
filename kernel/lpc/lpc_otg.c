/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_otg.h"
#include "lpc_exo_private.h"
#include "../kirq.h"
#include "../kstdlib.h"
#include "../kerror.h"
#include "../../userspace/stdio.h"
#include "lpc_pin.h"
#include <string.h>

//each is 16KB, 96KB limited by LPC18xx RAM size.
#define USB_DTD_COUNT           6
#define USB_DTD_CHUNK           0x4000

typedef LPC_USB0_Type* LPC_USB_Type_P;
#if defined(LPC183x) || defined(LPC185x)
static const uint8_t __USB_VECTORS[] =                          {8, 9};
static const LPC_USB_Type_P __USB_REGS[] =                      {LPC_USB0, (LPC_USB_Type_P)LPC_USB1};
#else
static const uint8_t __USB_VECTORS[] =                          {8};
static const LPC_USB_Type_P __USB_REGS[] =                      {LPC_USB0};
#endif

#define EP_CTRL(port)                       ((uint32_t*)(&(__USB_REGS[port]->ENDPTCTRL0)))

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

static inline EP* ep_data(USB_PORT_TYPE port, EXO* exo, unsigned int num)
{
    return (num & USB_EP_IN) ? (exo->otg.otg[port]->in[USB_EP_NUM(num)]) : (exo->otg.otg[port]->out[USB_EP_NUM(num)]);
}

static inline DQH* ep_dqh(USB_PORT_TYPE port, unsigned int num)
{
    return &(((DQH*)__USB_REGS[port]->ENDPOINTLISTADDR)[USB_EP_NUM(num) * 2 + ((num & USB_EP_IN) ? 1 : 0)]);
}

static inline DTD* ep_dtd(USB_PORT_TYPE port, unsigned int num, unsigned int i)
{
    return &(((DTD*)(__USB_REGS[port]->ENDPOINTLISTADDR + sizeof(DQH) * USB_EP_COUNT_MAX * 2))[(USB_EP_NUM(num) * 2 + ((num & USB_EP_IN) ? 1 : 0)) * USB_DTD_COUNT + i]);
}

static void lpc_otg_bus_reset(USB_PORT_TYPE port, EXO* exo)
{
    int i;
    //clear all SETUP token semaphores
    __USB_REGS[port]->ENDPTSETUPSTAT = __USB_REGS[port]->ENDPTSETUPSTAT;
    //Clear all the endpoint complete status bits
    __USB_REGS[port]->ENDPTCOMPLETE = __USB_REGS[port]->ENDPTCOMPLETE;
    //Cancel all primed status
    while (__USB_REGS[port]->ENDPTPRIME) {}
    //And flush all endpoints
    __USB_REGS[port]->ENDPTFLUSH = 0xffffffff;
    while (__USB_REGS[port]->ENDPTFLUSH) {}
    //Disable all EP (except EP0)
    for (i = 1; i < USB_EP_COUNT_MAX; ++i)
        EP_CTRL(port)[i] = 0x0;

    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        ep_dqh(port, i)->capa = 0x0;
        ep_dqh(port, i)->next = (void*)USB0_DQH_NEXT_T_Msk;

        ep_dqh(port, USB_EP_IN | i)->capa = 0x0;
        ep_dqh(port, USB_EP_IN | i)->next = (void*)USB0_DQH_NEXT_T_Msk;
    }
    __USB_REGS[port]->DEVICEADDR = 0;
}

static inline void lpc_otg_reset(USB_PORT_TYPE port, EXO* exo)
{
    if (exo->otg.otg[port]->device_ready)
    {
        ipc_ipost_inline(exo->otg.otg[port]->device, HAL_CMD(HAL_USB, USB_RESET), USB_HANDLE(port, USB_HANDLE_DEVICE), exo->otg.otg[port]->speed, 0);
        exo->otg.otg[port]->device_ready = false;
    }
    else
    {
        exo->otg.otg[port]->device_pending = true;
        exo->otg.otg[port]->pending_ipc = HAL_CMD(HAL_USB, USB_RESET);
    }
}

static inline void lpc_otg_suspend(USB_PORT_TYPE port, EXO* exo)
{
    if (exo->otg.otg[port]->device_ready)
    {
        ipc_ipost_inline(exo->otg.otg[port]->device, HAL_CMD(HAL_USB, USB_SUSPEND), USB_HANDLE(port, USB_HANDLE_DEVICE), 0, 0);
        exo->otg.otg[port]->device_ready = false;
    }
    else
    {
        exo->otg.otg[port]->device_pending = true;
        exo->otg.otg[port]->pending_ipc = HAL_CMD(HAL_USB, USB_SUSPEND);
    }
}

static inline void lpc_otg_wakeup(USB_PORT_TYPE port, EXO* exo)
{
    if (exo->otg.otg[port]->device_ready)
    {
        ipc_ipost_inline(exo->otg.otg[port]->device, HAL_CMD(HAL_USB, USB_WAKEUP), USB_HANDLE(port, USB_HANDLE_DEVICE), 0, 0);
        exo->otg.otg[port]->device_ready = false;
    }
    else
    {
        exo->otg.otg[port]->device_pending = true;
        exo->otg.otg[port]->pending_ipc = HAL_CMD(HAL_USB, USB_WAKEUP);
    }
}

static inline void lpc_otg_setup(USB_PORT_TYPE port, EXO* exo)
{
    IPC ipc;
    ipc.process = exo->otg.otg[port]->device;
    ipc.cmd = HAL_CMD(HAL_USB, USB_SETUP);
    ipc.param1 = USB_HANDLE(port, 0);
    ipc.param2 = ep_dqh(port, 0)->stp[0];
    ipc.param3 = ep_dqh(port, 0)->stp[1];
    ipc_ipost(&ipc);
}

static inline void lpc_otg_out(USB_PORT_TYPE port, EXO* exo, int ep_num)
{
    EP* ep = exo->otg.otg[port]->out[ep_num];
    if (ep->io)
    {
        ep->io->data_size = exo->otg.otg[port]->read_size[ep_num] - ((ep_dtd(port, ep_num, 0)->size_flags & USB0_DTD_SIZE_FLAGS_SIZE_Msk) >> USB0_DTD_SIZE_FLAGS_SIZE_Pos);
        iio_complete(exo->otg.otg[port]->device, HAL_IO_CMD(HAL_USB, IPC_READ), USB_HANDLE(port, ep_num), ep->io);
        ep->io = NULL;
    }
}

static void lpc_otg_in(USB_PORT_TYPE port, EXO* exo, int ep_num)
{
    EP* ep = exo->otg.otg[port]->in[ep_num];
    if (ep->io)
    {
        iio_complete(exo->otg.otg[port]->device, HAL_IO_CMD(HAL_USB, IPC_WRITE), USB_HANDLE(port, USB_EP_IN | ep_num), ep->io);
        ep->io = NULL;
    }
}

#if (USB_DEBUG_ERRORS)
static void lpc_otg_err(USB_PORT_TYPE port, EXO* exo, int num)
{
    DTD* dtd = ep_dtd(port, num, 0);
    if (dtd->size_flags & USB0_DTD_SIZE_FLAGS_HALTED_Msk)
    {
        if (dtd->size_flags & USB0_DTD_SIZE_FLAGS_BUFFER_ERR_Msk)
            printk("OTG: Buffer error EP%#02X\n", num);
        if (dtd->size_flags & USB0_DTD_SIZE_FLAGS_TRANSACTION_ERR_Msk)
            printk("OTG: Transaction error EP%#02X\n", num);
    }
}
#endif //USB_DEBUG_ERRORS

static void lpc_otg_on_isr(int vector, void* param)
{
    int i;
    EXO* exo = param;
    USB_PORT_TYPE port = USB_0;
    if (vector != __USB_VECTORS[0])
        port = USB_1;

    if (__USB_REGS[port]->USBSTS_D & USB0_USBSTS_D_URI_Msk)
    {
        lpc_otg_bus_reset(port, exo);

        exo->otg.otg[port]->suspended = false;
        __USB_REGS[port]->USBSTS_D = USB0_USBSTS_D_URI_Msk;
    }
    if (__USB_REGS[port]->USBSTS_D & USB0_USBSTS_D_SLI_Msk)
    {
        lpc_otg_suspend(port, exo);
        exo->otg.otg[port]->suspended = true;
        __USB_REGS[port]->USBSTS_D = USB0_USBSTS_D_SLI_Msk;
    }
    if (__USB_REGS[port]->USBSTS_D & USB0_USBSTS_D_PCI_Msk)
    {
        exo->otg.otg[port]->speed = __USB_REGS[port]->PORTSC1_D & USB0_PORTSC1_D_HSP_Msk ? USB_HIGH_SPEED : USB_FULL_SPEED;
        __USB_REGS[port]->USBSTS_D = USB0_USBSTS_D_PCI_Msk;
        if (exo->otg.otg[port]->suspended)
        {
            exo->otg.otg[port]->suspended = false;
            lpc_otg_wakeup(port, exo);
        }
        else
            lpc_otg_reset(port, exo);
    }

    if (__USB_REGS[port]->USBSTS_D & USB0_USBSTS_D_UI_Msk)
    {
#if (USB_DEBUG_ERRORS)
        if (__USB_REGS[port]->USBSTS_D & USB0_USBSTS_D_UEI_Msk)
        {
            for (i = 0; i < USB_EP_COUNT_MAX; ++i )
            {
                lpc_otg_err(port, exo, i);
                lpc_otg_err(port, exo, USB_EP_IN | i);
            }
            __USB_REGS[port]->USBSTS_D = USB0_USBSTS_D_UEI_Msk;
        }
#endif //USB_DEBUG_ERRORS
        for (i = 0; __USB_REGS[port]->ENDPTCOMPLETE && (i < USB_EP_COUNT_MAX); ++i )
        {
            if (__USB_REGS[port]->ENDPTCOMPLETE & EP_BIT(i))
            {
                lpc_otg_out(port, exo, i);
                __USB_REGS[port]->ENDPTCOMPLETE = EP_BIT(i);
            }
            if (__USB_REGS[port]->ENDPTCOMPLETE & EP_BIT(USB_EP_IN | i))
            {
                lpc_otg_in(port, exo, i);
                __USB_REGS[port]->ENDPTCOMPLETE = EP_BIT(USB_EP_IN | i);
            }
        }
        //Only for EP0
        if (__USB_REGS[port]->ENDPTSETUPSTAT & (1 << 0))
        {
            lpc_otg_setup(port, exo);
            __USB_REGS[port]->ENDPTSETUPSTAT = (1 << 0);
        }
        __USB_REGS[port]->USBSTS_D = USB0_USBSTS_D_UI_Msk;
    }

#if (USB_DEBUG_ERRORS)
    if (__USB_REGS[port]->USBSTS_D & USB0_USBSTS_D_SEI_Msk)
    {
        printk("OTG: system error\n");
        __USB_REGS[port]->USBSTS_D = USB0_USBSTS_D_SEI_Msk;
    }
#endif
}

void lpc_otg_init(EXO* exo)
{
    int i;
    for (i = 0; i < USB_COUNT; ++i)
        exo->otg.otg[i] = NULL;
}

static bool lpc_otg_ep_flush(USB_PORT_TYPE port, EXO* exo, int num)
{
    EP* ep = ep_data(port, exo, num);
    if (ep == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return false;
    }

    //flush
    __USB_REGS[port]->ENDPTFLUSH = EP_BIT(num);
    while (__USB_REGS[port]->ENDPTFLUSH & EP_BIT(num)) {}
    //reset sequence
    EP_CTRL(port)[USB_EP_NUM(num)] |= USB0_ENDPTCTRL_R_Msk << ((num & USB_EP_IN) ? 16 : 0);
    if (ep->io != NULL)
    {
        iio_complete_ex(exo->otg.otg[port]->device, HAL_IO_CMD(HAL_USB, (num & USB_EP_IN) ? IPC_WRITE : IPC_READ), USB_HANDLE(port, num), ep->io, ERROR_IO_CANCELLED);
        ep->io = NULL;
    }
    return true;
}

static inline void lpc_otg_ep_set_stall(USB_PORT_TYPE port, EXO* exo, int num)
{
    if (!lpc_otg_ep_flush(port, exo, num))
        return;
    EP_CTRL(port)[USB_EP_NUM(num)] |= USB0_ENDPTCTRL_S_Msk << ((num & USB_EP_IN) ? 16 : 0);
}

static inline void lpc_otg_ep_clear_stall(USB_PORT_TYPE port, EXO* exo, int num)
{
    if (!lpc_otg_ep_flush(port, exo, num))
        return;
    EP_CTRL(port)[USB_EP_NUM(num)] &= ~(USB0_ENDPTCTRL_S_Msk << ((num & USB_EP_IN) ? 16 : 0));
    EP_CTRL(port)[USB_EP_NUM(num)] |= USB0_ENDPTCTRL_R_Msk << ((num & USB_EP_IN) ? 16 : 0);
}


static inline bool lpc_otg_ep_is_stall(USB_PORT_TYPE port, int num)
{
    return (EP_CTRL(port)[USB_EP_NUM(num)] & (USB0_ENDPTCTRL_S_Msk << ((num & USB_EP_IN) ? 16 : 0))) ? true : false;
}

static inline void lpc_otg_open_ep(USB_PORT_TYPE port, EXO* exo, int num, USB_EP_TYPE type, unsigned int size)
{
    unsigned int reg;
    DQH* dqh;
    if (ep_data(port, exo, num) != NULL)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }

    EP* ep = kmalloc(sizeof(EP));
    if (ep == NULL)
        return;
    ep->io = NULL;
    num & USB_EP_IN ? (exo->otg.otg[port]->in[USB_EP_NUM(num)] = ep) : (exo->otg.otg[port]->out[USB_EP_NUM(num)] = ep);

    dqh = ep_dqh(port, num);
    dqh->capa = (size << USB0_DQH_CAPA_MAX_PACKET_LENGTH_Pos) | USB0_DQH_CAPA_ZLT_Msk;
    if (USB_EP_NUM(num) == 0)
        dqh->capa |= USB0_DQH_CAPA_IOS_Msk;
    dqh->next = (void*)USB0_DQH_NEXT_T_Msk;

    if (USB_EP_NUM(num))
        reg = (type << USB0_ENDPTCTRL_T_Pos) | USB0_ENDPTCTRL_E_Msk | USB0_ENDPTCTRL_R_Msk;
    else
        reg = USB0_ENDPTCTRL_E_Msk;

    if (num & USB_EP_IN)
        EP_CTRL(port)[USB_EP_NUM(num)] = (EP_CTRL(port)[USB_EP_NUM(num)] & 0xffff) | (reg << 16);
    else
        EP_CTRL(port)[USB_EP_NUM(num)] = (EP_CTRL(port)[USB_EP_NUM(num)] & (0xffff << 16)) | reg;
}

static void lpc_otg_close_ep(USB_PORT_TYPE port, EXO* exo, int num)
{
    if (!lpc_otg_ep_flush(port, exo, num))
        return;

    EP* ep = ep_data(port, exo, num);
    kfree(ep);
    num & USB_EP_IN ? (exo->otg.otg[port]->in[USB_EP_NUM(num)] = NULL) : (exo->otg.otg[port]->out[USB_EP_NUM(num)] = NULL);
}

static void lpc_otg_io(USB_PORT_TYPE port, EXO* exo, IPC* ipc, bool read)
{
    unsigned int i, size;
    unsigned int num = USB_NUM(ipc->param1);
    DTD* dtd;
    DQH* dqh;
    EP* ep = ep_data(port, exo, num);
    if (ep == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    if (ep->io != NULL)
    {
        kerror(ERROR_IN_PROGRESS);
        return;
    }
    if (read)
        size = exo->otg.otg[port]->read_size[USB_EP_NUM(num)] = ipc->param3;
    else
        size = ((IO*)ipc->param2)->data_size;
    if (size > USB_DTD_CHUNK * USB_DTD_COUNT)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    ep->io = (IO*)ipc->param2;

    i = 0;
    do {
        //prepare DTD
        dtd = ep_dtd(port, num, i);

        if (size > USB_DTD_CHUNK)
        {
            dtd->size_flags = (USB_DTD_CHUNK << USB0_DTD_SIZE_FLAGS_SIZE_Pos) | USB0_DTD_SIZE_FLAGS_ACTIVE_Msk;
            dtd->next = (unsigned int)ep_dtd(port, num, i + 1);
            size -= USB_DTD_CHUNK;
        }
        else
        {
            dtd->next = USB0_DQH_NEXT_T_Msk;
            dtd->size_flags = (size << USB0_DTD_SIZE_FLAGS_SIZE_Pos) | USB0_DTD_SIZE_FLAGS_IOC_Msk | USB0_DTD_SIZE_FLAGS_ACTIVE_Msk;
            size = 0;
        }

        dtd->buf[0] = (uint8_t*)io_data(ep->io) + i * USB_DTD_CHUNK;
        dtd->buf[1] = (void*)(((unsigned int)(dtd->buf[0]) + 0x1000) & 0xfffff000);
        dtd->buf[2] = dtd->buf[1] + 0x1000;
        dtd->buf[3] = dtd->buf[1] + 0x2000;
        dtd->buf[4] = dtd->buf[1] + 0x3000;

        ++i;
    } while (size);

    //insert in EQH
    dqh = ep_dqh(port, num);
    dqh->next = ep_dtd(port, num, 0);
    dqh->shadow_size_flags &= ~USB0_DTD_SIZE_FLAGS_STATUS_Msk;
    if (USB_EP_NUM(num) == 0)
        //required before EP0 transfers
        while (__USB_REGS[port]->ENDPTSETUPSTAT & (1 << 0)) {}
    __USB_REGS[port]->ENDPTPRIME = EP_BIT(num);
    kerror(ERROR_SYNC);
}

static inline void lpc_otg_open_device(USB_PORT_TYPE port, EXO* exo, HANDLE device)
{
    if (exo->otg.otg[port])
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }
    exo->otg.otg[port] = kmalloc(sizeof(OTG_TYPE));
    if (exo->otg.otg[port] == NULL)
        return;
    int i;
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        exo->otg.otg[port]->out[i] = NULL;
        exo->otg.otg[port]->in[i] = NULL;
    }
    exo->otg.otg[port]->device = device;
    exo->otg.otg[port]->suspended = false;
    exo->otg.otg[port]->speed = USB_FULL_SPEED;

    exo->otg.otg[port]->device_pending = false;
    exo->otg.otg[port]->device_ready = true;

#if defined(LPC183x) || defined(LPC185x)
    if (port == USB_1)
    {
#if (USB1_ULPI)
        //for external PHY USB1 clock must be disabled
        //userspace is responsable for ULPI pin configuration
        LPC_CCU1->CLK_USB1_CFG &= ~CCU1_CLK_USB1_CFG_RUN_Msk;
#else
        //configure IDIVA divider
        LPC_CGU->IDIVA_CTRL |= ((IDIV_A - 1) << CGU_IDIVA_CTRL_IDIV_Pos) | CGU_CLK_PLL1;
        LPC_CGU->IDIVA_CTRL &= ~CGU_IDIVA_CTRL_PD_Msk;
        //connect USB1 to IDIVA
        LPC_CGU->BASE_USB1_CLK = CGU_BASE_USB1_CLK_PD_Msk;
        LPC_CGU->BASE_USB1_CLK |= CGU_CLK_IDIVA;
        LPC_CGU->BASE_USB1_CLK &= ~CGU_BASE_USB1_CLK_PD_Msk;

        LPC_SCU->SFSUSB |= SCU_SFSUSB_EPWR_Msk;

        //enable VBUS monitoring
        lpc_pin_enable(P2_5, P2_5_USB1_VBUS | SCU_SFS_EPUN | SCU_SFS_EZI | SCU_SFS_ZIF);
#endif //USB1_ULPI
    }
    else
#endif //defined(LPC183x) || defined(LPC185x)
    {
        //Clock registers
        LPC_CGU->BASE_USB0_CLK |= CGU_CLK_PLL0USB;
        LPC_CGU->BASE_USB0_CLK &= ~CGU_BASE_USB0_CLK_PD_Msk;

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
            kerror(ERROR_HARDWARE);
            return;
        }
        //enable PLL clock
        LPC_CGU->PLL0USB_CTRL |= CGU_PLL0USB_CTRL_CLKEN_Msk;

        //turn on USB0 PHY
        LPC_CREG->CREG0 &= ~CREG_CREG0_USB0PHY_Msk;
    }

    //USB must be reset before mode change
    __USB_REGS[port]->USBCMD_D = USB0_USBCMD_D_RST_Msk;
    //lowest interrupt latency
    __USB_REGS[port]->USBCMD_D &= ~USB0_USBCMD_D_ITC_Msk;
    while (__USB_REGS[port]->USBCMD_D & USB0_USBCMD_D_RST_Msk) {}
    //set device mode
    __USB_REGS[port]->USBMODE_D = USB0_USBMODE_CM_DEVICE;

#if (LPC_USB_USE_BOTH)
    if (port == USB_1)
        __USB_REGS[port]->ENDPOINTLISTADDR = AHB_SRAM2_BASE + AHB_SRAM2_SIZE - (USB_EP_COUNT_MAX | 1) * 2048;
    else
#endif //LPC_USB_USE_BOTH
        __USB_REGS[port]->ENDPOINTLISTADDR = AHB_SRAM2_BASE + AHB_SRAM2_SIZE - (USB_EP_COUNT_MAX >> 1) * 2048;

    memset((void*)__USB_REGS[port]->ENDPOINTLISTADDR, 0, (sizeof(DQH) + sizeof(DTD)) * USB_EP_COUNT_MAX * 2);

    //clear any spurious pending interrupts
    __USB_REGS[port]->USBSTS_D = USB0_USBSTS_D_UI_Msk | USB0_USBSTS_D_UEI_Msk | USB0_USBSTS_D_PCI_Msk | USB0_USBSTS_D_SEI_Msk | USB0_USBSTS_D_URI_Msk |
                         USB0_USBSTS_D_SLI_Msk;

    //enable interrupts
    kirq_register(KERNEL_HANDLE, __USB_VECTORS[port], lpc_otg_on_isr, exo);
    NVIC_EnableIRQ(__USB_VECTORS[port]);
    NVIC_SetPriority(__USB_VECTORS[port], 1);

    //Unmask common interrupts
    __USB_REGS[port]->USBINTR_D = USB0_USBINTR_D_UE_Msk | USB0_USBINTR_D_PCE_Msk | USB0_USBINTR_D_URE_Msk | USB0_USBINTR_D_SLE_Msk;
#if (USB_DEBUG_ERRORS)
    __USB_REGS[port]->USBINTR_D |= USB0_USBINTR_D_UEE_Msk | USB0_USBINTR_D_SEE_Msk;
#endif

    //start
    __USB_REGS[port]->USBCMD_D |= USB0_USBCMD_D_RS_Msk;
}

static inline void lpc_otg_close_device(USB_PORT_TYPE port, EXO* exo)
{
    int i;
    //disable interrupts
    NVIC_DisableIRQ(__USB_VECTORS[port]);
    kirq_unregister(KERNEL_HANDLE, __USB_VECTORS[port]);
    //stop
    __USB_REGS[port]->USBCMD_D &= ~USB0_USBCMD_D_RS_Msk;

    //close all endpoints
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        if (exo->otg.otg[port]->out[i] != NULL)
            lpc_otg_close_ep(port, exo, i);
        if (exo->otg.otg[port]->in[i] != NULL)
            lpc_otg_close_ep(port, exo, USB_EP_IN | i);
    }
    //Mask all interrupts
    __USB_REGS[port]->USBINTR_D = 0;

#if defined(LPC183x) || defined(LPC185x)
#if !(USB1_ULPI)
    if (port == USB_1)
    {
        lpc_pin_disable(P2_5);
        LPC_SCU->SFSUSB &= ~SCU_SFSUSB_EPWR_Msk;

        LPC_CGU->BASE_USB1_CLK = CGU_BASE_USB1_CLK_PD_Msk;
        LPC_CGU->IDIVA_CTRL = CGU_IDIVA_CTRL_PD_Msk;
    }
    else
#endif //USB1_ULPI
#endif //defined(LPC183x) || defined(LPC185x)
    {
        //turn off USB0 PHY
        LPC_CREG->CREG0 |= CREG_CREG0_USB0PHY_Msk;
        //disable USB0PLL
        LPC_CGU->PLL0USB_CTRL = CGU_PLL0USB_CTRL_PD_Msk;

        LPC_CGU->BASE_USB0_CLK = CGU_BASE_USB0_CLK_PD_Msk;
    }

    //free object
    kfree(exo->otg.otg[port]);
    exo->otg.otg[port] = NULL;
}

static inline void lpc_otg_set_address(USB_PORT_TYPE port, EXO* exo, int addr)
{
    //according to datasheet, no special action is required if status will go in 2 ms
    __USB_REGS[port]->DEVICEADDR = (addr << USB0_DEVICEADDR_USBADR_Pos) | USB0_DEVICEADDR_USBADRA_Msk;
}

#if (USB_TEST_MODE_SUPPORT)
static inline void lpc_otg_set_test_mode(USB_PORT_TYPE port, EXO* exo, USB_TEST_MODES test_mode)
{
    __USB_REGS[port]->PORTSC1_D &= ~USB1_PORTSC1_H_PTC3_0_Msk;
    __USB_REGS[port]->PORTSC1_D |= test_mode << USB1_PORTSC1_H_PTC3_0_Pos;
}
#endif //USB_TEST_MODE_SUPPORT

static inline void lpc_otg_sync(EXO* exo)
{
    int port;
    for (port = 0; port < USB_COUNT; port++)
    {
        if (exo->otg.otg[port] == NULL)
            continue;
        if (exo->otg.otg[port]->device == INVALID_HANDLE)
            continue;
        if (exo->otg.otg[port]->device_pending)
        {
            switch (HAL_ITEM(exo->otg.otg[port]->pending_ipc))
            {
            case USB_RESET:
                ipc_ipost_inline(exo->otg.otg[port]->device, exo->otg.otg[port]->pending_ipc,
                                 USB_HANDLE(USB_0, USB_HANDLE_DEVICE), exo->otg.otg[port]->speed, 0);
                break;
            case USB_SUSPEND:
            case USB_WAKEUP:
                ipc_ipost_inline(exo->otg.otg[port]->device, exo->otg.otg[port]->pending_ipc, USB_HANDLE(USB_0, USB_HANDLE_DEVICE), 0, 0);
                break;
            default:
                break;
            }
            exo->otg.otg[port]->device_pending = false;
        }
        else
            exo->otg.otg[port]->device_ready = true;
    }
}

static inline void lpc_otg_device_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case USB_GET_SPEED:
        ipc->param2 = exo->otg.otg[USB_PORT(ipc->param1)]->speed;
        break;
    case IPC_OPEN:
        lpc_otg_open_device(USB_PORT(ipc->param1), exo, ipc->process);
        break;
    case IPC_CLOSE:
        lpc_otg_close_device(USB_PORT(ipc->param1), exo);
        break;
    case USB_SET_ADDRESS:
        lpc_otg_set_address(USB_PORT(ipc->param1), exo, ipc->param2);
        break;
#if (USB_TEST_MODE_SUPPORT)
    case USB_SET_TEST_MODE:
        lpc_otg_set_test_mode(USB_PORT(ipc->param1), exo, ipc->param2);
        break;
#endif //USB_TEST_MODE_SUPPORT
    case IPC_SYNC:
        lpc_otg_sync(exo);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

static inline void lpc_otg_ep_request(EXO* exo, IPC* ipc)
{
    if (USB_EP_NUM(USB_NUM(ipc->param1)) >= USB_EP_COUNT_MAX)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        lpc_otg_open_ep(USB_PORT(ipc->param1), exo, USB_NUM(ipc->param1), ipc->param2, ipc->param3);
        break;
    case IPC_CLOSE:
        lpc_otg_close_ep(USB_PORT(ipc->param1), exo, USB_NUM(ipc->param1));
        break;
    case IPC_FLUSH:
        lpc_otg_ep_flush(USB_PORT(ipc->param1), exo, USB_NUM(ipc->param1));
        break;
    case USB_EP_SET_STALL:
        lpc_otg_ep_set_stall(USB_PORT(ipc->param1), exo, USB_NUM(ipc->param1));
        break;
    case USB_EP_CLEAR_STALL:
        lpc_otg_ep_clear_stall(USB_PORT(ipc->param1), exo, USB_NUM(ipc->param1));
        break;
    case USB_EP_IS_STALL:
        ipc->param2 = lpc_otg_ep_is_stall(USB_PORT(ipc->param1), USB_NUM(ipc->param1));
        break;
    case IPC_READ:
        lpc_otg_io(USB_PORT(ipc->param1), exo, ipc, true);
        break;
    case IPC_WRITE:
        lpc_otg_io(USB_PORT(ipc->param1), exo, ipc, false);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

void lpc_otg_request(EXO* exo, IPC* ipc)
{
    if (USB_PORT(ipc->param1) >= USB_COUNT)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    if (USB_NUM(ipc->param1) == USB_HANDLE_DEVICE)
        lpc_otg_device_request(exo, ipc);
    else
        lpc_otg_ep_request(exo, ipc);
}
