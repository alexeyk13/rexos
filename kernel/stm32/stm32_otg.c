/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#include "stm32_otg.h"
#include "stm32_regsusb.h"
#include "../../userspace/stm32/stm32_driver.h"
#include "stm32_power.h"
#include "../../userspace/sys.h"
#include "../../userspace/usb.h"
#include "../kerror.h"
#include "../kirq.h"
#include "../kexo.h"
#include "../../userspace/object.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/power.h"
#include "../kstdlib.h"
#include <string.h>
#include "stm32_exo_private.h"

#define CORE_DEBUG               0
#define PIN_DEBUG                0


#if (PIN_DEBUG)
#include "config.h"
#include "gpio.h"
#endif

#if defined(STM32H7)
#if (STM32_USB_OTG_FS) && (STM32_USB_OTG_HS)
#define STM32_USB_DUAL            1
#else
#define STM32_USB_DUAL            0
#endif

typedef OTG_FS_DEVICE_TypeDef* OTG_FS_DEVICE_P;
typedef OTG_FS_GENERAL_TypeDef* OTG_FS_GENERAL_P;
typedef OTG_FS_PC_TypeDef* OTG_FS_PC_P;

static const OTG_FS_DEVICE_P __USB_DEVICE[] =   {OTG_FS_DEVICE, OTG_HS_DEVICE};
static const OTG_FS_GENERAL_P __USB_GENERAL[] =   {OTG_FS_GENERAL, OTG_HS_GENERAL};
static const OTG_FS_PC_P __USB_PC[] =   {OTG_FS_PC, OTG_HS_PC};
static const uint32_t __USB_FIFO_BASE[] =   {OTG_FS_FIFO_BASE, OTG_HS_FIFO_BASE};

static const uint32_t __USB_WKUP_IRQ[] =   {OTG_FS_WKUP_IRQn, OTG_HS_WKUP_IRQn};
static const uint32_t __USB_IRQ[] =        {OTG_FS_IRQn, OTG_HS_IRQn};


#if(STM32_USB_DUAL == 0)

#if (STM32_USB_OTG_FS)
#define  _OTG_DEVICE     __USB_DEVICE[0]
#define  _OTG_GENERAL    __USB_GENERAL[0]
#define  _OTG_FIFO_BASE  __USB_FIFO_BASE[0]
#define  _OTG_PC         __USB_PC[0]

#define  _OTG_WKUP_IRQ   __USB_WKUP_IRQ[0]
#define  _OTG_IRQ        __USB_IRQ[0]

#elif (STM32_USB_OTG_HS)
#define  _OTG_DEVICE  __USB_DEVICE[1]
#define  _OTG_GENERAL __USB_GENERAL[1]
#define  _OTG_FIFO_BASE __USB_FIFO_BASE[1]
#define  _OTG_PC         __USB_PC[1]

#define  _OTG_WKUP_IRQ   __USB_WKUP_IRQ[1]
#define  _OTG_IRQ        __USB_IRQ[1]

#else
#error define usb instance
#endif
#endif //STM32_USB_DUAL


#else
#define _OTG_DEVICE             OTG_FS_DEVICE
#define _OTG_IRQ                OTG_FS_IRQn
#endif //STM32H7

#define ALIGN_SIZE                                          (sizeof(int))
#define ALIGN(var)                                          (((var) + (ALIGN_SIZE - 1)) & ~(ALIGN_SIZE - 1))


#define GET_CORE_CLOCK          stm32_power_get_clock_inside(exo, POWER_CORE_CLOCK)

static inline OTG_FS_DEVICE_ENDPOINT_TypeDef* ep_reg_data(USB_PORT_TYPE port, int num)
{
    return (num & USB_EP_IN) ? &(_OTG_DEVICE->INEP[USB_EP_NUM(num)]) : &(_OTG_DEVICE->OUTEP[USB_EP_NUM(num)]);
}

static inline EP* ep_data(USB_PORT_TYPE port, EXO* exo, int num)
{
    return (num & USB_EP_IN) ? (exo->usb.in[USB_EP_NUM(num)]) : (exo->usb.out[USB_EP_NUM(num)]);
}

static inline void usb_reset_all()
{
#if defined(STM32H7)
       _OTG_GENERAL->RSTCTL |= OTG_FS_GENERAL_RSTCTL_CSRST;
       while (_OTG_GENERAL->RSTCTL & OTG_FS_GENERAL_RSTCTL_CSRST) {};
       _OTG_GENERAL->AHBCFG = OTG_FS_GENERAL_AHBCFG_GINT;

       _OTG_GENERAL->RXFSIZ = ((STM32_USB_MPS / 4) + 1) * 2 + 10 + 1;
       _OTG_GENERAL->TX0FSIZ = (0x20  << OTG_FS_GENERAL_TX0FSIZ_TX0FD_POS) | (_OTG_GENERAL->RXFSIZ << OTG_FS_GENERAL_TX0FSIZ_TX0FSA_POS);

       //repeat setup after reset
   #if (STM32_USB_OTG_ULPI)
       _OTG_GENERAL->USBCFG =  (1 << 20) |  (0 << 15) | OTG_FS_GENERAL_USBCFG_FDMOD | (9 << OTG_FS_GENERAL_USBCFG_TRDT_POS) | (17 << OTG_FS_GENERAL_USBCFG_TOCAL_POS);
   #else
       _OTG_GENERAL->USBCFG = (1 << 15) | OTG_FS_GENERAL_USBCFG_FDMOD | (trdt << OTG_FS_GENERAL_USBCFG_TRDT_POS) | OTG_FS_GENERAL_USBCFG_PHYSEL | (17 << OTG_FS_GENERAL_USBCFG_TOCAL_POS);
   #endif // STM32_USB_OTG_ULPI

       _OTG_GENERAL->INTMSK = OTG_FS_GENERAL_INTMSK_USBSUSPM | OTG_FS_GENERAL_INTMSK_ENUMDNEM |
               OTG_FS_GENERAL_INTMSK_IEPM | OTG_FS_GENERAL_INTMSK_RXFLVLM | OTG_FS_GENERAL_INTMSK_USBRSTM ;

       _OTG_GENERAL->CCFG |= OTG_FS_GENERAL_CCFG_VBUSBSEN;
       _OTG_DEVICE->IEPMSK = OTG_FS_DEVICE_IEPMSK_XFRCM;
       _OTG_DEVICE->CTL = OTG_FS_DEVICE_CTL_POPRGDNE;

       _OTG_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_OTGM;
#endif // STM32H7
}

static bool stm32_otg_ep_flush(USB_PORT_TYPE port, EXO* exo, int num)
{
    EP* ep = ep_data(port, exo, num);
    if (ep == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return false;
    }
    if (ep->io != NULL)
    {
        kexo_io_ex(exo->usb.device, HAL_IO_CMD(HAL_USB, (num & USB_EP_IN) ? IPC_WRITE : IPC_READ), num, ep->io, ERROR_IO_CANCELLED);
        ep->io = NULL;
        ep_reg_data(port, num)->CTL |= OTG_FS_DEVICE_ENDPOINT_CTL_SNAK | OTG_FS_DEVICE_ENDPOINT_CTL_EPDIS;
    }
    ep->io_active = false;
    if(num & USB_EP_IN)
    {
         _OTG_GENERAL->RSTCTL = ((1 << 5) | (USB_EP_NUM(num) << 6));
        while ((_OTG_GENERAL->RSTCTL & OTG_HS_GENERAL_RSTCTL_TXFFLSH) == OTG_HS_GENERAL_RSTCTL_TXFFLSH);

    }
    return true;
}

static inline void stm32_otg_ep_set_stall(USB_PORT_TYPE port, EXO* exo, int num)
{
    if (!stm32_otg_ep_flush(port, exo, num))
        return;
    ep_reg_data(port, num)->CTL |= OTG_FS_DEVICE_ENDPOINT_CTL_STALL;
}

static inline void stm32_otg_ep_clear_stall(USB_PORT_TYPE port, EXO* exo, int num)
{
    if (!stm32_otg_ep_flush(port, exo, num))
        return;
    ep_reg_data(port, num)->CTL &= ~OTG_FS_DEVICE_ENDPOINT_CTL_STALL;
}

static inline bool stm32_otg_ep_is_stall(USB_PORT_TYPE port, int num)
{
    return ep_reg_data(port, num)->CTL & OTG_FS_DEVICE_ENDPOINT_CTL_STALL ? true : false;
}

static inline void usb_suspend(EXO* exo)
{
    usb_reset_all();
    IPC ipc;
    _OTG_GENERAL->INTMSK &= ~OTG_FS_GENERAL_INTMSK_USBSUSPM;
    ipc.process = exo->usb.device;
    ipc.param1 = USB_HANDLE_DEVICE;
    ipc.cmd = HAL_CMD(HAL_USB, USB_SUSPEND);
    ipc_ipost(&ipc);
}

static inline void usb_wakeup(EXO* exo)
{
    IPC ipc;
    _OTG_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_USBSUSPM;
    ipc.process = exo->usb.device;
    ipc.param1 = USB_HANDLE_DEVICE;
    ipc.cmd = HAL_CMD(HAL_USB, USB_WAKEUP);
    ipc_ipost(&ipc);
}

static void stm32_otg_rx_prepare(EXO* exo, unsigned int ep_num)
{
    EP* ep = exo->usb.out[ep_num];
    int size = ep->size - ep->io->data_size;

    uint32_t pkt_cnt;
    pkt_cnt = ((size + ep->mps - 1) / ep->mps);
    if(pkt_cnt == 0 )
        pkt_cnt = 1;
    size = pkt_cnt *  ep->mps;
    if (ep_num)
        _OTG_DEVICE->OUTEP[ep_num].TSIZ = (pkt_cnt << OTG_FS_DEVICE_ENDPOINT_TSIZ_PKTCNT_POS) | (size << OTG_FS_DEVICE_ENDPOINT_TSIZ_XFRSIZ_POS);
    else //EP0
        _OTG_DEVICE->OUTEP[0].TSIZ = (pkt_cnt << OTG_FS_DEVICE_ENDPOINT_TSIZ_PKTCNT_POS) | (size << OTG_FS_DEVICE_ENDPOINT_TSIZ_XFRSIZ_POS) |
                                       (3 << OTG_FS_DEVICE_ENDPOINT_TSIZ_STUPCNT_OUT0_POS);

    _OTG_DEVICE->OUTEP[ep_num].CTL |= OTG_FS_DEVICE_ENDPOINT_CTL_EPENA | OTG_FS_DEVICE_ENDPOINT_CTL_CNAK;
}

static void memcpy4(void* dst, void* src, int size)
{
    int* dst1 = dst;
    int* src1 = src;
    size = ALIGN(size) /4;
    __disable_irq();         // stm32h7 workaround!

    while(size--)
        *dst1++ = *src1++;
    __enable_irq();
}

static inline void stm32_otg_tx(EXO* exo, int num)
{
    uint32_t pkt_cnt;
    bool use_int = false;
    num = USB_EP_NUM(num);
    EP* ep = exo->usb.in[num];
    int size = ep->io->data_size - ep->size;

#if (CORE_DEBUG)
    printk("t:%d e:%d\n",size, num);
#endif //CORE_DEBUG

    pkt_cnt = ((size + ep->mps - 1) / ep->mps);
    if(pkt_cnt == 0 )
        pkt_cnt = 1;

    _OTG_DEVICE->INEP[num].TSIZ = (pkt_cnt << OTG_FS_DEVICE_ENDPOINT_TSIZ_PKTCNT_POS) | (size << OTG_FS_DEVICE_ENDPOINT_TSIZ_XFRSIZ_POS);

    if(size > _OTG_DEVICE->INEP[num].FSTS * 4)
    {
        size = _OTG_DEVICE->INEP[num].FSTS * 4;
        use_int = true;
    }

    _OTG_DEVICE->INEP[num].CTL |= OTG_FS_DEVICE_ENDPOINT_CTL_EPENA | OTG_FS_DEVICE_ENDPOINT_CTL_CNAK;

#if (PIN_DEBUG)
    gpio_set_pin(TEST1_PIN);
    memcpy4((void*)(_OTG_FIFO_BASE + USB_EP_NUM(num) * 0x1000), io_data(ep->io) +  ep->size, size);
    gpio_reset_pin(TEST1_PIN);
#else
    if(size)
        memcpy4((void*)(_OTG_FIFO_BASE + USB_EP_NUM(num) * 0x1000), io_data(ep->io) +  ep->size, size);
#endif // PIN_DEBUG
    ep->size += size;
    if(use_int)
        _OTG_DEVICE->EIPEMPMSK |= (1 << num);
}

USB_SPEED stm32_otg_get_speed(EXO* exo)
{
#if defined(STM32H7) || defined(STM32F4)
    if (_OTG_DEVICE->CFG & OTG_FS_DEVICE_CFG_DSPD_FS)
        return USB_FULL_SPEED;
    else
        return USB_HIGH_SPEED;
#else
    //according to datasheet STM32F1_CL doesn't support low speed mode...
    return USB_FULL_SPEED;
#endif
}

static inline void usb_enumdne(EXO* exo)
{
    _OTG_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_USBSUSPM;
    IPC ipc;
    ipc.process = exo->usb.device;
    ipc.param1 = USB_HANDLE_DEVICE;
    ipc.param2 = stm32_otg_get_speed(exo);
    ipc.cmd = HAL_CMD(HAL_USB, USB_RESET);
    ipc_ipost(&ipc);
}

//--------------------------ISR -------------------------------------------
static inline void stm32_otg_on_isr_rx(EXO* exo)
{
    IPC ipc;
    unsigned int sta = _OTG_GENERAL->RXSTSP;
    unsigned int pktsts = sta & OTG_FS_GENERAL_RXSTSR_PKTSTS;
    int bcnt = (sta & OTG_FS_GENERAL_RXSTSR_BCNT) >> OTG_FS_GENERAL_RXSTSR_BCNT_POS;
    unsigned int ep_num = (sta & OTG_FS_GENERAL_RXSTSR_EPNUM) >> OTG_FS_GENERAL_RXSTSR_EPNUM_POS;
    EP* ep = exo->usb.out[ep_num];
#if (CORE_DEBUG)
    printk("sts:%x\n", sta);
#endif //CORE_DEBUG
    if (pktsts == OTG_FS_GENERAL_RXSTSR_PKTSTS_SETUP_RX)
    {
//ignore all data on setup packet
        exo->usb.setup_lo = ((uint32_t*)(_OTG_FIFO_BASE + ep_num * 0x1000))[0];
        exo->usb.setup_hi = ((uint32_t*)(_OTG_FIFO_BASE + ep_num * 0x1000))[0];
#if (CORE_DEBUG)
        printk("s:%x %x\n", exo->usb.setup_lo, exo->usb.setup_hi);
#endif //CORE_DEBUG

    }
    else  if ((pktsts == OTG_FS_GENERAL_RXSTSR_PKTSTS_SETUP_DONE))
    {
#if (CORE_DEBUG)
    printk("sd\n");
#endif //CORE_DEBUG
        ipc.process = exo->usb.device;
        ipc.cmd = HAL_CMD(HAL_USB, USB_SETUP);
        ipc.param1 = USB_HANDLE(USB_0, 0);
        ipc.param2 = exo->usb.setup_lo;
        ipc.param3 = exo->usb.setup_hi;
        ipc_ipost(&ipc);
        _OTG_DEVICE->OUTEP[0].INT = OTG_FS_DEVICE_ENDPOINT_INT_STUP;
    }
    else  if ((pktsts == OTG_FS_GENERAL_RXSTSR_PKTSTS_OUT_RX) && bcnt)
    {
#if (CORE_DEBUG)
       printk("r:%d e:%d\n",bcnt, ep_num);
#endif //CORE_DEBUG
        memcpy4(io_data(ep->io) + ep->io->data_size, (void*)(_OTG_FIFO_BASE + ep_num * 0x1000), bcnt);
        ep->io->data_size += bcnt;
    }
    else  if (pktsts == OTG_FS_GENERAL_RXSTSR_PKTSTS_OUT_DONE)
    {
        iio_complete(exo->usb.device, HAL_IO_CMD(HAL_USB, IPC_READ), ep_num, ep->io);
        ep->io_active = false;
        ep->io = NULL;
        _OTG_DEVICE->OUTEP[ep_num].INT = OTG_FS_DEVICE_ENDPOINT_INT_XFRC | OTG_FS_DEVICE_ENDPOINT_INT_OTEPDIS | OTG_FS_DEVICE_ENDPOINT_INT_NAK ;
    }

    _OTG_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_RXFLVLM;
}

static inline void stm32_otg_on_isr_tx_fifo_empty(EXO* exo, int num)
{
    EP* ep = exo->usb.in[num];
    int size = ep->io->data_size - ep->size;
    if(size <= 0)
    {
        _OTG_DEVICE->EIPEMPMSK &= ~(1 << num);
        return;
    }

    if(size > _OTG_DEVICE->INEP[num].FSTS * 4)
        size = _OTG_DEVICE->INEP[num].FSTS * 4;
    else
        _OTG_DEVICE->EIPEMPMSK &= ~(1 << num);

#if (CORE_DEBUG)
    printk("S:%d\n", ep->size);
#endif //CORE_DEBUG

#if (PIN_DEBUG)
    gpio_set_pin(TEST1_PIN);
    memcpy4((void*)(_OTG_FIFO_BASE + num * 0x1000), io_data(ep->io) +  ep->size, size);
    gpio_reset_pin(TEST1_PIN);
#else
    memcpy4((void*)(_OTG_FIFO_BASE + num * 0x1000), io_data(ep->io) +  ep->size, size);
#endif // PIN_DEBUG

    ep->size += size;
}

static inline void stm32_otg_on_isr_tx(EXO* exo, uint32_t num)
{
    EP* ep = exo->usb.in[num];
    if (_OTG_DEVICE->INEP[num].INT & OTG_FS_DEVICE_ENDPOINT_INT_XFRC)
    {
        _OTG_DEVICE->INEP[num].INT = OTG_FS_DEVICE_ENDPOINT_INT_XFRC;
        ep->io_active = false;
        iio_complete(exo->usb.device, HAL_IO_CMD(HAL_USB, IPC_WRITE), USB_EP_IN | num, ep->io);
        ep->io = NULL;
        return;
    }

    if(_OTG_DEVICE->INEP[num].INT & OTG_FS_DEVICE_ENDPOINT_INT_TXFE/* && ep->io_active*/)
        stm32_otg_on_isr_tx_fifo_empty(exo, num);
}

static inline void usb_on_isr_reset()
{
    _OTG_DEVICE->CFG &= OTG_FS_DEVICE_CFG_DAD;
}

static void usb_on_isr(int vector, void* param)
{
    EXO* exo = param;
    unsigned int sta = _OTG_GENERAL->INTSTS;
#if (CORE_DEBUG)
    printk("%x\n",sta);
#endif //CORE_DEBUG
    //first two most often called
    if ((_OTG_GENERAL->INTMSK & OTG_FS_GENERAL_INTMSK_RXFLVLM) && (sta & OTG_FS_GENERAL_INTSTS_RXFLVL))
    {
        //mask interrupts, will be umasked by process after FIFO read
        _OTG_GENERAL->INTMSK &= ~OTG_FS_GENERAL_INTMSK_RXFLVLM;
        stm32_otg_on_isr_rx(exo);
        return;
    }

    uint32_t num;
    num = 32- __CLZ(_OTG_DEVICE->AINT & 0xff);
    if(num)
    {
        stm32_otg_on_isr_tx(exo, num-1);
        return;
    }

    //rarely called
    if (sta & OTG_FS_GENERAL_INTSTS_ENUMDNE)
    {
#if (CORE_DEBUG)
        printk("rst\n");
#endif //CORE_DEBUG
        usb_enumdne(exo);
        _OTG_GENERAL->INTSTS |= OTG_FS_GENERAL_INTSTS_ENUMDNE;
        return;
    }
    if ((sta & OTG_FS_GENERAL_INTSTS_USBSUSP) && (_OTG_GENERAL->INTMSK & OTG_FS_GENERAL_INTMSK_USBSUSPM))
    {
        usb_suspend(exo);
        _OTG_GENERAL->INTSTS |= OTG_FS_GENERAL_INTSTS_USBSUSP;
        return;
    }
    if (sta & OTG_FS_GENERAL_INTSTS_WKUPINT)
    {
        usb_wakeup(exo);
        _OTG_GENERAL->INTSTS |= OTG_FS_GENERAL_INTSTS_WKUPINT | OTG_FS_GENERAL_INTSTS_USBSUSP;
    }

    if (sta & OTG_FS_GENERAL_INTSTS_USBRST)
    {
        usb_on_isr_reset();
        _OTG_GENERAL->INTSTS |= OTG_FS_GENERAL_INTSTS_USBRST;
    }
    _OTG_GENERAL->OTGINT  = 0xFFFFFF;// clear other request
}
//------------------------------------------------------------------------
static void small_delay()
{
    int i;
    for(i = 0; i < 1000; i++) __NOP();
}

static void stm32_otg_open_device(EXO* exo, HANDLE device)
{
    int trdt;
    exo->usb.device = device;

    //enable GPIO
#if defined(STM32H7)
    PWR->CR3 |= PWR_CR3_USB33DEN;
#if (STM32_USB_CRYSTALLESS)
    RCC->CR |= RCC_CR_HSI48ON;
    while((RCC->CR & RCC_CR_HSI48RDY) == 0);
#endif

#if (USB_CLOCK_SRC == USB_CLOCK_SRC_PLL3_Q)
    stm32_power_pll3_on();
#endif

    RCC->D2CCIP2R = (RCC->D2CCIP2R & ~RCC_D2CCIP2R_USBSEL_Msk) | USB_CLOCK_SRC;

#if (STM32_USB_OTG_FS)
    RCC->AHB1ENR |= RCC_AHB1ENR_USB2OTGFSEN;
#endif
#if (STM32_USB_OTG_HS)
    RCC->AHB1ENR |= RCC_AHB1ENR_USB1OTGHSEN;
#if (STM32_USB_OTG_ULPI)
    RCC->AHB1ENR |= RCC_AHB1ENR_USB1OTGHSULPIEN;
#endif // STM32_USB_OTG_ULPI
#endif
    small_delay();
    trdt = 6;

#else // STM32H7
    stm32_pin_request_inside(exo, HAL_REQ(HAL_PIN, IPC_OPEN), A9, STM32_GPIO_MODE_INPUT_FLOAT, false);
    stm32_pin_request_inside(exo, HAL_REQ(HAL_PIN, IPC_OPEN), A10, STM32_GPIO_MODE_INPUT_PULL, true);
    //enable clock, setup prescaller
    switch (GET_CORE_CLOCK)
    {
    case 72000000:
        RCC->CFGR &= ~(1 << 22);
        break;
    case 48000000:
        RCC->CFGR |= 1 << 22;
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        return;
    }
#if defined(STM32F10X_CL)
    //enable clock
    RCC->AHBENR |= RCC_AHBENR_OTGFSEN;
    //F100, F101, F103
#elif defined(STM32F1)
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;
#endif
    trdt = GET_CORE_CLOCK == 72000000 ? 9 : 5;
    kirq_register(KERNEL_HANDLE, OTG_FS_IRQn, usb_on_isr, exo);
    NVIC_EnableIRQ(OTG_FS_IRQn);
    NVIC_SetPriority(OTG_FS_IRQn, 13);

#endif // STM32H7

#if (STM32_USB_OTG_ULPI)
    trdt = 9;
    _OTG_GENERAL->CCFG = 0;
    //disable HNP/SRP
    _OTG_GENERAL->OTGCTL = 0;
    //Device init: for stm32h7 must select phy and device mode before reset.
    _OTG_DEVICE->CFG = 0 | (1 << 14);
    _OTG_GENERAL->USBCFG =  (1 << 21) | (0 << 15) | OTG_FS_GENERAL_USBCFG_FDMOD | (trdt << OTG_FS_GENERAL_USBCFG_TRDT_POS) | (17 << OTG_FS_GENERAL_USBCFG_TOCAL_POS);
#else
    _OTG_GENERAL->CCFG = OTG_FS_GENERAL_CCFG_PWRDWN;
    //disable HNP/SRP
    _OTG_GENERAL->OTGCTL = 0;
    //Device init: for stm32h7 must select phy and device mode before reset.
    _OTG_DEVICE->CFG = OTG_FS_DEVICE_CFG_DSPD_FS;
    _OTG_GENERAL->USBCFG = (OTG_FS_GENERAL_USBCFG_FYLPC << 15) | OTG_FS_GENERAL_USBCFG_FDMOD | (trdt << OTG_FS_GENERAL_USBCFG_TRDT_POS) | OTG_FS_GENERAL_USBCFG_PHYSEL | (17 << OTG_FS_GENERAL_USBCFG_TOCAL_POS);
#endif // STM32_USB_OTG_ULPI

    //reset core
    _OTG_GENERAL->RSTCTL |= OTG_FS_GENERAL_RSTCTL_CSRST;
    while (_OTG_GENERAL->RSTCTL & OTG_FS_GENERAL_RSTCTL_CSRST) {};
    while ((_OTG_GENERAL->RSTCTL & OTG_FS_GENERAL_RSTCTL_AHBIDL) == 0) {}
    small_delay();

    //refer to programming manual: 1. Setup AHB
    _OTG_GENERAL->AHBCFG = OTG_FS_GENERAL_AHBCFG_GINT;

    _OTG_GENERAL->RXFSIZ = ((STM32_USB_MPS / 4) + 1) * 2 + 10 + 1;
    _OTG_GENERAL->TX0FSIZ = (0x20  << OTG_FS_GENERAL_TX0FSIZ_TX0FD_POS) | (_OTG_GENERAL->RXFSIZ << OTG_FS_GENERAL_TX0FSIZ_TX0FSA_POS);

    //repeat setup after reset
#if (STM32_USB_OTG_ULPI)
    _OTG_GENERAL->USBCFG =  (1 << 20) |  (0 << 15) | OTG_FS_GENERAL_USBCFG_FDMOD | (trdt << OTG_FS_GENERAL_USBCFG_TRDT_POS) | (17 << OTG_FS_GENERAL_USBCFG_TOCAL_POS);
#else
    _OTG_GENERAL->USBCFG = (1 << 15) | OTG_FS_GENERAL_USBCFG_FDMOD | (trdt << OTG_FS_GENERAL_USBCFG_TRDT_POS) | OTG_FS_GENERAL_USBCFG_PHYSEL | (17 << OTG_FS_GENERAL_USBCFG_TOCAL_POS);
#endif // STM32_USB_OTG_ULPI
    //2.
    _OTG_GENERAL->INTMSK = OTG_FS_GENERAL_INTMSK_USBSUSPM | OTG_FS_GENERAL_INTMSK_ENUMDNEM |
            OTG_FS_GENERAL_INTMSK_IEPM | OTG_FS_GENERAL_INTMSK_RXFLVLM | OTG_FS_GENERAL_INTMSK_USBRSTM ;

    //3.
    _OTG_GENERAL->CCFG |= OTG_FS_GENERAL_CCFG_VBUSBSEN;
    //disable endpoint interrupts, except IN XFRC
    _OTG_DEVICE->IEPMSK = OTG_FS_DEVICE_IEPMSK_XFRCM;
    _OTG_DEVICE->OEPMSK = 0;// | OTG_FS_DEVICE_IEPMSK_XFRCM;
    _OTG_DEVICE->CTL = OTG_FS_DEVICE_CTL_POPRGDNE;

    //Setup data FIFO
    _OTG_GENERAL->RXFSIZ = ((STM32_USB_MPS / 4) + 1) * 2 + 10 + 1;

    //enable interrupts
#if defined(STM32H7)
    kirq_register(KERNEL_HANDLE, _OTG_WKUP_IRQ, usb_on_isr, exo);
    NVIC_EnableIRQ(_OTG_WKUP_IRQ);
#endif// STM32H7

    kirq_register(KERNEL_HANDLE, _OTG_IRQ, usb_on_isr, exo);
    NVIC_EnableIRQ(_OTG_IRQ);
    NVIC_SetPriority(_OTG_IRQ, 13);

    //Unmask global interrupts
    _OTG_GENERAL->INTMSK |= OTG_FS_GENERAL_INTMSK_OTGM;
}

static inline void stm32_otg_open_ep(USB_PORT_TYPE port, EXO* exo, int num, USB_EP_TYPE type, unsigned int size)
{
    if (ep_data(port, exo, num) != NULL)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }
    EP* ep = kmalloc(sizeof(EP));
    if (ep == NULL)
        return;
    num & USB_EP_IN ? (exo->usb.in[USB_EP_NUM(num)] = ep) : (exo->usb.out[USB_EP_NUM(num)] = ep);
    ep->io = NULL;
    ep->mps = size;
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
#if (CORE_DEBUG)
        printk("EP %d open \n", num );
#endif //CORE_DEBUG

    if (num & USB_EP_IN)
    {
        uint32_t fifo_size;
        //setup TX FIFO num for IN endpoint
        fifo_used =_OTG_GENERAL->RXFSIZ & 0xffff;
        for (i = 0; i < USB_EP_COUNT_MAX; ++i)
        {
            if(exo->usb.in[i] == NULL)
                continue;

            fifo_size = exo->usb.in[i]->mps / 2;
            if(fifo_size < 16)
                fifo_size = 16;

            if(i)
                _OTG_GENERAL->DIEPTXF[i - 1] = (fifo_size << OTG_FS_GENERAL_TX0FSIZ_TX0FD_POS) | ((fifo_used * 1) <<OTG_FS_GENERAL_TX0FSIZ_TX0FSA_POS);
            else
                _OTG_GENERAL->TX0FSIZ = (fifo_size  << OTG_FS_GENERAL_TX0FSIZ_TX0FD_POS) | ((fifo_used * 1) << OTG_FS_GENERAL_TX0FSIZ_TX0FSA_POS);
            fifo_used += fifo_size;
        }
        ctl |= (USB_EP_NUM(num)) << OTG_FS_DEVICE_ENDPOINT_CTL_TXFNUM_POS;

        //enable interrupts for XFRCM
        _OTG_DEVICE->AINTMSK |= 1 << USB_EP_NUM(num);
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

    ep_reg_data(port, num)->CTL = ctl;
}

static inline void stm32_otg_close_ep(USB_PORT_TYPE port, EXO* exo, int num)
{
    if (!stm32_otg_ep_flush(port, exo, num))
        return;
    ep_reg_data(port, num)->CTL &= ~OTG_FS_DEVICE_ENDPOINT_CTL_USBAEP;
    EP* ep = ep_data(port, exo, num);

    ep->mps = 0;
    if (num & USB_EP_IN)
    {
        _OTG_DEVICE->AINTMSK &= ~(1 << USB_EP_NUM(num));
        _OTG_DEVICE->EIPEMPMSK &= ~(1 << USB_EP_NUM(num));
    }
    kfree(ep);
    num & USB_EP_IN ? (exo->usb.in[USB_EP_NUM(num)] = NULL) : (exo->usb.out[USB_EP_NUM(num)] = NULL);
}

static inline void stm32_otg_close_device(USB_PORT_TYPE port, EXO* exo)
{
    int i;
    //Mask global interrupts
    _OTG_GENERAL->INTMSK &= ~OTG_FS_GENERAL_INTMSK_OTGM;

    //disable interrupts
    NVIC_DisableIRQ(OTG_FS_IRQn);
    kirq_unregister(KERNEL_HANDLE, OTG_FS_IRQn);

    //close all endpoints
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        if (exo->usb.out[i] != NULL)
            stm32_otg_close_ep(port, exo, i);
        if (exo->usb.in[i] != NULL)
            stm32_otg_close_ep(port, exo, USB_EP_IN | i);
    }
    exo->usb.device = INVALID_HANDLE;

    //power down
#if defined(STM32F10X_CL)
    //disable clock
    RCC->AHBENR &= ~RCC_AHBENR_OTGFSEN;
    //F100, F101, F103
#elif defined(STM32F1)
    RCC->APB1ENR &= ~RCC_APB1ENR_USBEN;
#endif
#if defined(STM32H7)
    RCC->AHB1ENR &= ~(RCC_AHB1ENR_USB2OTGFSEN | RCC_AHB1ENR_USB1OTGHSEN | RCC_AHB1ENR_USB1OTGHSULPIEN);
#else
    //disable pins
    stm32_pin_request_inside(exo, HAL_REQ(HAL_PIN, IPC_CLOSE), A9, 0, 0);
    stm32_pin_request_inside(exo, HAL_REQ(HAL_PIN, IPC_CLOSE), A10, 0, 0);
#endif//STM32H7

}

static inline void stm32_otg_set_address(int addr)
{
#if (CORE_DEBUG)
        printk("set ADDR %d\n", addr );
#endif //CORE_DEBUG
    _OTG_DEVICE->CFG &= OTG_FS_DEVICE_CFG_DAD;
    _OTG_DEVICE->CFG |= addr << OTG_FS_DEVICE_CFG_DAD_POS;
}

static bool stm32_usb_io_prepare(EXO* exo, IPC* ipc)
{
    EP* ep = ep_data(USB_PORT(ipc->param1), exo, ipc->param1);
    if (ep == NULL)
    {
        printk("ep==null");
        kerror(ERROR_NOT_CONFIGURED);
        return false;
    }
    if (ep->io_active)
    {
        printk("ep->io_active");
        kerror(ERROR_IN_PROGRESS);
        return false;
    }
    ep->io = (IO*)ipc->param2;
    kerror(ERROR_SYNC);
    return true;
}

static inline void stm32_otg_read(EXO* exo, IPC* ipc)
{
    unsigned int ep_num = USB_EP_NUM(ipc->param1);
    EP* ep = exo->usb.out[ep_num];
    if (stm32_usb_io_prepare(exo, ipc))
    {
        ep->io->data_size = 0;
        ep->size = ipc->param3;
        ep->io_active = true;
        stm32_otg_rx_prepare(exo, ep_num);
    }
}

static inline void stm32_otg_write(EXO* exo, IPC* ipc)
{
    unsigned int ep_num = USB_EP_NUM(ipc->param1);
    EP* ep = exo->usb.in[ep_num];
    if (stm32_usb_io_prepare(exo, ipc))
    {
        ep->size = 0;
        ep->io_active = true;
        stm32_otg_tx(exo, USB_EP_IN | ep_num);
    }
}

#if (USB_TEST_MODE_SUPPORT)
static inline void stm32_otg_set_test_mode(EXO* exo, USB_TEST_MODES test_mode)
{
    _OTG_DEVICE->CTL &= ~OTG_FS_DEVICE_CTL_TCTL;
    _OTG_DEVICE->CTL |= test_mode << OTG_FS_DEVICE_CTL_TCTL_POS;
}
#endif //USB_TEST_MODE_SUPPORT

void stm32_otg_init(EXO* exo)
{
    int i;
    exo->usb.device = INVALID_HANDLE;
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
    {
        exo->usb.out[i] = NULL;
        exo->usb.in[i] = NULL;
    }
}

static inline void stm32_otg_device_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case USB_GET_SPEED:
        ipc->param2 = stm32_otg_get_speed(exo);
        break;
    case IPC_OPEN:
        stm32_otg_open_device(exo, ipc->process);
        break;
    case IPC_CLOSE:
        stm32_otg_close_device(USB_PORT(ipc->param1), exo);
        break;
    case USB_SET_ADDRESS:
        stm32_otg_set_address(ipc->param2);
        break;
#if (USB_TEST_MODE_SUPPORT)
    case USB_SET_TEST_MODE:
        stm32_otg_set_test_mode(exo, ipc->param2);
        break;
#endif //USB_TEST_MODE_SUPPORT
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}


static void stm32_otg_ep_request(EXO* exo, IPC* ipc)
{
    if (USB_EP_NUM(ipc->param1) >= USB_EP_COUNT_MAX)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        stm32_otg_open_ep(USB_PORT(ipc->param1), exo, USB_NUM(ipc->param1), ipc->param2, ipc->param3);
        break;
    case IPC_CLOSE:
        stm32_otg_close_ep(USB_PORT(ipc->param1), exo, USB_NUM(ipc->param1));
        break;
    case IPC_FLUSH:
        stm32_otg_ep_flush(USB_PORT(ipc->param1), exo, USB_NUM(ipc->param1));
        break;
    case USB_EP_SET_STALL:
        stm32_otg_ep_set_stall(USB_PORT(ipc->param1), exo, USB_NUM(ipc->param1));
        break;
    case USB_EP_CLEAR_STALL:
        stm32_otg_ep_clear_stall(USB_PORT(ipc->param1), exo, USB_NUM(ipc->param1));
        break;
    case USB_EP_IS_STALL:
        ipc->param2 = stm32_otg_ep_is_stall(USB_PORT(ipc->param1), USB_NUM(ipc->param1));
        break;
    case IPC_READ:
        stm32_otg_read(exo, ipc);
        break;
    case IPC_WRITE:
        stm32_otg_write(exo, ipc);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

void stm32_otg_request(EXO* exo, IPC* ipc)
{
    if (ipc->param1 == USB_HANDLE_DEVICE)
        stm32_otg_device_request(exo, ipc);
    else
        stm32_otg_ep_request(exo, ipc);
}
