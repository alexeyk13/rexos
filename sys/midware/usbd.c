/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "usbd.h"
#include "../../userspace/sys.h"
#include "../../userspace/usb.h"
#include "../../userspace/stdio.h"
#include "../../userspace/block.h"
#include "../../userspace/direct.h"
#include "../../userspace/stdlib.h"
#include "sys_config.h"
#include <string.h>
#include "../../userspace/array.h"
#include "../../userspace/file.h"
#if (USBD_CDC_CLASS)
#include "cdc.h"
#endif //USBD_CDC_CLASS

typedef enum {
    USBD_STATE_DEFAULT = 0,
    USBD_STATE_ADDRESSED,
    USBD_STATE_CONFIGURED
} USBD_STATE;

typedef enum {
    USB_SETUP_STATE_REQUEST = 0,
    USB_SETUP_STATE_DATA_IN,
    //in case response is less, than request
    USB_SETUP_STATE_DATA_IN_ZLP,
    USB_SETUP_STATE_DATA_OUT,
    USB_SETUP_STATE_STATUS_IN,
    USB_SETUP_STATE_STATUS_OUT
} USB_SETUP_STATE;

typedef struct {
    uint16_t index, lang;
    USB_STRING_DESCRIPTOR_TYPE* string;
} USBD_STRING;

typedef struct _USBD {
    HANDLE usb, block;
    //SETUP state machine
    SETUP setup;
    USB_SETUP_STATE setup_state;
    // USBD state machine
    USBD_STATE state;
    bool suspended;
    bool self_powered, remote_wakeup;
#if (USB_TEST_MODE)
    USB_TEST_MODES test_mode;
#endif //USB_TEST_MODE
    USB_SPEED speed;
    uint8_t ep0_size;
    uint8_t configuration, iface, iface_alt;
    //descriptors
    USB_DEVICE_DESCRIPTOR_TYPE *dev_descriptor_fs, *dev_descriptor_hs;
    ARRAY *conf_descriptors_fs, *conf_descriptors_hs;
    ARRAY *string_descriptors;
    ARRAY *ifaces;
    uint8_t ifacecnt;
} USBD;

typedef struct {
    const USBD_CLASS* usbd_class;
    void* param;
} USBD_CLASS_ENTRY;

#define IFACE(usbd, iface)                          (((USBD_CLASS_ENTRY*)((usbd)->ifaces->data))[(iface)])


#if (SYS_INFO)
const char* const USBD_TEXT_STATES[] =              {"Default", "Addressed", "Configured"};
#endif

static const USBD_CLASS* __USBD_CLASSES[] =         {
#if (USBD_CDC_CLASS)
                                                        &__CDC_CLASS,
#endif //USBD_CDC_CLASS
                                                        (const USBD_CLASS*)NULL
                                                    };

void usbd();

const REX __USBD = {
    //name
    "USB device stack",
    //size
    USBD_PROCESS_SIZE,
    //priority - midware priority
    150,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    10,
    //function
    usbd
};

void usbd_class_reset(USBD* usbd)
{
    int i;
    for (i = 0; i < usbd->ifacecnt; ++i)
    {
        if (IFACE(usbd, i).usbd_class != NULL)
            IFACE(usbd, i).usbd_class->usbd_class_reset(usbd, IFACE(usbd, i).param);
    }
    array_clear(&usbd->ifaces);
    //TODO: inform vendor
    //TODO: inform handlers
}

static inline void usbd_class_suspend(USBD* usbd)
{
    int i;
    for (i = 0; i < usbd->ifacecnt; ++i)
    {
        if (IFACE(usbd, i).usbd_class != NULL)
            IFACE(usbd, i).usbd_class->usbd_class_suspend(usbd, IFACE(usbd, i).param);
    }
    //TODO: inform vendor
    //TODO: inform handlers
}

void usbd_class_resume(USBD* usbd)
{
    int i;
    for (i = 0; i < usbd->ifacecnt; ++i)
    {
        if (IFACE(usbd, i).usbd_class != NULL)
            IFACE(usbd, i).usbd_class->usbd_class_resume(usbd, IFACE(usbd, i).param);
    }
    //TODO: inform vendor
    //TODO: inform handlers
}

void usbd_fatal(USBD* usbd)
{
    if (usbd->state == USBD_STATE_CONFIGURED)
    {
        usbd->state = USBD_STATE_DEFAULT;
        usbd_class_reset(usbd);
    }
    ack(usbd->usb, USB_EP_SET_STALL, HAL_HANDLE(HAL_USB, 0), 0, 0);
    ack(usbd->usb, USB_EP_SET_STALL, HAL_HANDLE(HAL_USB, USB_EP_IN | 0), 0, 0);
}

void usbd_fatal_stub(USBD* usbd)
{
#if (USB_DEBUG_ERRORS)
    printf("USBD fatal: stub called\n\r");
#endif
    usbd_fatal(usbd);
}

static inline void usbd_class_configured(USBD* usbd)
{
    int i;
    ARRAY* cfg_array = (usbd->speed >= USB_HIGH_SPEED) ? usbd->conf_descriptors_hs : usbd->conf_descriptors_fs;
    USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg = NULL;
    USB_INTERFACE_DESCRIPTOR_TYPE* iface;
    //find configuration
    for (i = 0; i < cfg_array->size; ++i)
        if (((USB_CONFIGURATION_DESCRIPTOR_TYPE*)(void_array_data(cfg_array)[i]))->bConfigurationValue == usbd->configuration)
        {
            cfg = (USB_CONFIGURATION_DESCRIPTOR_TYPE*)(void_array_data(cfg_array)[i]);
            break;
        }
    if (cfg == NULL)
    {
#if (USB_DEBUG_ERRORS)
        printf("USBD fatal: No selected configuration (%d) found\n\r", usbd->configuration);
#endif
        usbd_fatal(usbd);
        return;
    }
    //find num of interfaces in configuration
    for (iface = usb_get_first_interface(cfg), usbd->ifacecnt = 0; iface != NULL; iface = usb_get_next_interface(cfg, iface))
        if (iface->bInterfaceNumber >= usbd->ifacecnt)
            usbd->ifacecnt = iface->bInterfaceNumber + 1;
    if (array_add(&usbd->ifaces, usbd->ifacecnt * sizeof(USBD_CLASS_ENTRY)) == NULL)
    {
#if (USB_DEBUG_ERRORS)
        printf("USBD fatal: Out of memory\n\r");
#endif
        process_exit();
        return;
    }
    for (iface = usb_get_first_interface(cfg), i = 0; iface != NULL; iface = usb_get_next_interface(cfg, iface), ++i)
        IFACE(usbd, i).usbd_class = NULL;

    //check all classes for interface
    for (i = 0; __USBD_CLASSES[i] != NULL; ++i)
        __USBD_CLASSES[i]->usbd_class_configured(usbd, cfg);

    //TODO: inform vendor
    //TODO: inform handlers
}

static inline void usbd_open(USBD* usbd)
{
    usbd->block = block_create(USBD_BLOCK_SIZE);
    if ((usbd->block = block_create(USBD_BLOCK_SIZE)) == INVALID_HANDLE)
        return;
    fopen(usbd->usb, HAL_HANDLE(HAL_USB, USB_HANDLE_DEVICE), 0);
}

static inline void usbd_close(USBD* usbd)
{
    if (usbd->state == USBD_STATE_CONFIGURED)
        usbd_class_reset(usbd);
    if (usbd->ep0_size)
    {
        fclose(usbd->usb, HAL_HANDLE(HAL_USB, 0));
        fclose(usbd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | 0));
        usbd->ep0_size = 0;
    }

    block_destroy(usbd->block);
    usbd->block = INVALID_HANDLE;
}

static inline void usbd_register_object(ARRAY** ar, HANDLE handle)
{
    int i;
    for (i = 0; i < void_array_size(*ar); ++i)
        if ((HANDLE)(void_array_data(*ar)[i]) == handle)
        {
            error(ERROR_ALREADY_CONFIGURED);
            return;
        }
    if (void_array_add(ar, 1))
        void_array_data(*ar)[void_array_size(*ar) - 1] = (void*)handle;
}

static inline void usbd_unregister_object(ARRAY** ar, HANDLE handle)
{
    int i;
    for (i = 0; i < void_array_size(*ar); ++i)
        if ((HANDLE)(void_array_data(*ar)[i]) == handle)
        {
            void_array_remove(ar, i, 1);
            return;
        }
    error(ERROR_NOT_CONFIGURED);
}

static inline bool usbd_register_device(USB_DEVICE_DESCRIPTOR_TYPE** descriptor, void* ptr)
{
    if ((*descriptor) != NULL)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return false;
    }
    (*descriptor) = ptr;
    return true;
}

static inline bool usbd_register_configuration(ARRAY** ar, int index, void* ptr)
{
    if (index < void_array_size(*ar))
    {
        if (void_array_data(*ar)[index] != NULL)
        {
            error(ERROR_ALREADY_CONFIGURED);
            return false;
        }
    }
    else
    {
        if (void_array_add(ar, index + 1 - void_array_size(*ar)) == NULL)
            return false;
    }
    void_array_data(*ar)[index] = ptr;
    return true;
}

static inline bool usbd_register_string(USBD* usbd, unsigned int index, unsigned int lang, void* ptr)
{
    int i;
    USBD_STRING* item;
    for (i = 0; i < void_array_size(usbd->string_descriptors); ++i)
    {
        item = (USBD_STRING*)(void_array_data(usbd->string_descriptors)[i]);
        if (item->index == index && item->lang == lang)
        {
            error(ERROR_ALREADY_CONFIGURED);
            return false;
        }
    }
    item = (USBD_STRING*)malloc(sizeof(USBD_STRING));
    if (item == NULL)
        return false;
    void_array_add(&usbd->string_descriptors, 1);
    void_array_data(usbd->string_descriptors)[void_array_size(usbd->string_descriptors) - 1] = item;
    item->index = index;
    item->lang = lang;
    item->string = ptr;
    return true;
}

static inline void free_if_own(void* ptr)
{
    if ((unsigned int)ptr >= (unsigned int)(__GLOBAL->heap) && (unsigned int)ptr < (unsigned int)(__GLOBAL->heap) + USBD_PROCESS_SIZE)
        free(ptr);
}

static inline void usbd_unregister_device(USB_DEVICE_DESCRIPTOR_TYPE** descriptor)
{
    if ((*descriptor) == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    free_if_own(*descriptor);
    (*descriptor) = NULL;
}

static inline void usbd_unregister_configuration(ARRAY** ar, int index)
{
    if (index >= void_array_size(*ar) || void_array_data(*ar)[index] == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    //free non-persistent
    free_if_own(void_array_data(*ar)[index]);
    void_array_data(*ar)[index] = NULL;
    if (void_array_size(*ar) == index + 1)
        void_array_remove(ar, index, 1);
}

static inline void usbd_unregister_string(USBD* usbd, unsigned int index, unsigned int lang)
{
    int i;
    USBD_STRING* item;
    for (i = 0; i < void_array_size(usbd->string_descriptors); ++i)
    {
        item = (USBD_STRING*)(void_array_data(usbd->string_descriptors)[i]);
        if (item->index == index && item->lang == lang)
        {
            //free non-persistent
            free_if_own(item->string);
            void_array_remove(&usbd->string_descriptors, i, 1);
        }
    }
    error(ERROR_NOT_CONFIGURED);
}

void usbd_register_descriptor(USBD* usbd, USBD_DESCRIPTOR_TYPE type, unsigned int size, HANDLE process)
{
    char buf[size];
    USBD_DESCRIPTOR_REGISTER_STRUCT* udrs;
    void* ptr;
    if (!direct_read(process, buf, size))
        return;
    udrs = (USBD_DESCRIPTOR_REGISTER_STRUCT*)buf;
    if (udrs->flags & USBD_FLAG_PERSISTENT_DESCRIPTOR)
        ptr = *(char**)(buf + USBD_DESCRIPTOR_REGISTER_STRUCT_SIZE_ALIGNED);
    else
    {
        ptr = malloc(size - USBD_DESCRIPTOR_REGISTER_STRUCT_SIZE_ALIGNED);
        if (ptr == NULL)
            return;
        memcpy(ptr, (void*)(buf + USBD_DESCRIPTOR_REGISTER_STRUCT_SIZE_ALIGNED), size - USBD_DESCRIPTOR_REGISTER_STRUCT_SIZE_ALIGNED);
    }
    bool res = false;
    switch (type)
    {
    case USB_DESCRIPTOR_DEVICE_FS:
        res = usbd_register_device(&usbd->dev_descriptor_fs, ptr);
        break;
    case USB_DESCRIPTOR_DEVICE_HS:
        res = usbd_register_device(&usbd->dev_descriptor_hs, ptr);
        break;
    case USB_DESCRIPTOR_CONFIGURATION_FS:
        res = usbd_register_configuration(&usbd->conf_descriptors_fs, udrs->index, ptr);
        break;
    case USB_DESCRIPTOR_CONFIGURATION_HS:
        res = usbd_register_configuration(&usbd->conf_descriptors_hs, udrs->index, ptr);
        break;
    case USB_DESCRIPTOR_STRING:
        res = usbd_register_string(usbd, udrs->index, udrs->lang, ptr);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
    if (!res && (udrs->flags & USBD_FLAG_PERSISTENT_DESCRIPTOR) == 0)
        free(ptr);
}

void usbd_unregister_descriptor(USBD* usbd, USBD_DESCRIPTOR_TYPE type, unsigned int index, unsigned int lang)
{
    switch (type)
    {
    case USB_DESCRIPTOR_DEVICE_FS:
        usbd_unregister_device(&usbd->dev_descriptor_fs);
        break;
    case USB_DESCRIPTOR_DEVICE_HS:
        usbd_unregister_device(&usbd->dev_descriptor_hs);
        break;
    case USB_DESCRIPTOR_CONFIGURATION_FS:
        usbd_unregister_configuration(&usbd->conf_descriptors_fs, index);
        break;
    case USB_DESCRIPTOR_CONFIGURATION_HS:
        usbd_unregister_configuration(&usbd->conf_descriptors_hs, index);
        break;
    case USB_DESCRIPTOR_STRING:
        usbd_unregister_string(usbd, index, lang);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

static inline void usbd_reset(USBD* usbd, USB_SPEED speed)
{
    usbd->suspended = false;
#if (USB_DEBUG_REQUESTS)
    printf("USB device reset\n\r");
#endif
    if (usbd->ep0_size)
    {
        fclose(usbd->usb, HAL_HANDLE(HAL_USB, 0));
        fclose(usbd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | 0));
    }
    usbd->speed = speed;
    usbd->ep0_size = usbd->speed == USB_LOW_SPEED ? 8 : 64;

    unsigned int size = usbd->ep0_size;
    fopen_p(usbd->usb, HAL_HANDLE(HAL_USB, 0), USB_EP_CONTROL, (void*)size);
    fopen_p(usbd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | 0), USB_EP_CONTROL, (void*)size);

    if (usbd->state == USBD_STATE_CONFIGURED)
    {
        usbd->state = USBD_STATE_DEFAULT;
        usbd_class_reset(usbd);
    }
}

static inline void usbd_suspend(USBD* usbd)
{
    if (!usbd->suspended)
    {
        usbd->suspended = true;
#if (USB_DEBUG_REQUESTS)
        printf("USB device suspend\n\r");
#endif
        if (usbd->ep0_size)
        {
            fflush(usbd->usb, HAL_HANDLE(HAL_USB, 0));
            fflush(usbd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | 0));
        }
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
        if (usbd->state == USBD_STATE_CONFIGURED)
            usbd_class_suspend(usbd);
    }
}

static inline void usbd_wakeup(USBD* usbd)
{
    if (usbd->suspended)
    {
        usbd->suspended = false;
#if (USB_DEBUG_REQUESTS)
        printf("USB device wakeup\n\r");
#endif

        if (usbd->state == USBD_STATE_CONFIGURED)
            usbd_class_resume(usbd);
    }
}

static inline int safecpy_write(USBD* usbd, void* src, int size)
{
    void* ptr = block_open(usbd->block);
    if (size > USBD_BLOCK_SIZE)
        size = USBD_BLOCK_SIZE;
    if (ptr != NULL)
    {
        memcpy(ptr, src, size);
        return size;
    }
    else
        return -1;
}

int send_descriptor(USBD* usbd, void* descriptor, uint8_t type, int size)
{
    USB_DESCRIPTOR_TYPE* dst;
    if (descriptor == NULL)
    {
#if (USB_DEBUG_ERRORS)
        printf("USB descriptor type %d not present\n\r", type);
#endif
        return -1;
    }
    dst = block_open(usbd->block);
    if (dst == NULL)
        return -1;
    if (size > USBD_BLOCK_SIZE)
        size = USBD_BLOCK_SIZE;
    memcpy(dst, descriptor, size);
    dst->bDescriptorType = type;
    return size;
}

int send_configuration_descriptor(USBD* usbd, ARRAY** ar, int index, uint8_t type)
{
    USB_CONFIGURATION_DESCRIPTOR_TYPE* dst;
    if (index >= (*ar)->size)
    {
#if (USB_DEBUG_ERRORS)
        printf("USB CONFIGURATION %d descriptor not present\n\r", index);
#endif
        error(ERROR_NOT_CONFIGURED);
        return -1;
    }
    dst = (USB_CONFIGURATION_DESCRIPTOR_TYPE*)(void_array_data(*ar)[index]);
    return send_descriptor(usbd, dst, type, dst->wTotalLength);
}

//static inline int send_strings_descriptor(USBD* usbd, int index, int lang_id)
int send_strings_descriptor(USBD* usbd, int index, int lang_id)
{
    int i;
    USBD_STRING* item;
    for (i = 0; i < usbd->string_descriptors->size; ++i)
    {
        item = (USBD_STRING*)(void_array_data(usbd->string_descriptors)[i]);
        if (item->index == index && item->lang == lang_id)
            return send_descriptor(usbd, item->string, USB_STRING_DESCRIPTOR_INDEX, item->string->bLength);
    }
#if (USB_DEBUG_ERRORS)
    printf("USB: STRING descriptor %d, lang_id: %#X not present\n\r", index, lang_id);
#endif
    return -1;
}

static inline int usbd_device_get_status(USBD* usbd)
{
#if (USB_DEBUG_REQUESTS)
    printf("USB: get device status\n\r");
#endif
    uint16_t status = 0;
    if (usbd->self_powered)
        status |= 1 << 0;
    if (usbd->remote_wakeup)
        status |= 1 << 1;
    return safecpy_write(usbd, &status, sizeof(uint16_t));
}

#if (USB_TEST_MODE)
static inline int usbd_device_set_feature(USBD* usbd)
{
    unsigned int res = -1;
#if (USB_DEBUG_REQUESTS)
    printf("USB: device set feature\n\r");
#endif
    //According to documentation the only feature can be set is TEST_MODE
    switch (usbd->setup.wValue)
    {
    case USBD_FEATURE_TEST_MODE:
        usbd->test_mode = usbd->setup.wIndex >> 16;
        ack(usbd->usb, USB_SET_TEST_MODE, usbd->test_mode, 0, 0);
        res = 0;
        break;
    default:
        break;
    }
    return res;
}
#endif //USB_TEST_MODE

static inline int usbd_device_clear_feature(USBD* usbd)
{
    unsigned int res = -1;
#if (USB_DEBUG_REQUESTS)
    printf("USB: device clear feature\n\r");
#endif
    switch (usbd->setup.wValue)
    {
    case USBD_FEATURE_DEVICE_REMOTE_WAKEUP:
        usbd->remote_wakeup = false;
        res = 0;
        break;
    default:
        break;
    }
    return res;
}

static inline int usbd_set_address(USBD* usbd)
{
#if (USB_DEBUG_REQUESTS)
    printf("USB set ADDRESS %#X\n\r", usbd->setup.wValue);
#endif
    ack(usbd->usb, USB_SET_ADDRESS, usbd->setup.wValue, 0, 0);
    switch (usbd->state)
    {
    case USBD_STATE_DEFAULT:
        usbd->state = USBD_STATE_ADDRESSED;
        break;
    case USBD_STATE_ADDRESSED:
        if (usbd->setup.wValue == 0)
            usbd->state = USBD_STATE_DEFAULT;
        break;
    default:
        break;
    }
    return 0;
}

static inline int usbd_get_descriptor(USBD* usbd)
{
    //can be called in any device state
    int res = -1;

    int index = usbd->setup.wValue & 0xff;
    switch (usbd->setup.wValue >> 8)
    {
    case USB_DEVICE_DESCRIPTOR_INDEX:
#if (USB_DEBUG_REQUESTS)
        printf("USB get DEVICE descriptor\n\r");
#endif
        if (usbd->speed >= USB_HIGH_SPEED)
            res = send_descriptor(usbd, usbd->dev_descriptor_hs, USB_DEVICE_DESCRIPTOR_INDEX, usbd->dev_descriptor_hs->bLength);
        else
            res = send_descriptor(usbd, usbd->dev_descriptor_fs, USB_DEVICE_DESCRIPTOR_INDEX, usbd->dev_descriptor_fs->bLength);
        break;
    case USB_CONFIGURATION_DESCRIPTOR_INDEX:
#if (USB_DEBUG_REQUESTS)
        printf("USB get CONFIGURATION %d descriptor\n\r", index);
#endif
        if (usbd->speed >= USB_HIGH_SPEED)
            res = send_configuration_descriptor(usbd, &usbd->conf_descriptors_hs, index, USB_CONFIGURATION_DESCRIPTOR_INDEX);
        else
            res = send_configuration_descriptor(usbd, &usbd->conf_descriptors_fs, index, USB_CONFIGURATION_DESCRIPTOR_INDEX);
        break;
    case USB_STRING_DESCRIPTOR_INDEX:
#if (USB_DEBUG_REQUESTS)
        printf("USB get STRING %d descriptor, LangID: %#X\n\r", index, usbd->setup.wIndex);
#endif
        res = send_strings_descriptor(usbd, index, usbd->setup.wIndex);
        break;
    case USB_DEVICE_QUALIFIER_DESCRIPTOR_INDEX:
#if (USB_DEBUG_REQUESTS)
        printf("USB get DEVICE qualifier descriptor\n\r");
#endif
        if (usbd->speed >= USB_HIGH_SPEED)
            res = send_descriptor(usbd, usbd->dev_descriptor_fs, USB_DEVICE_QUALIFIER_DESCRIPTOR_INDEX, usbd->dev_descriptor_fs->bLength);
        else
            res = send_descriptor(usbd, usbd->dev_descriptor_hs, USB_DEVICE_QUALIFIER_DESCRIPTOR_INDEX, usbd->dev_descriptor_hs->bLength);
        break;
    case USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX:
#if (USB_DEBUG_REQUESTS)
        printf("USB get other speed CONFIGURATION %d descriptor\n\r", index);
#endif
        if (usbd->speed >= USB_HIGH_SPEED)
            res = send_configuration_descriptor(usbd, &usbd->conf_descriptors_fs, index, USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX);
        else
            res = send_configuration_descriptor(usbd, &usbd->conf_descriptors_hs, index, USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX);
        break;
    }

    return res;
}

static inline int usbd_get_configuration(USBD* usbd)
{
#if (USB_DEBUG_REQUESTS)
    printf("USB: get configuration\n\r");
#endif
    char configuration = (char)usbd->configuration;
    return safecpy_write(usbd, &configuration, 1);
}

static inline int usbd_set_configuration(USBD* usbd)
{
#if (USB_DEBUG_REQUESTS)
    printf("USB: set configuration %d\n\r", usbd->setup.wValue);
#endif
    //read USB 2.0 specification for more details
    if (usbd->state == USBD_STATE_CONFIGURED)
    {
        usbd->configuration = 0;
        usbd->iface = 0;
        usbd->iface_alt = 0;

        usbd->state = USBD_STATE_ADDRESSED;
        usbd_class_reset(usbd);
    }
    else if (usbd->state == USBD_STATE_ADDRESSED && usbd->setup.wValue)
    {
        usbd->configuration = usbd->setup.wValue;
        usbd->iface = 0;
        usbd->iface_alt = 0;
        usbd->state = USBD_STATE_CONFIGURED;

        usbd_class_configured(usbd);
    }
    return 0;
}

static inline int usbd_device_request(USBD* usbd)
{
    int res = -1;
    switch (usbd->setup.bRequest)
    {
    case USB_REQUEST_GET_STATUS:
        res = usbd_device_get_status(usbd);
        break;
#if (USB_TEST_MODE)
    case USB_REQUEST_SET_FEATURE:
        res = usbd_device_set_feature(usbd);
        break;
#endif
    case USB_REQUEST_CLEAR_FEATURE:
        res = usbd_device_clear_feature(usbd);
        break;
    case USB_REQUEST_SET_ADDRESS:
        res = usbd_set_address(usbd);
        break;
    case USB_REQUEST_GET_DESCRIPTOR:
        res = usbd_get_descriptor(usbd);
        break;
    case USB_REQUEST_GET_CONFIGURATION:
        res = usbd_get_configuration(usbd);
        break;
    case USB_REQUEST_SET_CONFIGURATION:
        res = usbd_set_configuration(usbd);
        break;
    }

    return res;
}

static inline int usbd_interface_get_status(USBD* usbd)
{
#if (USB_DEBUG_REQUESTS)
    printf("USB: get interface status\n\r");
#endif
    uint16_t status = 0;
    return safecpy_write(usbd, &status, sizeof(uint16_t));
}

static inline int usbd_set_interface(USBD* usbd)
{
#if (USB_DEBUG_REQUESTS)
    printf("USB: interface set\n\r");
#endif
    usbd->iface = usbd->setup.wIndex;
    usbd->iface_alt = usbd->setup.wValue;
    return 0;
}

static inline int usbd_get_interface(USBD* usbd)
{
#if (USB_DEBUG_REQUESTS)
    printf("USB: interface get\n\r");
#endif
    uint8_t alt = usbd->iface_alt;
    return safecpy_write(usbd, &alt, 1);
}

static inline int usbd_interface_request(USBD* usbd)
{
    int res = -1;
    switch (usbd->setup.bRequest)
    {
    case USB_REQUEST_GET_STATUS:
        res = usbd_interface_get_status(usbd);
        break;
    case USB_REQUEST_SET_INTERFACE:
        res = usbd_set_interface(usbd);
        break;
    case USB_REQUEST_GET_INTERFACE:
        res = usbd_get_interface(usbd);
        break;
    }
    return res;
}

static inline int usbd_endpoint_get_status(USBD* usbd)
{
#if (USB_DEBUG_REQUESTS)
    printf("USB: get endpoint status\n\r");
#endif
    uint16_t status = 0;
    if (get(usbd->usb, USB_EP_IS_STALL, HAL_HANDLE(HAL_USB, usbd->setup.wIndex & 0xffff), 0, 0))
        status |= 1 << 0;
    return safecpy_write(usbd, &status, sizeof(uint16_t));
}

static inline int usbd_endpoint_set_feature(USBD* usbd)
{
    unsigned int res = -1;
#if (USB_DEBUG_REQUESTS)
    printf("USB: endpoint set feature\n\r");
#endif
    switch (usbd->setup.wValue)
    {
    case USBD_FEATURE_ENDPOINT_HALT:
        ack(usbd->usb, USB_EP_SET_STALL, HAL_HANDLE(HAL_USB, usbd->setup.wIndex & 0xffff), 0, 0);
        res = 0;
        break;
    default:
        break;
    }
    return res;
}

static inline int usbd_endpoint_clear_feature(USBD* usbd)
{
    unsigned int res = -1;
#if (USB_DEBUG_REQUESTS)
    printf("USB: endpoint clear feature\n\r");
#endif
    switch (usbd->setup.wValue)
    {
    case USBD_FEATURE_ENDPOINT_HALT:
        ack(usbd->usb, USB_EP_CLEAR_STALL, HAL_HANDLE(HAL_USB, usbd->setup.wIndex & 0xffff), 0, 0);
        res = 0;
        break;
    default:
        break;
    }
    return res;
}

static inline int usbd_endpoint_request(USBD* usbd)
{
    int res = -1;
    switch (usbd->setup.bRequest)
    {
    case USB_REQUEST_GET_STATUS:
        res = usbd_endpoint_get_status(usbd);
        break;
    case USB_REQUEST_SET_FEATURE:
        res = usbd_endpoint_set_feature(usbd);
        break;
    case USB_REQUEST_CLEAR_FEATURE:
        res = usbd_endpoint_clear_feature(usbd);
        break;
    }
    return res;
}

static inline int usbd_class_interface_request(USBD* usbd, unsigned int iface)
{
    if (iface >= usbd->ifacecnt || IFACE(usbd, iface).usbd_class == NULL)
    {
#if (USB_DEBUG_ERRORS)
        printf("USBD class error: Interface %d not configured\n\r", iface);
#endif
        return -1;
    }
    return IFACE(usbd, iface).usbd_class->usbd_class_setup(usbd, IFACE(usbd, iface).param, &usbd->setup, usbd->block);
}

void usbd_setup_process(USBD* usbd)
{
    int res = -1;
    switch (usbd->setup.bmRequestType & BM_REQUEST_TYPE)
    {
    case BM_REQUEST_TYPE_STANDART:
        switch (usbd->setup.bmRequestType & BM_REQUEST_RECIPIENT)
        {
        case BM_REQUEST_RECIPIENT_DEVICE:
            res = usbd_device_request(usbd);
            break;
        case BM_REQUEST_RECIPIENT_INTERFACE:
            res = usbd_interface_request(usbd);
            break;
        case BM_REQUEST_RECIPIENT_ENDPOINT:
            res = usbd_endpoint_request(usbd);
            break;
        }
        break;
    case BM_REQUEST_TYPE_CLASS:
        if ((usbd->setup.bmRequestType & BM_REQUEST_RECIPIENT) == BM_REQUEST_RECIPIENT_INTERFACE)
            res = usbd_class_interface_request(usbd, usbd->setup.wIndex);
        break;
    case BM_REQUEST_TYPE_VENDOR:
        //TODO:
        break;
    }

    if (res > usbd->setup.wLength)
        res = usbd->setup.wLength;
    //success. start transfers
    if (res >= 0)
    {
        if ((usbd->setup.bmRequestType & BM_REQUEST_DIRECTION) == BM_REQUEST_DIRECTION_HOST_TO_DEVICE)
        {
            //data already received, sending status
            usbd->setup_state = USB_SETUP_STATE_STATUS_IN;
            fwrite_async_null(usbd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | 0));
        }
        else
        {
            //response less, than required and multiples of EP0SIZE - we need to send ZLP on end of transfers
            if (res < usbd->setup.wLength && ((res % usbd->ep0_size) == 0))
            {
                if (res)
                {
                    usbd->setup_state = USB_SETUP_STATE_DATA_IN_ZLP;
                    fwrite_async(usbd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | 0), usbd->block, res);
                }
                //if no data at all, but request success, we will send ZLP right now
                else
                {
                    usbd->setup_state = USB_SETUP_STATE_DATA_IN;
                    fwrite_async_null(usbd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | 0));
                }
            }
            else if (res)
            {
                usbd->setup_state = USB_SETUP_STATE_DATA_IN;
                fwrite_async(usbd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | 0), usbd->block, res);
            }
            //data stage is optional
            else
            {
                usbd->setup_state = USB_SETUP_STATE_STATUS_OUT;
                fread_async_null(usbd->usb, HAL_HANDLE(HAL_USB, 0));
            }
        }
    }
    else
    {
        if ((usbd->setup.bmRequestType & BM_REQUEST_RECIPIENT) == BM_REQUEST_RECIPIENT_ENDPOINT)
            ack(usbd->usb, USB_EP_SET_STALL, HAL_HANDLE(HAL_USB, usbd->setup.wIndex), 0, 0);
        else
            usbd_fatal(usbd);
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
#if (USB_DEBUG_ERRORS)
        printf("Unhandled ");
        switch (usbd->setup.bmRequestType & BM_REQUEST_TYPE)
        {
        case BM_REQUEST_TYPE_STANDART:
            printf("STANDART");
            break;
        case BM_REQUEST_TYPE_CLASS:
            printf("CLASS");
            break;
        case BM_REQUEST_TYPE_VENDOR:
            printf("VENDOR");
            break;
        }
        printf(" request\n\r");

        printf("bmRequestType: %X\n\r", usbd->setup.bmRequestType);
        printf("bRequest: %X\n\r", usbd->setup.bRequest);
        printf("wValue: %X\n\r", usbd->setup.wValue);
        printf("wIndex: %X\n\r", usbd->setup.wIndex);
        printf("wLength: %X\n\r", usbd->setup.wLength);
#endif

    }
}

static inline void usbd_setup_received(USBD* usbd)
{
    //Back2Back setup received
    if (usbd->setup_state != USB_SETUP_STATE_REQUEST)
    {
#if (USB_DEBUG_REQUESTS)
        printf("USB B2B SETUP received, state: %d\n\r", usbd->setup_state);
#endif
        //reset control EP if transaction in progress
        switch (usbd->setup_state)
        {
        case USB_SETUP_STATE_DATA_IN:
        case USB_SETUP_STATE_DATA_IN_ZLP:
        case USB_SETUP_STATE_STATUS_IN:
            fflush(usbd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | 0));
            break;
        case USB_SETUP_STATE_DATA_OUT:
        case USB_SETUP_STATE_STATUS_OUT:
            fflush(usbd->usb, HAL_HANDLE(HAL_USB, 0));
            break;
        default:
            break;
        }

        usbd->setup_state = USB_SETUP_STATE_REQUEST;
    }

    //if data from host - read it first before processing
    if ((usbd->setup.bmRequestType & BM_REQUEST_DIRECTION) == BM_REQUEST_DIRECTION_HOST_TO_DEVICE)
    {
        if (usbd->setup.wLength)
        {
            usbd->setup_state = USB_SETUP_STATE_DATA_OUT;
            fread_async(usbd->usb, HAL_HANDLE(HAL_USB, 0), usbd->block, usbd->setup.wLength);
        }
        //data stage is optional
        else
            usbd_setup_process(usbd);
    }
    else
        usbd_setup_process(usbd);
}

void usbd_read_complete(USBD* usbd)
{
    switch (usbd->setup_state)
    {
    case USB_SETUP_STATE_DATA_OUT:
        usbd_setup_process(usbd);
        break;
    case USB_SETUP_STATE_STATUS_OUT:
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
        break;
    default:
#if (USB_DEBUG_ERRORS)
        printf("USBD invalid setup state on read: %d\n\r", usbd->setup_state);
#endif
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
        break;
    }
}

void usbd_class_read_complete(USBD* usbd, IPC* ipc)
{
    //TODO: temporaily solution for future refactoring
    __USBD_CLASSES[0]->usbd_class_request(usbd, ((USBD_CLASS_ENTRY*)(array_data(usbd->ifaces)))[0].param, ipc);
}

void usbd_write_complete(USBD* usbd)
{
    switch (usbd->setup_state)
    {
    case USB_SETUP_STATE_DATA_IN_ZLP:
        //TX ZLP and switch to normal state
        usbd->setup_state = USB_SETUP_STATE_DATA_IN;
        fwrite_async_null(usbd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | 0));
        break;
    case USB_SETUP_STATE_DATA_IN:
        usbd->setup_state = USB_SETUP_STATE_STATUS_OUT;
        fread_async_null(usbd->usb, HAL_HANDLE(HAL_USB, 0));
        break;
    case USB_SETUP_STATE_STATUS_IN:
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
        break;
    default:
#if (USB_DEBUG_ERRORS)
        printf("USBD invalid state on write: %s\n\r", usbd->setup_state);
#endif
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
        break;
    }
}

void usbd_class_write_complete(USBD* usbd, IPC* ipc)
{
    //TODO: temporaily solution for future refactoring
    __USBD_CLASSES[0]->usbd_class_request(usbd, ((USBD_CLASS_ENTRY*)(array_data(usbd->ifaces)))[0].param, ipc);
}


#if (SYS_INFO)
static inline void usbd_info(USBD* usbd)
{
    printf("USB device info\n\r\n\r");
    printf("State: %s\n\r", USBD_TEXT_STATES[usbd->state]);
    if (usbd->state != USBD_STATE_CONFIGURED)
        return;
    printf("Configuration: %d\n\r", usbd->configuration);
}
#endif

static inline void usbd_init(USBD* usbd)
{
    usbd->usb = object_get(SYS_OBJ_USB);
    usbd->setup_state = USB_SETUP_STATE_REQUEST;
    usbd->state = USBD_STATE_DEFAULT;
    usbd->suspended = false;
    usbd->self_powered = usbd->remote_wakeup = false;
#if (USB_TEST_MODE)
    usbd->test_mode = USB_TEST_MODE_NORMAL;
#endif //USB_TEST_MODE

    usbd->speed = USB_LOW_SPEED;
    usbd->ep0_size = 0;
    usbd->configuration = 0;
    usbd->dev_descriptor_fs = usbd->dev_descriptor_hs = NULL;

    void_array_create(&usbd->conf_descriptors_fs, 1);
    void_array_create(&usbd->conf_descriptors_hs, 1);
    //at least 3: manufacturer, product, string 0
    void_array_create(&usbd->string_descriptors, 3);
    array_create(&usbd->ifaces, sizeof(USBD_CLASS_ENTRY));
}

static inline bool usbd_request(USBD* usbd, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
    case USBD_REGISTER_DESCRIPTOR:
        usbd_register_descriptor(usbd, ipc->param1, ipc->param2, ipc->process);
        need_post = true;
        break;
    case USBD_UNREGISTER_DESCRIPTOR:
        usbd_unregister_descriptor(usbd, ipc->param1, ipc->param2, ipc->param3);
        need_post = true;
        break;
    case USB_RESET:
        usbd_reset(usbd, ipc->param1);
        //called from ISR, no response
        break;
    case USB_SUSPEND:
        usbd_suspend(usbd);
        //called from ISR, no response
        break;
    case USB_WAKEUP:
        usbd_wakeup(usbd);
        //called from ISR, no response
        break;
    case USB_SETUP:
        ((uint32_t*)(&usbd->setup))[0] = ipc->param1;
        ((uint32_t*)(&usbd->setup))[1] = ipc->param2;
        usbd_setup_received(usbd);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

void usbd()
{
    USBD usbd;
    IPC ipc;
    bool need_post;

#if (SYS_INFO) || (USB_DEBUG_REQUESTS) || (USB_DEBUG_ERRORS)
    open_stdout();
#endif
    object_set_self(SYS_OBJ_USBD);

    usbd_init(&usbd);
    for (;;)
    {
        ipc_read_ms(&ipc, 0, 0);
        error(ERROR_OK);
        need_post = false;
        switch (ipc.cmd)
        {
        case IPC_PING:
            need_post = true;
            break;
#if (SYS_INFO)
        case IPC_GET_INFO:
            usbd_info(&usbd);
            need_post = true;
            break;
#endif
        case IPC_OPEN:
            usbd_open(&usbd);
            need_post = true;
            break;
        case IPC_CLOSE:
            usbd_close(&usbd);
            need_post = true;
            break;
        case IPC_READ_COMPLETE:
            if (USB_EP_NUM(HAL_ITEM(ipc.param1)))
                usbd_class_read_complete(&usbd, &ipc);
            else
                usbd_read_complete(&usbd);
            break;
        case IPC_WRITE_COMPLETE:
            if (USB_EP_NUM(HAL_ITEM(ipc.param1)))
                usbd_class_write_complete(&usbd, &ipc);
            else
                usbd_write_complete(&usbd);
            break;
        default:
            need_post = usbd_request(&usbd, &ipc);
            break;
        }
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}

bool usbd_register_interface(USBD* usbd, unsigned int iface, const USBD_CLASS* usbd_class, void* param)
{
    if (iface >= usbd->ifacecnt)
        return false;
    if (IFACE(usbd, iface).usbd_class != NULL)
        return false;
    IFACE(usbd, iface).usbd_class = usbd_class;
    IFACE(usbd, iface).param = param;
    return true;
}

bool usbd_unregister_interface(USBD* usbd, unsigned int iface, const USBD_CLASS* usbd_class)
{
    if (iface >= usbd->ifacecnt)
        return false;
    if (IFACE(usbd, iface).usbd_class != usbd_class)
        return false;
    IFACE(usbd, iface).usbd_class = NULL;
    return true;
}
