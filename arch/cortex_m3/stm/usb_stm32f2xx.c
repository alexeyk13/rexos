/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "usb.h"
#include "usbd.h"
#include "gpio.h"
#include "gpio_stm32.h"
#include "hw_config.h"
#include "dbg.h"
#include "arch.h"
#include "mem_private.h"
#include "stm32f2xx_regsusb.h"
#include "rcc_stm32f2.h"
#include "error_private.h"

#define HS_CLK_PIN									GPIO_A5
#define HS_D0_PIN										GPIO_A3
#define HS_D1_PIN										GPIO_B0
#define HS_D2_PIN										GPIO_B1
#define HS_D3_PIN										GPIO_B10
#define HS_D4_PIN										GPIO_B11
#define HS_D5_PIN										GPIO_B12
#define HS_D6_PIN										GPIO_B13
#define HS_D7_PIN										GPIO_B5
#define HS_STP_PIN									GPIO_C0

#define USB_HS_PHY_MHZ								60

typedef struct {
	int ep_size;
	char* buf_in;
	unsigned int size_in;
	char* buf_out;
	unsigned int size_out;
	USB_EP_HANDLER usbd_on_in_writed;
	USB_EP_HANDLER usbd_on_out_readed;
	void* param;
}USBD_EP_HW;

typedef struct {
	const USBD_CALLBACKS* cb;
	void* param;
	const USB_DESCRIPTORS_TYPE* descriptors;
	USBD_EP_HW ep[8];
}USBD_HW;

static USBD_HW* _usb_handlers[USB_COUNT]  =			{NULL};

void usb_ep_nak(USB_CLASS idx, EP_CLASS ep)
{
	USB_HS_DEVICE_ENDPOINT_TypeDef* EP;
	if (IS_EP_IN(ep))
		EP = &(USB_HS_DEVICE->INEP[EP_NUM(ep)]);
	else
		EP = &(USB_HS_DEVICE->OUTEP[EP_NUM(ep)]);

	if ((EP->CTL & USB_HS_DEVICE_ENDPOINT_CTL_NAKSTS) == 0)
		EP->CTL |= USB_HS_DEVICE_ENDPOINT_CTL_SNAK;
}

void usb_ep_clear_nak(USB_CLASS idx, EP_CLASS ep)
{
	USB_HS_DEVICE_ENDPOINT_TypeDef* EP;
	if (IS_EP_IN(ep))
		EP = &(USB_HS_DEVICE->INEP[EP_NUM(ep)]);
	else
		EP = &(USB_HS_DEVICE->OUTEP[EP_NUM(ep)]);

	if (EP->CTL & USB_HS_DEVICE_ENDPOINT_CTL_NAKSTS)
		EP->CTL |= USB_HS_DEVICE_ENDPOINT_CTL_CNAK;
}

void usb_on_reset(USB_CLASS idx)
{
	//find maximal packet size and size for endpoints in our descriptor
	int mps = 64;
	int i;
	for (i = 0; i < 8; ++i)
		if (_usb_handlers[idx]->ep[i].ep_size > mps)
			mps = _usb_handlers[idx]->ep[i].ep_size;
	//setup RX/TX FIFO
	uint32_t fifo_addr = 0;
	uint32_t fifo_depth = ((mps / 4) + 1) * 2;
	USB_HS_GENERAL->RXSFSIZ = fifo_depth;
	fifo_addr += fifo_depth;
	fifo_depth = _usb_handlers[idx]->ep[0].ep_size / 4 * 2;
	USB_HS_GENERAL->TX0FSIZ =  (fifo_depth << USB_HS_GENERAL_TX0FSIZ_TX0FD_POS) | fifo_addr;
	fifo_addr += fifo_depth;
	for (i = 1; i < 8; ++i)
	{
		if (_usb_handlers[idx]->ep[i].ep_size)
		{
			fifo_depth = _usb_handlers[idx]->ep[i].ep_size / 4 * 2;
			USB_HS_GENERAL->DIEPTXF[i - 1] = (fifo_depth << USB_HS_GENERAL_TX0FSIZ_TX0FD_POS) | fifo_addr;
			fifo_addr += fifo_depth;
		}
		else
			USB_HS_GENERAL->DIEPTXF[i - 1] = 0;
	}

	//enable up to 3 B2B setup packets
	USB_HS_DEVICE->OUTEP[0].TSIZ = 3 << USB_HS_DEVICE_ENDPOINT_TSIZ_STUPCNT_OUT0_POS;
	//NAK all OUT endpoints
	for (i = 0; i < 8; ++i)
		usb_ep_nak(idx, EP_OUT(i));
	//unmask endpoint interrupts
	USB_HS_DEVICE->IEPMSK = USB_HS_DEVICE_IEPMSK_XFRCM;
	USB_HS_DEVICE->OEPMSK = USB_HS_DEVICE_OEPMSK_STUPM |	USB_HS_DEVICE_OEPMSK_XFRCM;

	USB_HS_DEVICE->IEPEACHMSK1 = USB_HS_DEVICE_IEPMSK_XFRCM;
	USB_HS_DEVICE->OEPEACHMSK1 = USB_HS_DEVICE_OEPMSK_XFRCM;

	_usb_handlers[idx]->cb->usbd_on_reset(_usb_handlers[idx]->param);
}

static inline void usb_read_chunk(USB_CLASS idx, EP_CLASS ep, int size)
{
	if (ep == EP_OUT0)
		USB_HS_DEVICE->OUTEP[EP_NUM(ep)].TSIZ &= ~(USB_HS_DEVICE_ENDPOINT_TSIZ_PKTCNT_OUT0 | USB_HS_DEVICE_ENDPOINT_TSIZ_XFRSIZ_OUT0);
	else
		USB_HS_DEVICE->OUTEP[EP_NUM(ep)].TSIZ &= ~(USB_HS_DEVICE_ENDPOINT_TSIZ_PKTCNT | USB_HS_DEVICE_ENDPOINT_TSIZ_XFRSIZ);
	if (size)
		USB_HS_DEVICE->OUTEP[EP_NUM(ep)].TSIZ |= (1 << USB_HS_DEVICE_ENDPOINT_TSIZ_PKTCNT_POS) | (size << USB_HS_DEVICE_ENDPOINT_TSIZ_XFRSIZ_POS);
	else
		USB_HS_DEVICE->OUTEP[EP_NUM(ep)].TSIZ |= (1 << USB_HS_DEVICE_ENDPOINT_TSIZ_PKTCNT_POS) | (_usb_handlers[idx]->ep[EP_NUM(ep)].ep_size << USB_HS_DEVICE_ENDPOINT_TSIZ_XFRSIZ_POS);
	USB_HS_DEVICE->OUTEP[EP_NUM(ep)].CTL |= USB_HS_DEVICE_ENDPOINT_CTL_EPENA | USB_HS_DEVICE_ENDPOINT_CTL_CNAK;
}

static inline void usb_write_chunk(USB_CLASS idx, EP_CLASS ep, int size)
{
	if (ep == EP_IN0)
		USB_HS_DEVICE->INEP[EP_NUM(ep)].TSIZ &= ~(USB_HS_DEVICE_ENDPOINT_TSIZ_PKTCNT_IN0 | USB_HS_DEVICE_ENDPOINT_TSIZ_XFRSIZ_IN0);
	else
		USB_HS_DEVICE->INEP[EP_NUM(ep)].TSIZ &= ~(USB_HS_DEVICE_ENDPOINT_TSIZ_PKTCNT | USB_HS_DEVICE_ENDPOINT_TSIZ_XFRSIZ);

	USB_HS_DEVICE->INEP[EP_NUM(ep)].TSIZ |= (1 << USB_HS_DEVICE_ENDPOINT_TSIZ_PKTCNT_POS) | (size << USB_HS_DEVICE_ENDPOINT_TSIZ_XFRSIZ_POS);
	USB_HS_DEVICE->INEP[EP_NUM(ep)].CTL |= USB_HS_DEVICE_ENDPOINT_CTL_EPENA | USB_HS_DEVICE_ENDPOINT_CTL_CNAK;
}

static inline void usb_on_ep_in(USB_CLASS idx, EP_CLASS ep)
{
	uint32_t sta = USB_HS_DEVICE->INEP[EP_NUM(ep)].INT;
	if (sta & USB_HS_DEVICE_ENDPOINT_INT_XFRC)
	{
		USB_HS_DEVICE->INEP[EP_NUM(ep)].INT = USB_HS_DEVICE_ENDPOINT_INT_XFRC;
		unsigned int ep_size = _usb_handlers[idx]->ep[EP_NUM(ep)].ep_size;
		if (_usb_handlers[idx]->ep[EP_NUM(ep)].size_in > ep_size)
		{
			_usb_handlers[idx]->ep[EP_NUM(ep)].size_in -= ep_size;
			unsigned int size = _usb_handlers[idx]->ep[EP_NUM(ep)].size_in;
			usb_write_chunk(idx, ep, size > ep_size ? ep_size : size);
		}
		else
		{
			usb_ep_nak(idx, ep);
			_usb_handlers[idx]->ep[EP_NUM(ep)].usbd_on_in_writed(ep, _usb_handlers[idx]->ep[EP_NUM(ep)].param);
		}
	}
}

static inline void usb_on_ep_out(USB_CLASS idx, EP_CLASS ep)
{
	uint32_t sta = USB_HS_DEVICE->OUTEP[EP_NUM(ep)].INT;
	if (sta & USB_HS_DEVICE_ENDPOINT_INT_XFRC)
	{
		USB_HS_DEVICE->OUTEP[EP_NUM(ep)].INT = USB_HS_DEVICE_ENDPOINT_INT_XFRC;
		unsigned int ep_size = _usb_handlers[idx]->ep[EP_NUM(ep)].ep_size;
		if (_usb_handlers[idx]->ep[EP_NUM(ep)].size_out > ep_size)
		{
			_usb_handlers[idx]->ep[EP_NUM(ep)].size_out -= ep_size;
			unsigned int size = _usb_handlers[idx]->ep[EP_NUM(ep)].size_out;
			usb_read_chunk(idx, ep, size > ep_size ? ep_size : size);
		}
		else
		{
			usb_ep_nak(idx, ep);
			_usb_handlers[idx]->ep[EP_NUM(ep)].usbd_on_out_readed(ep, _usb_handlers[idx]->ep[EP_NUM(ep)].param);
		}
	}
	else if (sta & USB_HS_DEVICE_ENDPOINT_INT_STUP)
	{
		USB_HS_DEVICE->OUTEP[EP_NUM(ep)].INT = USB_HS_DEVICE_ENDPOINT_INT_STUP;
		_usb_handlers[idx]->cb->usbd_on_setup(_usb_handlers[idx]->param);
	}
}

void OTG_HS_IRQHandler(void)
{
	uint32_t sta = USB_HS_GENERAL->INTSTS;
	if (sta & USB_HS_GENERAL_INTSTS_OEPINT)
		usb_on_ep_out(USB_1, EP_OUT(31 - __CLZ((USB_HS_DEVICE->AINT) & 0xffff0000) - 16));
	if (sta & USB_HS_GENERAL_INTSTS_IEPINT)
		usb_on_ep_in(USB_1, EP_IN(31 - __CLZ((USB_HS_DEVICE->AINT) & 0x0000ffff)));
	else if (sta & USB_HS_GENERAL_INTSTS_USBRST)
	{
		usb_on_reset(USB_1);
		USB_HS_GENERAL->INTSTS |= USB_HS_GENERAL_INTSTS_USBRST;
	}
	else if (sta & USB_HS_GENERAL_INTSTS_WKUPINT)
	{
		_usb_handlers[USB_1]->cb->usbd_on_resume(_usb_handlers[USB_1]->param);
		USB_HS_GENERAL->INTSTS |= USB_HS_GENERAL_INTSTS_WKUPINT;
	}
	else if (sta & USB_HS_GENERAL_INTSTS_USBSUSP)
	{
		_usb_handlers[USB_1]->cb->usbd_on_suspend(_usb_handlers[USB_1]->param);
		USB_HS_GENERAL->INTSTS |= USB_HS_GENERAL_INTSTS_USBSUSP;
	}
	else if (sta & USB_HS_GENERAL_INTSTS_ESUSP)
	{
		_usb_handlers[USB_1]->cb->usbd_on_suspend(_usb_handlers[USB_1]->param);
		USB_HS_GENERAL->INTSTS |= USB_HS_GENERAL_INTSTS_ESUSP;
	}
}

void OTG_HS_EP1_OUT_IRQHandler(void)
{
	usb_on_ep_out(USB_1, EP_OUT1);
}

void OTG_HS_EP1_IN_IRQHandler(void)
{
	usb_on_ep_in(USB_1, EP_IN1);
}

void usb_enable_device(USB_CLASS idx, USB_ENABLE_DEVICE_PARAMS *params, int priority)
{
	if (idx < USB_COUNT)
	{
		_usb_handlers[idx] = (USBD_HW*)sys_alloc(sizeof(USBD_HW));
		if (_usb_handlers[idx])
		{
			_usb_handlers[idx]->cb = params->cb;
			_usb_handlers[idx]->param = params->param;
			_usb_handlers[idx]->descriptors = params->descriptors;

			int i, j, k;
			_usb_handlers[idx]->ep[0].ep_size = _usb_handlers[idx]->descriptors->p_device->bMaxPacketSize0;
			for (i = 1; i < 8; ++i)
				_usb_handlers[idx]->ep[i].ep_size = 0;
			for (i = 0; i < _usb_handlers[idx]->descriptors->num_of_configurations; ++i)
			{
				USB_CONFIGURATION_DESCRIPTOR_TYPE* configuration = _usb_handlers[idx]->descriptors->pp_configurations[i];
				P_USB_INTERFACE_DESCRIPTOR_TYPE iface = (P_USB_INTERFACE_DESCRIPTOR_TYPE)((char*)configuration + configuration->bLength);
				for (j = 0; j < configuration->bNumInterfaces; ++j)
				{
					ASSERT(iface->bDescriptorType == USB_INTERFACE_DESCRIPTOR_INDEX);
					P_USB_ENDPOINT_DESCRIPTOR_TYPE ep = (P_USB_ENDPOINT_DESCRIPTOR_TYPE)((char*)iface + iface->bLength);
					for (k = 0; k < iface->bNumEndpoints; ++k)
					{
						ASSERT(ep->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_INDEX);

						if (IS_EP_IN(ep->bEndpointAddress))
							if (_usb_handlers[idx]->ep[EP_NUM(ep->bEndpointAddress)].ep_size < ep->wMaxPacketSize)
							{
								_usb_handlers[idx]->ep[EP_NUM(ep->bEndpointAddress)].ep_size = ep->wMaxPacketSize;
							}

						ep = (P_USB_ENDPOINT_DESCRIPTOR_TYPE)((char*)ep + ep->bLength);
					}

					iface = (P_USB_INTERFACE_DESCRIPTOR_TYPE)((char*)iface + iface->bLength + iface->bNumEndpoints * USB_ENDPOINT_DESCRIPTOR_SIZE);
				}
			}
			//enable pins
			gpio_enable_afio(HS_CLK_PIN, AFIO_MODE_OTG);
			gpio_enable_afio(HS_D0_PIN, AFIO_MODE_OTG);
			gpio_enable_afio(HS_D1_PIN, AFIO_MODE_OTG);
			gpio_enable_afio(HS_D2_PIN, AFIO_MODE_OTG);
			gpio_enable_afio(HS_D3_PIN, AFIO_MODE_OTG);
			gpio_enable_afio(HS_D4_PIN, AFIO_MODE_OTG);
			gpio_enable_afio(HS_D5_PIN, AFIO_MODE_OTG);
			gpio_enable_afio(HS_D6_PIN, AFIO_MODE_OTG);
			gpio_enable_afio(HS_D7_PIN, AFIO_MODE_OTG);
			gpio_enable_afio(HS_NXT_PIN, AFIO_MODE_OTG);
			gpio_enable_afio(HS_DIR_PIN, AFIO_MODE_OTG);
			gpio_enable_afio(HS_STP_PIN, AFIO_MODE_OTG);

			RCC->AHB1ENR |= RCC_AHB1ENR_OTGHSEN | RCC_AHB1ENR_OTGHSULPIEN;

			//ULPI phy, internal phy VBus
			USB_HS_GENERAL->USBCFG = USB_HS_GENERAL_USBCFG_FDMOD | ((_ahb_freq / 1000000 * 4 / USB_HS_PHY_MHZ + 1) << USB_HS_GENERAL_USBCFG_TRDT_POS);
#if (USB_STM_RESET_CORE)
			//reset core after phy selection
			while ((USB_HS_GENERAL->RSTCTL & USB_HS_GENERAL_RSTCTL_AHBIDL) == 0) {}
			USB_HS_GENERAL->RSTCTL |= USB_HS_GENERAL_RSTCTL_CSRST;
			while (USB_HS_GENERAL->RSTCTL & USB_HS_GENERAL_RSTCTL_CSRST) {};
			__NOP();
			__NOP();
			__NOP();
			__NOP();
			__NOP();
			__NOP();
#endif //USB_STM_RESET_CORE
			//setup global registers
			//disable OTG
			USB_HS_GENERAL->OTGCTL &= ~(USB_HS_GENERAL_OTGCTL_DHNPEN | USB_HS_GENERAL_OTGCTL_HSHNPEN | USB_HS_GENERAL_OTGCTL_HNPRQ | USB_HS_GENERAL_OTGCTL_SRQ);
			//setup AHB. Mask interrupts for now. TxFIFO half empty, enabling double buffering
	#ifdef USB_DMA_ENABLED
			USB_HS_GENERAL->AHBCFG = USB_HS_GENERAL_AHBCFG_DMAEN | USB_HS_GENERAL_AHBCFG_HBSTLEN_INCR16;
			//setup threshold for dma, same as AHB
			USB_HS_DEVICE->THRCTL = USB_HS_DEVICE_THRCTL_RXTHREN | USB_HS_DEVICE_THRCTL_NONISOTHREN | USB_HS_DEVICE_THRCTL_ISOTHREN |
											(16 << USB_HS_DEVICE_THRCTL_RXTHRLEN_POS) | (16 << USB_HS_DEVICE_THRCTL_TXTHRLEN_POS);
	#else
			USB_HS_GENERAL->AHBCFG = 0;
	#endif //USB_DMA_ENABLED
			//HS, address 0
			USB_HS_DEVICE->CFG = 0;

			//enable USB core interrupts
			USB_HS_GENERAL->INTMSK = USB_HS_GENERAL_INTMSK_WKUPM | USB_HS_GENERAL_INTMSK_USBRSTM | USB_HS_GENERAL_INTMSK_USBSUSPM | USB_HS_GENERAL_INTMSK_ESUSPM |
											 USB_HS_GENERAL_INTMSK_OEPM | USB_HS_GENERAL_INTMSK_IEPM;
			//disable endpoint interrupts
			USB_HS_DEVICE->IEPMSK = 0;
			USB_HS_DEVICE->OEPMSK = 0;
			USB_HS_DEVICE->IEPEACHMSK1 = 0;
			USB_HS_DEVICE->OEPEACHMSK1 = 0;

			//unmask global interrupts bit
			USB_HS_GENERAL->AHBCFG |= USB_HS_GENERAL_AHBCFG_GINT;

			//enable irq
			NVIC_EnableIRQ(OTG_HS_IRQn);
			NVIC_EnableIRQ(OTG_HS_EP1_IN_IRQn);
			NVIC_EnableIRQ(OTG_HS_EP1_OUT_IRQn);
			NVIC_EnableIRQ(OTG_HS_WKUP_IRQn);
			//irq priority
			NVIC_SetPriority(OTG_HS_IRQn, priority);
			NVIC_SetPriority(OTG_HS_EP1_IN_IRQn, priority);
			NVIC_SetPriority(OTG_HS_EP1_OUT_IRQn, priority);
			NVIC_SetPriority(OTG_HS_WKUP_IRQn, priority);
		}
		else
			error_dev(ERROR_MEM_OUT_OF_SYSTEM_MEMORY, DEV_USB, idx);
	}
	else
		error_dev(ERROR_DEVICE_INDEX_OUT_OF_RANGE, DEV_USB, idx);

}

void usb_disable(USB_CLASS idx)
{
	if (idx < USB_COUNT)
	{
		NVIC_DisableIRQ(OTG_HS_IRQn);
		NVIC_DisableIRQ(OTG_HS_EP1_IN_IRQn);
		NVIC_DisableIRQ(OTG_HS_EP1_OUT_IRQn);
		NVIC_DisableIRQ(OTG_HS_WKUP_IRQn);

		//mask global interrupts bit
		USB_HS_GENERAL->AHBCFG &= ~USB_HS_GENERAL_AHBCFG_GINT;
		//disable clocks
		RCC->AHB1ENR &= ~(RCC_AHB1ENR_OTGHSEN | RCC_AHB1ENR_OTGHSULPIEN);

		sys_free(_usb_handlers[idx]);
		_usb_handlers[idx] = NULL;

		//disable pins
		gpio_disable_pin(HS_CLK_PIN);
		gpio_disable_pin(HS_D0_PIN);
		gpio_disable_pin(HS_D1_PIN);
		gpio_disable_pin(HS_D2_PIN);
		gpio_disable_pin(HS_D3_PIN);
		gpio_disable_pin(HS_D4_PIN);
		gpio_disable_pin(HS_D5_PIN);
		gpio_disable_pin(HS_D6_PIN);
		gpio_disable_pin(HS_D7_PIN);
		gpio_disable_pin(HS_NXT_PIN);
		gpio_disable_pin(HS_DIR_PIN);
		gpio_disable_pin(HS_STP_PIN);
	}
	else
		error_dev(ERROR_DEVICE_INDEX_OUT_OF_RANGE, DEV_USB, idx);
}

void usb_ep_enable(USB_CLASS idx, EP_CLASS ep, USB_EP_HANDLER cb, void *param, int max_packet_size, EP_TYPE_CLASS type)
{
	USB_HS_DEVICE_ENDPOINT_TypeDef* EP;
	if (IS_EP_IN(ep))
	{
		_usb_handlers[idx]->ep[EP_NUM(ep)].usbd_on_in_writed = cb;
		EP = &(USB_HS_DEVICE->INEP[EP_NUM(ep)]);
	}
	else
	{
		_usb_handlers[idx]->ep[EP_NUM(ep)].usbd_on_out_readed = cb;
		EP = &(USB_HS_DEVICE->OUTEP[EP_NUM(ep)]);
	}
	_usb_handlers[idx]->ep[EP_NUM(ep)].param = param;

	//setup ep type, DATA0 for bulk/interrupt, EVEN frame for isochronous endpoint
	switch (type)
	{
	case EP_TYPE_CONTROL:
		EP->CTL = USB_HS_DEVICE_ENDPOINT_CTL_EPTYP_CONTROL;
		break;
	case EP_TYPE_BULK:
		EP->CTL = USB_HS_DEVICE_ENDPOINT_CTL_EPTYP_BULK | USB_HS_DEVICE_ENDPOINT_CTL_DPID;
		break;
	case EP_TYPE_INTERRUPT:
		EP->CTL = USB_HS_DEVICE_ENDPOINT_CTL_EPTYP_INTERRUPT | USB_HS_DEVICE_ENDPOINT_CTL_DPID;
		break;
	case EP_TYPE_ISOCHRON:
		EP->CTL = USB_HS_DEVICE_ENDPOINT_CTL_EPTYP_ISOCHRONOUS | USB_HS_DEVICE_ENDPOINT_CTL_SEVNFRM;
		break;
	}

	//setup TX FIFO num for in endpoint
	if (IS_EP_IN(ep))
		EP->CTL |= (EP_NUM(ep) << USB_HS_DEVICE_ENDPOINT_CTL_TXFNUM_POS);

	//EP_OUT0 has differrent mps structure
	if (ep == EP_OUT0)
	{
		switch (max_packet_size)
		{
		case 8:
			EP->CTL |= USB_HS_DEVICE_ENDPOINT_CTL_MPSIZ0_8;
			break;
		case 16:
			EP->CTL |= USB_HS_DEVICE_ENDPOINT_CTL_MPSIZ0_16;
			break;
		case 32:
			EP->CTL |= USB_HS_DEVICE_ENDPOINT_CTL_MPSIZ0_32;
			break;
		default:
			EP->CTL |= USB_HS_DEVICE_ENDPOINT_CTL_MPSIZ0_64;
		}
	}
	else
		EP->CTL |= max_packet_size;

	//enable EP
	EP->CTL |= USB_HS_DEVICE_ENDPOINT_CTL_USBAEP;

	//unmask interrupts
	if (EP_NUM(ep) == 1)
		USB_HS_DEVICE->EACHINTMSK |= (IS_EP_IN(ep) ? USB_HS_DEVICE_EACHINTMSK_IEP1INTM : USB_HS_DEVICE_EACHINTMSK_OEP1INTM);

	USB_HS_DEVICE->AINTMSK |= (1 << (IS_EP_IN(ep) ? EP_NUM(ep + 0) : EP_NUM(ep + 16)));

	//prepare buffers
	_usb_handlers[idx]->ep[EP_NUM(ep)].ep_size = max_packet_size;
	if (IS_EP_IN(ep))
	{
		_usb_handlers[idx]->ep[EP_NUM(ep)].buf_in = NULL;
		_usb_handlers[idx]->ep[EP_NUM(ep)].size_in = 0;
	}
	else
	{
		_usb_handlers[idx]->ep[EP_NUM(ep)].buf_out = NULL;
		_usb_handlers[idx]->ep[EP_NUM(ep)].size_out = 0;
	}
	usb_ep_nak(idx, ep);
}

void usb_ep_disable(USB_CLASS idx, EP_CLASS ep)
{
	USB_HS_DEVICE_ENDPOINT_TypeDef* EP;
	if (IS_EP_IN(ep))
		EP = &(USB_HS_DEVICE->INEP[EP_NUM(ep)]);
	else
		EP = &(USB_HS_DEVICE->OUTEP[EP_NUM(ep)]);

	EP->CTL &= ~USB_HS_DEVICE_ENDPOINT_CTL_USBAEP;
	//mask interrupts
	if (EP_NUM(ep) == 1)
		USB_HS_DEVICE->EACHINTMSK &= ~(IS_EP_IN(ep) ? USB_HS_DEVICE_EACHINTMSK_IEP1INTM : USB_HS_DEVICE_EACHINTMSK_OEP1INTM);
	else
		USB_HS_DEVICE->AINTMSK &= ~(1 << (IS_EP_IN(ep) ? EP_NUM(ep + 0) : EP_NUM(ep + 16)));
	usb_ep_nak(idx, ep);
}

void usb_ep_stall(USB_CLASS idx, EP_CLASS ep)
{
	USB_HS_DEVICE_ENDPOINT_TypeDef* EP;
	if (IS_EP_IN(ep))
		EP = &(USB_HS_DEVICE->INEP[EP_NUM(ep)]);
	else
		EP = &(USB_HS_DEVICE->OUTEP[EP_NUM(ep)]);

	if ((EP->CTL & USB_HS_DEVICE_ENDPOINT_CTL_STALL) == 0)
		EP->CTL |= USB_HS_DEVICE_ENDPOINT_CTL_STALL;
}

void usb_ep_clear_stall(USB_CLASS idx, EP_CLASS ep)
{
	USB_HS_DEVICE_ENDPOINT_TypeDef* EP;
	if (IS_EP_IN(ep))
		EP = &(USB_HS_DEVICE->INEP[EP_NUM(ep)]);
	else
		EP = &(USB_HS_DEVICE->OUTEP[EP_NUM(ep)]);

	if (EP->CTL & USB_HS_DEVICE_ENDPOINT_CTL_STALL)
		EP->CTL &= ~USB_HS_DEVICE_ENDPOINT_CTL_STALL;
}

bool usb_ep_is_stall(USB_CLASS idx, EP_CLASS ep)
{
	USB_HS_DEVICE_ENDPOINT_TypeDef* EP;
	if (IS_EP_IN(ep))
		EP = &(USB_HS_DEVICE->INEP[EP_NUM(ep)]);
	else
		EP = &(USB_HS_DEVICE->OUTEP[EP_NUM(ep)]);

	return (EP->CTL & USB_HS_DEVICE_ENDPOINT_CTL_STALL) ? true : false;
}

void usb_read(USB_CLASS idx, EP_CLASS ep, char* buf, int size)
{
	ASSERT(IS_EP_OUT(ep));
	if (USB_HS_DEVICE->OUTEP[EP_NUM(ep)].CTL & USB_HS_DEVICE_ENDPOINT_CTL_EPENA)
	{
		USB_HS_DEVICE->OUTEP[EP_NUM(ep)].CTL |= USB_HS_DEVICE_ENDPOINT_CTL_EPDIS;
		while (USB_HS_DEVICE->OUTEP[EP_NUM(ep)].CTL & USB_HS_DEVICE_ENDPOINT_CTL_EPDIS) {}
	}
	USB_HS_DEVICE->OUTEP[EP_NUM(ep)].DMAADDR = (uint32_t)buf;
	_usb_handlers[idx]->ep[EP_NUM(ep)].size_out = size;
	_usb_handlers[idx]->ep[EP_NUM(ep)].buf_out = buf;
	unsigned int ep_size = _usb_handlers[idx]->ep[EP_NUM(ep)].ep_size;
	usb_read_chunk(idx, ep, size > ep_size ? ep_size : size);
}

void usb_write(USB_CLASS idx, EP_CLASS ep, char* buf, int size)
{
	ASSERT(IS_EP_IN(ep));
	if (USB_HS_DEVICE->INEP[EP_NUM(ep)].CTL & USB_HS_DEVICE_ENDPOINT_CTL_EPENA)
	{
		USB_HS_DEVICE->INEP[EP_NUM(ep)].CTL |= USB_HS_DEVICE_ENDPOINT_CTL_EPDIS;
		while (USB_HS_DEVICE->INEP[EP_NUM(ep)].CTL & USB_HS_DEVICE_ENDPOINT_CTL_EPDIS) {}
	}
	USB_HS_DEVICE->INEP[EP_NUM(ep)].DMAADDR = (uint32_t)buf;
	_usb_handlers[idx]->ep[EP_NUM(ep)].size_in = size;
	_usb_handlers[idx]->ep[EP_NUM(ep)].buf_in = buf;
	unsigned int ep_size = _usb_handlers[idx]->ep[EP_NUM(ep)].ep_size;
	usb_write_chunk(idx, ep, size > ep_size ? ep_size : size);
}

void usb_set_address(USB_CLASS idx, uint8_t address)
{
	USB_HS_DEVICE->CFG &= USB_HS_DEVICE_CFG_DAD;
	USB_HS_DEVICE->CFG |= address << USB_HS_DEVICE_CFG_DAD_POS;
}

USB_SPEED usb_get_current_speed(USB_CLASS idx)
{
	USB_SPEED res = USB_HIGH_SPEED;
	switch (USB_HS_DEVICE->STS & USB_HS_DEVICE_STS_ENUMSPD)
	{
	case USB_HS_DEVICE_STS_ENUMSPD_FS:
		res = USB_FULL_SPEED;
		break;
	default:
		break;
	}
	return res;
}
