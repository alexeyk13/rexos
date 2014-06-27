#include "usb_desc_user.h"

#define USB_MANUFACTURER_STR_INDEX						0x01
#define USB_PRODUCT_STR_INDEX								0x02
#define USB_CONFIGURATION1_STR_INDEX					0x03
#define USB_INTERFACE1_C1_STR_INDEX						0x04
#define USB_SERIALNUM_STR_INDEX							0x05

#define USB_STRING_DESCRIPTORS_STRINGS_COUNT			5
#define USB_STRING_DESCRIPTORS_LANGUAGES_COUNT		1

typedef struct {
	uint8_t	bLength;					//Size of this descriptor in bytes
	uint8_t  bDescriptorType;		//STRING Descriptor Type
	uint16_t data[USB_STRING_DESCRIPTORS_LANGUAGES_COUNT];
} USB_MAIN_STRING_DESCRIPTOR_TYPE, *P_USB_MAIN_STRING_DESCRIPTOR_TYPE;

#pragma pack(push, 1)

const USB_DEVICE_DESCRIPTOR_TYPE USB_DEVICE_DESCRIPTOR __attribute__((aligned(4))) = {
	USB_DEVICE_DESCRIPTOR_SIZE,						/*bLength */
	USB_DEVICE_DESCRIPTOR_INDEX,						/*bDescriptorType*/
	0x0200,													/*bcdUSB */
	0x00,														/*bDeviceClass*/
	0x00,														/*bDeviceSubClass*/
	0x00,														/*bDeviceProtocol*/
	USB_MAX_EP0_SIZE,										/*bMaxPacketSize*/
	0x2345,													/*idVendor*/
	0x1234,													/*idProduct*/
	0x0100,													/*bcdDevice release*/
	USB_MANUFACTURER_STR_INDEX,						/*Index of manufacturer  string*/
	USB_PRODUCT_STR_INDEX,								/*Index of product string*/
	USB_SERIALNUM_STR_INDEX,							/*Index of serial number string*/
	1															/*bNumConfigurations*/
};

const USB_QUALIFIER_DESCRIPTOR_TYPE USB_QUALIFIER_DESCRIPTOR __attribute__((aligned(4))) = {
	USB_QUALIFIER_DESCRIPTOR_SIZE,					/*bLength*/
	USB_DEVICE_QUALIFIER_DESCRIPTOR_INDEX,			/*bDescriptorType*/
	0x0200,													/*bcdUSB*/
	0,															/*bDeviceClass*/
	0,															/*bDeviceSubClass*/
	0,															/*bDeviceProtocol*/
	USB_MAX_EP0_SIZE,										/*bMaxPacketSize0*/
	1,															/*bNumConfiguration*/
	0															/*bReserved*/
};

typedef struct {
	USB_CONFIGURATION_DESCRIPTOR_TYPE configuration;
	USB_INTERFACE_DESCRIPTOR_TYPE interface;
	USB_ENDPOINT_DESCRIPTOR_TYPE endpoints[2];
} USB_USER_CONFIGURATION_TYPE, *P_USB_USER_CONFIGURATION_TYPE;

const USB_USER_CONFIGURATION_TYPE USB_USER_CONFIGURATION __attribute__((aligned(4))) = {
	//configuration descriptor
	{
		USB_CONFIGURATION_DESCRIPTOR_SIZE,			/*bLength*/
		USB_CONFIGURATION_DESCRIPTOR_INDEX,			/*bDescriptorType*/
		USB_CONFIGURATION_DESCRIPTOR_SIZE +			/*wTotalLength*/
		USB_INTERFACE_DESCRIPTOR_SIZE +
		USB_ENDPOINT_DESCRIPTOR_SIZE * 2,
		1,														/*bNumInterfaces*/
		1,														/*bConfigurationValue*/
		USB_CONFIGURATION1_STR_INDEX,					/*iConfiguration*/
		0x80,													/*bmAttributes*/
		50														/*bMaxPower*/
	},
	//interface descriptor
	{
		USB_INTERFACE_DESCRIPTOR_SIZE,				/*bLength*/
		USB_INTERFACE_DESCRIPTOR_INDEX,				/*bDescriptorType*/
		0,														/*bInterfaceNumber*/
		0,														/*bAlternateSetting*/
		2,														/*bNumEndpoints*/
		0x08,													/*bInterfaceClass*/
		0x06,													/*bInterfaceSubClass*/
		0x50,													/*bInterfaceProtocol*/
		USB_INTERFACE1_C1_STR_INDEX					/*iInterface*/
	},
	//endpoints
	{
		{
			USB_ENDPOINT_DESCRIPTOR_SIZE,				/*bLength*/
			USB_ENDPOINT_DESCRIPTOR_INDEX,			/*bDescriptorType*/
			0x80 | 0x1,										/*bEndpointAddress*/
			0x02,												/*bmAttributes*/
			512,												/*wMaxPacketSize*/
			0													/*bInterval*/
		},
		{
			USB_ENDPOINT_DESCRIPTOR_SIZE,				/*bLength*/
			USB_ENDPOINT_DESCRIPTOR_INDEX,			/*bDescriptorType*/
			0x00 | 0x1,										/*bEndpointAddress*/
			0x02,												/*bmAttributes*/
			512,												/*wMaxPacketSize*/
			0													/*bInterval*/
		}
	}
};

const USB_USER_CONFIGURATION_TYPE USB_FS_USER_CONFIGURATION __attribute__((aligned(4))) = {
	//configuration descriptor
	{
		USB_CONFIGURATION_DESCRIPTOR_SIZE,			/*bLength*/
		USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX,/*bDescriptorType*/
		USB_CONFIGURATION_DESCRIPTOR_SIZE +			/*wTotalLength*/
		USB_INTERFACE_DESCRIPTOR_SIZE +
		USB_ENDPOINT_DESCRIPTOR_SIZE * 2,
		1,														/*bNumInterfaces*/
		1,														/*bConfigurationValue*/
		USB_CONFIGURATION1_STR_INDEX,					/*iConfiguration*/
		0x80,													/*bmAttributes*/
		50														/*bMaxPower*/
	},
	//interface descriptor
	{
		USB_INTERFACE_DESCRIPTOR_SIZE,				/*bLength*/
		USB_INTERFACE_DESCRIPTOR_INDEX,				/*bDescriptorType*/
		0,														/*bInterfaceNumber*/
		0,														/*bAlternateSetting*/
		2,														/*bNumEndpoints*/
		0x08,													/*bInterfaceClass*/
		0x06,													/*bInterfaceSubClass*/
		0x50,													/*bInterfaceProtocol*/
		USB_INTERFACE1_C1_STR_INDEX					/*iInterface*/
	},
	//endpoints
	{
		{
			USB_ENDPOINT_DESCRIPTOR_SIZE,				/*bLength*/
			USB_ENDPOINT_DESCRIPTOR_INDEX,			/*bDescriptorType*/
			0x80 | 0x1,										/*bEndpointAddress*/
			0x02,												/*bmAttributes*/
			64,												/*wMaxPacketSize*/
			0													/*bInterval*/
		},
		{
			USB_ENDPOINT_DESCRIPTOR_SIZE,				/*bLength*/
			USB_ENDPOINT_DESCRIPTOR_INDEX,			/*bDescriptorType*/
			0x00 | 0x1,										/*bEndpointAddress*/
			0x02,												/*bmAttributes*/
			64,												/*wMaxPacketSize*/
			0													/*bInterval*/
		}
	}
};

const USB_MAIN_STRING_DESCRIPTOR_TYPE USB_MAIN_STRING_DESCRIPTOR __attribute__((aligned(4))) = {
	2 + USB_STRING_DESCRIPTORS_LANGUAGES_COUNT * 2,/*bLength*/
	USB_STRING_DESCRIPTOR_INDEX,						/*bDescriptorType*/
	{0x0409}													/*English*/
};

const uint8_t MANUFACTURER_STRING[] = {
	2 + 7 * 2,												/*bLength*/
	USB_STRING_DESCRIPTOR_INDEX,						/*bDescriptorType*/
	'M', 0,
	'K', 0,
	'e', 0,
	'r', 0,
	'n', 0,
	'e', 0,
	'l', 0,
};

const uint8_t PRODUCT_STRING[] = {
	2 + 9 * 2,												/*bLength*/
	USB_STRING_DESCRIPTOR_INDEX,						/*bDescriptorType*/
	'U', 0,
	'S', 0,
	'B', 0,
	' ', 0,
	'F', 0,
	'l', 0,
	'a', 0,
	's', 0,
	'h', 0,
};

const uint8_t DEFAULT_STRING[] = {
	2 + 7 * 2,												/*bLength*/
	USB_STRING_DESCRIPTOR_INDEX,						/*bDescriptorType*/
	'D', 0,
	'e', 0,
	'f', 0,
	'a', 0,
	'u', 0,
	'l', 0,
	't', 0
};

//TODO: generate from device id
const uint8_t SERIAL_STRING[] = {
	2 + 12 * 2,												/*bLength*/
	USB_STRING_DESCRIPTOR_INDEX,						/*bDescriptorType*/
	'0', 0,
	'1', 0,
	'2', 0,
	'3', 0,
	'4', 0,
	'5', 0,
	'6', 0,
	'7', 0,
	'8', 0,
	'9', 0,
	'A', 0,
	'B', 0
};

#pragma pack(pop)

//array of configurations
const P_USB_CONFIGURATION_DESCRIPTOR_TYPE		USB_CONFIGURATION_DESCRIPTORS[] = {
	(P_USB_CONFIGURATION_DESCRIPTOR_TYPE)&USB_USER_CONFIGURATION
};

//array of fs configurations
const P_USB_CONFIGURATION_DESCRIPTOR_TYPE		USB_FS_CONFIGURATION_DESCRIPTORS[] = {
	(P_USB_CONFIGURATION_DESCRIPTOR_TYPE)&USB_FS_USER_CONFIGURATION
};

//array of strings for language 0 (0x0409 - English)
const P_USB_STRING_DESCRIPTOR_TYPE				USB_LANG0_STRING_DESCRIPTORS[] = {
	//Manufacturer
	(P_USB_STRING_DESCRIPTOR_TYPE)&MANUFACTURER_STRING,
	//Product
	(P_USB_STRING_DESCRIPTOR_TYPE)&PRODUCT_STRING,
	//Configuration 1
	(P_USB_STRING_DESCRIPTOR_TYPE)&DEFAULT_STRING,
	//Interface 1 of configuration 1
	(P_USB_STRING_DESCRIPTOR_TYPE)&DEFAULT_STRING,
	//Serial No
	(P_USB_STRING_DESCRIPTOR_TYPE)&SERIAL_STRING,
};

//strings language pack
const PP_USB_STRING_DESCRIPTOR_TYPE				USB_ALL_STRING_DESCRIPTORS = {
	//Lang 0 (0x0409)
	(PP_USB_STRING_DESCRIPTOR_TYPE)&USB_LANG0_STRING_DESCRIPTORS
};

//all together
const USB_DESCRIPTORS_TYPE	USB_DESCRIPTORS = {
	//pointer to DEVICE descriptor
	(P_USB_DEVICE_DESCRIPTOR_TYPE)&USB_DEVICE_DESCRIPTOR,
	//pointer to USB_QUALIFIER_DESCRIPTOR_TYPE for HS
	(P_USB_QUALIFIER_DESCRIPTOR_TYPE)&USB_QUALIFIER_DESCRIPTOR,
	//pointer to USB_QUALIFIER_DESCRIPTOR_TYPE for FS
	(P_USB_QUALIFIER_DESCRIPTOR_TYPE)&USB_QUALIFIER_DESCRIPTOR,
	//number of configurations. At least one.
	1,
	//pointer to array of pointers to configurations. array size == num_of_configurations
	(PP_USB_CONFIGURATION_DESCRIPTOR_TYPE)&USB_CONFIGURATION_DESCRIPTORS,
	//pointer to array of pointers to alternative (FS) configurations. array size == num_of_configurations. May be same as previous
	(PP_USB_CONFIGURATION_DESCRIPTOR_TYPE)&USB_FS_CONFIGURATION_DESCRIPTORS,
	//number of languages in device descriptors. At least one.
	USB_STRING_DESCRIPTORS_LANGUAGES_COUNT,
	//number of strings in each language
	USB_STRING_DESCRIPTORS_STRINGS_COUNT,
	//pointer to STRING descriptor 0 (language string descriptor)
	(P_USB_STRING_DESCRIPTOR_TYPE)&USB_MAIN_STRING_DESCRIPTOR,
	//pointer to array of pointers to language array pointer of strings
	//P_USB_STRING_DESCRIPTOR_TYPE** pp_strings;
	(PPP_USB_STRING_DESCRIPTOR_TYPE)&USB_ALL_STRING_DESCRIPTORS
};

const P_USB_DESCRIPTORS_TYPE _p_usb_descriptors = (P_USB_DESCRIPTORS_TYPE)&USB_DESCRIPTORS;
