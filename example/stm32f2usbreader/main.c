#include "gpio_user.h"
#include "config.h"
#include "gpio_user.h"
#include "gpio.h"
#include "sd_card.h"
#include "storage.h"
#include "usbd.h"
#include "usb_msc.h"
#include "usb_desc_user.h"
#include "printf.h"

#include "gpio_stm32.h"

const SCSI_DESCRIPTOR _scsi_descriptor = {
	0x00,
	"My company",
	"Flash cardreader",
	"0.1",
	"MY_CMPNY Flash cardreader"
};

SD_CARD*						_sd_card;
STORAGE_HOST_CB			_storage_cb;

USBD*							_usbd;
USB_MSC*						_msc;
USBD_STATE_CB				_state_cb;

void io_monitor(void* param, STORAGE_STATE state)
{
	switch (state)
	{
	case STORAGE_STATE_IDLE:
		led_off(LED_2);
		break;
	default:
		led_on(LED_2);
		break;
	}
}

void usb_state_changed(USBD_STATE state, USBD_STATE prior_state, void* param)
{
	switch (state)
	{
	case USBD_STATE_CONFIGURED:
		led_on(LED_1);
		printf("USB plugged\n\r");
		break;
	case USBD_STATE_SUSPENDED:
		if (prior_state == USBD_STATE_CONFIGURED)
		{
			led_off(LED_1);
			printf("USB unplugged\n\r");
		}
		break;
	default:
		break;
	}
}

void application_init(void)
{
	gpio_user_init();

	_sd_card = sd_card_create(SDIO_1, SDIO_IRQ_PRIORITY);

	_storage_cb.state_changed = io_monitor;
	_storage_cb.media_state_changed = NULL;
	_storage_cb.param = NULL;
	storage_register_handler(sd_card_get_storage(_sd_card), &_storage_cb);

	_usbd = usbd_create(USB_1, _p_usb_descriptors, USB_IRQ_PRIORITY);
	_state_cb.state_handler = usb_state_changed;
	_state_cb.param = NULL;
	usbd_register_state_callback(_usbd, &_state_cb);
	_msc = usb_msc_create(_usbd, 1, sd_card_get_storage(_sd_card), (P_SCSI_DESCRIPTOR)&_scsi_descriptor);
}

void idle_task(void)
{
	for (;;)
	{
		__WFI();
	}
}
