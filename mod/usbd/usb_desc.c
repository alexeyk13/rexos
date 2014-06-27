/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "usb_desc.h"
#include "dbg.h"

P_USB_CONFIGURATION_DESCRIPTOR_TYPE descriptors_get_configuration(P_USB_DESCRIPTORS_TYPE descriptors, uint8_t configuration)
{
	int i;
	P_USB_CONFIGURATION_DESCRIPTOR_TYPE res = NULL;
	for (i = 0; i < descriptors->num_of_configurations; ++i)
	{
		ASSERT(descriptors->pp_configurations[i]->bDescriptorType == USB_CONFIGURATION_DESCRIPTOR_INDEX);
		if (descriptors->pp_configurations[i]->bConfigurationValue == configuration)
		{
			res = descriptors->pp_configurations[i];
			break;
		}
	}
	return res;
}

P_USB_CONFIGURATION_DESCRIPTOR_TYPE descriptors_get_other_speed_configuration(P_USB_DESCRIPTORS_TYPE descriptors, uint8_t configuration)
{
	int i;
	P_USB_CONFIGURATION_DESCRIPTOR_TYPE res = NULL;
	if (descriptors != NULL)
	{
		for (i = 0; i < descriptors->num_of_configurations; ++i)
		{
			if (descriptors->pp_fs_configurations)
			{
				ASSERT(descriptors->pp_configurations[i]->bDescriptorType == USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX);
				if (descriptors->pp_fs_configurations[i]->bConfigurationValue == configuration)
				{
					res = descriptors->pp_fs_configurations[i];
					break;
				}
			}
		}
	}
	return res;
}

P_USB_INTERFACE_DESCRIPTOR_TYPE configuration_get_interface(P_USB_CONFIGURATION_DESCRIPTOR_TYPE configuration, uint8_t iface_num, uint8_t iface_alt_num)
{
	P_USB_INTERFACE_DESCRIPTOR_TYPE res = NULL;
	if (configuration != NULL)
	{
		P_USB_INTERFACE_DESCRIPTOR_TYPE iface;
		int i;
		iface = (P_USB_INTERFACE_DESCRIPTOR_TYPE)((char*)configuration + configuration->bLength);
		for (i = 0; i < configuration->bNumInterfaces; ++i)
		{
			ASSERT(iface->bDescriptorType == USB_INTERFACE_DESCRIPTOR_INDEX);
			if (iface->bInterfaceNumber == iface_num && iface->bAlternateSetting == iface_alt_num)
			{
				res = iface;
				break;
			}
			iface = (P_USB_INTERFACE_DESCRIPTOR_TYPE)((char*)iface + iface->bLength + iface->bNumEndpoints * USB_ENDPOINT_DESCRIPTOR_SIZE);
		}
	}
	return res;
}

P_USB_ENDPOINT_DESCRIPTOR_TYPE interface_get_endpoint(P_USB_INTERFACE_DESCRIPTOR_TYPE iface, uint8_t ep_num)
{
	P_USB_ENDPOINT_DESCRIPTOR_TYPE res = NULL;
	if (iface != NULL)
	{
		P_USB_ENDPOINT_DESCRIPTOR_TYPE ep;
		int i;
		ep = (P_USB_ENDPOINT_DESCRIPTOR_TYPE)((char*)iface + iface->bLength);
		for (i = 0; i < iface->bNumEndpoints; ++i)
		{
			ASSERT(ep->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_INDEX);
			if (ep->bEndpointAddress == ep_num)
			{
				res = ep;
				break;
			}
			ep = (P_USB_ENDPOINT_DESCRIPTOR_TYPE)((char*)ep + ep->bLength);
		}
	}
	return res;
}

