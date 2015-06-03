/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "usbd.h"
#include "../userspace/sys.h"
#include "../userspace/usb.h"
#include "../userspace/stdio.h"
#include "../userspace/block.h"
#include "../userspace/direct.h"
#include "../userspace/stdlib.h"
#include <string.h>
#include "../userspace/array.h"
#include "../userspace/file.h"
#include "usbdp.h"
#if (USBD_CDC_CLASS)
#include "cdcd.h"
#endif //USBD_CDC_CLASS
#if (USBD_HID_KBD_CLASS)
#include "hidd_kbd.h"
#endif //USBD_HID_KBD_CLASS
#if (USBD_MSC_CLASS)
#include "mscd.h"
#endif //USBD_MSC_CLASS
#if (USBD_CCID_CLASS)
#include "ccidd.h"
#endif //USBD_CCID_CLASS

typedef enum {
    USB_SETUP_STATE_REQUEST = 0,
    USB_SETUP_STATE_VENDOR_REQUEST,
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
    HANDLE usb, block, user;
    //SETUP state machine
    SETUP setup;
    USB_SETUP_STATE setup_state;
    // USBD state machine
    USBD_STATE state;
    bool self_powered, remote_wakeup;
#if (USB_2_0)
    USB_TEST_MODES test_mode;
#endif //USB_2_0
    USB_SPEED speed;
    uint8_t ep0_size;
    uint8_t configuration;
    //descriptors
    USB_DEVICE_DESCRIPTOR_TYPE *dev_descriptor_fs;
#if (USB_2_0)
    USB_DEVICE_DESCRIPTOR_TYPE *dev_descriptor_hs;
#endif //USB_2_0
    ARRAY *conf_descriptors_fs;
#if (USB_2_0)
    ARRAY *conf_descriptors_hs;
#endif //USB_2_0
    ARRAY *string_descriptors;
    ARRAY *ifaces;
    uint8_t ep_iface[USB_EP_COUNT_MAX];
    uint8_t ifacecnt;
} USBD;

typedef struct {
    const USBD_CLASS* usbd_class;
    void* param;
} USBD_IFACE_ENTRY;

#define IFACE(usbd, iface)                          ((USBD_IFACE_ENTRY*)array_at((usbd)->ifaces, iface))
#define STRING(usbd, i)                             ((USBD_STRING*)array_at((usbd)->string_descriptors, i))

#define USBD_INVALID_INTERFACE                      0xff

void usbd_stub_class_state_change(USBD* usbd, void* param);
int usbd_stub_class_setup(USBD* usbd, void* param, SETUP* setup, HANDLE block);
bool usbd_stub_class_request(USBD* usbd, void* param, IPC* ipc);

const USBD_CLASS __USBD_STUB_CLASS = {
    NULL,
    usbd_stub_class_state_change,
    usbd_stub_class_state_change,
    usbd_stub_class_state_change,
    usbd_stub_class_setup,
    usbd_stub_class_request,
};

static const USBD_CLASS* __USBD_CLASSES[] =         {
#if (USBD_CDC_CLASS)
                                                        &__CDCD_CLASS,
#endif //USBD_CDC_CLASS
#if (USBD_HID_KBD_CLASS)
                                                        &__HIDD_KBD_CLASS,
#endif //USBD_HID_KBD_CLASS
#if (USBD_MSC_CLASS)
                                                        &__MSCD_CLASS,
#endif //USBD_MSC_CLASS
#if (USBD_CCID_CLASS)
                                                        &__CCIDD_CLASS,
#endif //USBD_CCID_CLASS
                                                        &__USBD_STUB_CLASS
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
    USBD_IPC_COUNT,
    //function
    usbd
};

void usbd_stub_class_state_change(USBD* usbd, void* param)
{
    //state changing request stub calling is not error in case of some interfaces are not used
    //Moreover, it's normal condition to avoid MS Windows bug for composite devices, where interfaces are starting from 1
}

int usbd_stub_class_setup(USBD* usbd, void* param, SETUP* setup, HANDLE block)
{
#if (USBD_DEBUG_ERRORS)
    printf("USBD class SETUP stub!\n\r");
#endif
    return -1;
}

bool usbd_stub_class_request(USBD* usbd, void* param, IPC* ipc)
{
#if (USBD_DEBUG_ERRORS)
    printf("USBD class request stub!\n\r");
#endif
    //less chance to halt app
    return true;
}

void usbd_inform(USBD* usbd, unsigned int alert, bool need_wait)
{
    if (usbd->user != INVALID_HANDLE)
    {
        if (need_wait)
            ack(usbd->user, HAL_CMD(HAL_USBD, USBD_ALERT), alert, 0, 0);
        else
            ipc_post_inline(usbd->user, HAL_CMD(HAL_USBD, USBD_ALERT), alert, 0, 0);
    }
}

void usbd_class_reset(USBD* usbd)
{
    int i;
    for (i = 0; i < usbd->ifacecnt; ++i)
        IFACE(usbd, i)->usbd_class->usbd_class_reset(usbd, IFACE(usbd, i)->param);
    usbd->ifacecnt = 0;
    array_clear(&usbd->ifaces);
}

static inline void usbd_class_suspend(USBD* usbd)
{
    int i;
    usbd_inform(usbd, USBD_ALERT_SUSPEND, true);
    for (i = 0; i < usbd->ifacecnt; ++i)
        IFACE(usbd, i)->usbd_class->usbd_class_suspend(usbd, IFACE(usbd, i)->param);
}

void usbd_class_resume(USBD* usbd)
{
    int i;
    for (i = 0; i < usbd->ifacecnt; ++i)
        IFACE(usbd, i)->usbd_class->usbd_class_resume(usbd, IFACE(usbd, i)->param);
    usbd_inform(usbd, USBD_ALERT_RESUME, false);
}

void usbd_fatal(USBD* usbd)
{
    if (usbd->state == USBD_STATE_CONFIGURED)
    {
        usbd->state = USBD_STATE_DEFAULT;
        usbd_inform(usbd, USBD_ALERT_RESET, true);
        usbd_class_reset(usbd);
    }
    ack(usbd->usb, HAL_CMD(HAL_USB, USB_EP_SET_STALL), 0, 0, 0);
    ack(usbd->usb, HAL_CMD(HAL_USB, USB_EP_SET_STALL), USB_EP_IN | 0, 0, 0);
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
#if (USB_2_0)
    ARRAY* cfg_array = (usbd->speed >= USB_HIGH_SPEED) ? usbd->conf_descriptors_hs : usbd->conf_descriptors_fs;
#else
    ARRAY* cfg_array = usbd->conf_descriptors_fs;
#endif //#if (USB_2_0)

    USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg = NULL;
    USB_INTERFACE_DESCRIPTOR_TYPE* iface;
    //find configuration
    for (i = 0; i < cfg_array->size; ++i)
        if ((*(USB_CONFIGURATION_DESCRIPTOR_TYPE**)array_at(cfg_array, i))->bConfigurationValue == usbd->configuration)
        {
            cfg = *(USB_CONFIGURATION_DESCRIPTOR_TYPE**)array_at(cfg_array, i);
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
    for (iface = usbdp_get_first_interface(cfg), usbd->ifacecnt = 0; iface != NULL; iface = usbdp_get_next_interface(cfg, iface))
        if (iface->bInterfaceNumber >= usbd->ifacecnt)
            usbd->ifacecnt = iface->bInterfaceNumber + 1;
    for (i = 0; i <= usbd->ifacecnt; ++i)
        array_append(&usbd->ifaces);
    if (usbd->ifaces == NULL)
    {
#if (USB_DEBUG_ERRORS)
        printf("USBD fatal: Out of memory\n\r");
#endif
        process_exit();
        return;
    }
    for (i = 0; i < usbd->ifacecnt; ++i)
        IFACE(usbd, i)->usbd_class = &__USBD_STUB_CLASS;
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
        usbd->ep_iface[i] = USBD_INVALID_INTERFACE;

    //check all classes for interface
    for (i = 0; __USBD_CLASSES[i] != &__USBD_STUB_CLASS; ++i)
        __USBD_CLASSES[i]->usbd_class_configured(usbd, cfg);

    usbd_inform(usbd, USBD_ALERT_CONFIGURED, false);
}

static inline void usbd_open(USBD* usbd)
{
    if (usbd->block != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    usbd->block = block_create(USBD_BLOCK_SIZE);
    if (usbd->block == INVALID_HANDLE)
        return;
    usbd->setup_state = USB_SETUP_STATE_REQUEST;
    usbd->state = USBD_STATE_DEFAULT;
    usbd->self_powered = usbd->remote_wakeup = false;
#if (USB_2_0)
    usbd->test_mode = USB_TEST_MODE_NORMAL;
#endif //USB_2_0

    usbd->speed = USB_LOW_SPEED;
    usbd->ep0_size = 0;
    usbd->configuration = 0;
    fopen(usbd->usb, HAL_USB, USB_HANDLE_DEVICE, 0);
}

static inline void usbd_close(USBD* usbd)
{
    if (usbd->block == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    if (usbd->state == USBD_STATE_CONFIGURED)
        usbd_class_reset(usbd);
    if (usbd->ep0_size)
    {
        fclose(usbd->usb, HAL_USB, 0);
        fclose(usbd->usb, HAL_USB, USB_EP_IN | 0);
        usbd->ep0_size = 0;
    }

    fclose(usbd->usb, HAL_USB, USB_HANDLE_DEVICE);
    block_destroy(usbd->block);
    usbd->block = INVALID_HANDLE;
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
    if (index < array_size(*ar) && *((void**)array_at(*ar, index)) != NULL)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return false;
    }
    while ((*ar) && array_size(*ar) <= index)
        array_append(ar);
    if (*ar == NULL)
        return false;
    *((void**)array_at(*ar, index)) = ptr;
    return true;
}

static inline bool usbd_register_string(USBD* usbd, unsigned int index, unsigned int lang, void* ptr)
{
    int i;
    for (i = 0; i < array_size(usbd->string_descriptors); ++i)
    {
        if (STRING(usbd, i)->index == index && STRING(usbd, i)->lang == lang)
        {
            error(ERROR_ALREADY_CONFIGURED);
            return false;
        }
    }
    array_append(&usbd->string_descriptors);

    STRING(usbd, array_size(usbd->string_descriptors) - 1)->index = index;
    STRING(usbd, array_size(usbd->string_descriptors) - 1)->lang = lang;
    STRING(usbd, array_size(usbd->string_descriptors) - 1)->string = ptr;
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
    if (index >= array_size(*ar) || *(void**)array_at(*ar, index) == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    //free non-persistent
    free_if_own(*(void**)array_at(*ar, index));
    *(void**)array_at(*ar, index) = NULL;
    if (array_size(*ar) == index + 1)
        array_remove(ar, index);
}

static inline void usbd_unregister_string(USBD* usbd, unsigned int index, unsigned int lang)
{
    int i;
    for (i = 0; i < array_size(usbd->string_descriptors); ++i)
    {
        if (STRING(usbd, i)->index == index && STRING(usbd, i)->lang == lang)
        {
            //free non-persistent
            free_if_own(STRING(usbd, i)->string);
            array_remove(&usbd->string_descriptors, i);
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
#if (USB_2_0)
    case USB_DESCRIPTOR_DEVICE_HS:
        res = usbd_register_device(&usbd->dev_descriptor_hs, ptr);
        break;
    case USB_DESCRIPTOR_CONFIGURATION_HS:
        res = usbd_register_configuration(&usbd->conf_descriptors_hs, udrs->index, ptr);
        break;
#endif //USB_2_0
    case USB_DESCRIPTOR_CONFIGURATION_FS:
        res = usbd_register_configuration(&usbd->conf_descriptors_fs, udrs->index, ptr);
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

static inline void usbd_unregister_descriptor(USBD* usbd, USBD_DESCRIPTOR_TYPE type, unsigned int index, unsigned int lang)
{
    switch (type)
    {
    case USB_DESCRIPTOR_DEVICE_FS:
        usbd_unregister_device(&usbd->dev_descriptor_fs);
        break;
    case USB_DESCRIPTOR_CONFIGURATION_FS:
        usbd_unregister_configuration(&usbd->conf_descriptors_fs, index);
        break;
#if (USB_2_0)
    case USB_DESCRIPTOR_DEVICE_HS:
        usbd_unregister_device(&usbd->dev_descriptor_hs);
        break;
    case USB_DESCRIPTOR_CONFIGURATION_HS:
        usbd_unregister_configuration(&usbd->conf_descriptors_hs, index);
        break;
#endif //USB_2_0
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
    //don't reset on configured state to be functional in virtualbox environment
    if (usbd->state != USBD_STATE_CONFIGURED)
    {
#if (USBD_DEBUG_REQUESTS)
        printf("USB device reset\n\r");
#endif
        usbd->state = USBD_STATE_DEFAULT;
        if (usbd->ep0_size)
        {
            fclose(usbd->usb, HAL_USB, 0);
            fclose(usbd->usb, HAL_USB, USB_EP_IN | 0);
        }

        usbd->speed = speed;
        usbd->ep0_size = usbd->speed == USB_LOW_SPEED ? 8 : 64;

        unsigned int size = usbd->ep0_size;
        fopen_p(usbd->usb, HAL_USB, 0, USB_EP_CONTROL, (void*)size);
        fopen_p(usbd->usb, HAL_USB, USB_EP_IN | 0, USB_EP_CONTROL, (void*)size);
    }
    usbd->setup_state = USB_SETUP_STATE_REQUEST;
}

static inline void usbd_suspend(USBD* usbd)
{
    if (usbd->state == USBD_STATE_CONFIGURED)
    {
#if (USBD_DEBUG_REQUESTS)
        printf("USB device suspend\n\r");
#endif
        usbd_class_suspend(usbd);
        if (usbd->ep0_size)
        {
            fflush(usbd->usb, HAL_USB, 0);
            fflush(usbd->usb, HAL_USB, USB_EP_IN | 0);
        }
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
    }
}

static inline void usbd_wakeup(USBD* usbd)
{
    if (usbd->state == USBD_STATE_CONFIGURED)
    {
#if (USBD_DEBUG_REQUESTS)
        printf("USB device wakeup\n\r");
#endif
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
    dst = *((USB_CONFIGURATION_DESCRIPTOR_TYPE**)array_at(*ar, index));
    return send_descriptor(usbd, dst, type, dst->wTotalLength);
}

static inline int send_strings_descriptor(USBD* usbd, int index, int lang_id)
{
    int i;
    for (i = 0; i < array_size(usbd->string_descriptors); ++i)
        if (STRING(usbd, i)->index == index && STRING(usbd, i)->lang == lang_id)
            return send_descriptor(usbd, STRING(usbd, i)->string, USB_STRING_DESCRIPTOR_INDEX, STRING(usbd, i)->string->bLength);
#if (USBD_DEBUG_ERRORS)
    printf("USB: STRING descriptor %d, lang_id: %#X not present\n\r", index, lang_id);
#endif
    return -1;
}

static inline int usbd_device_get_status(USBD* usbd)
{
#if (USBD_DEBUG_REQUESTS)
    printf("USB: get device status\n\r");
#endif
    uint16_t status = 0;
    if (usbd->self_powered)
        status |= 1 << 0;
    if (usbd->remote_wakeup)
        status |= 1 << 1;
    return safecpy_write(usbd, &status, sizeof(uint16_t));
}

static inline int usbd_device_set_feature(USBD* usbd)
{
    unsigned int res = -1;
    //According to documentation the only feature can be set is TEST_MODE
    //However, linux is issuing feature REMOTE_WAKEUP, so we must handle it to proper
    //remote wakeup
    switch (usbd->setup.wValue)
    {
    case USBD_FEATURE_DEVICE_REMOTE_WAKEUP:
#if (USBD_DEBUG_REQUESTS)
        printf("USB: device set feature REMOTE WAKEUP\n\r");
#endif
        usbd->remote_wakeup = true;
        res = 0;
        break;
#if (USB_2_0)
    case USBD_FEATURE_TEST_MODE:
#if (USBD_DEBUG_REQUESTS)
        printf("USB: device set feature TEST_MODE\n\r");
#endif
        usbd->test_mode = usbd->setup.wIndex >> 16;
        ack(usbd->usb, HAL_CMD(HAL_USB, USB_SET_TEST_MODE), USB_HANDLE_DEVICE, usbd->test_mode, 0);
        res = 0;
        break;
#endif //USB_2_0
    default:
        break;
    }
    return res;
}

static inline int usbd_device_clear_feature(USBD* usbd)
{
    unsigned int res = -1;
#if (USBD_DEBUG_REQUESTS)
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
#if (USBD_DEBUG_REQUESTS)
    printf("USB set ADDRESS %#X\n\r", usbd->setup.wValue);
#endif
    ack(usbd->usb, HAL_CMD(HAL_USB, USB_SET_ADDRESS), USB_HANDLE_DEVICE, usbd->setup.wValue, 0);
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
#if (USBD_DEBUG_REQUESTS)
        printf("USB get DEVICE descriptor\n\r");
#endif
#if (USB_2_0)
        if (usbd->speed >= USB_HIGH_SPEED && usbd->dev_descriptor_hs)
            res = send_descriptor(usbd, usbd->dev_descriptor_hs, USB_DEVICE_DESCRIPTOR_INDEX, usbd->dev_descriptor_hs->bLength);
        else
#endif //USB_2_0
            if (usbd->dev_descriptor_fs)
                res = send_descriptor(usbd, usbd->dev_descriptor_fs, USB_DEVICE_DESCRIPTOR_INDEX, usbd->dev_descriptor_fs->bLength);
        break;
    case USB_CONFIGURATION_DESCRIPTOR_INDEX:
#if (USBD_DEBUG_REQUESTS)
        printf("USB get CONFIGURATION %d descriptor\n\r", index);
#endif
#if (USB_2_0)
        if (usbd->speed >= USB_HIGH_SPEED && index < usbd->conf_descriptors_hs->size)
            res = send_configuration_descriptor(usbd, &usbd->conf_descriptors_hs, index, USB_CONFIGURATION_DESCRIPTOR_INDEX);
        else
#endif //USB_2_0
            if (index < usbd->conf_descriptors_fs->size)
                res = send_configuration_descriptor(usbd, &usbd->conf_descriptors_fs, index, USB_CONFIGURATION_DESCRIPTOR_INDEX);
        break;
    case USB_STRING_DESCRIPTOR_INDEX:
#if (USBD_DEBUG_REQUESTS)
        printf("USB get STRING %d descriptor, LangID: %#X\n\r", index, usbd->setup.wIndex);
#endif
        res = send_strings_descriptor(usbd, index, usbd->setup.wIndex);
        break;
#if (USB_2_0)
    case USB_DEVICE_QUALIFIER_DESCRIPTOR_INDEX:
#if (USBD_DEBUG_REQUESTS)
        printf("USB get DEVICE qualifier descriptor\n\r");
#endif
        if (usbd->speed >= USB_HIGH_SPEED && usbd->dev_descriptor_fs)
            res = send_descriptor(usbd, usbd->dev_descriptor_fs, USB_DEVICE_QUALIFIER_DESCRIPTOR_INDEX, usbd->dev_descriptor_fs->bLength);
        else if (usbd->dev_descriptor_hs)
            res = send_descriptor(usbd, usbd->dev_descriptor_hs, USB_DEVICE_QUALIFIER_DESCRIPTOR_INDEX, usbd->dev_descriptor_hs->bLength);
        else if (usbd->dev_descriptor_fs)
            res = send_descriptor(usbd, usbd->dev_descriptor_fs, USB_DEVICE_QUALIFIER_DESCRIPTOR_INDEX, usbd->dev_descriptor_fs->bLength);
        break;
    case USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX:
#if (USBD_DEBUG_REQUESTS)
        printf("USB get other speed CONFIGURATION %d descriptor\n\r", index);
#endif
        if (usbd->speed >= USB_HIGH_SPEED && index < usbd->conf_descriptors_fs->size)
            res = send_configuration_descriptor(usbd, &usbd->conf_descriptors_fs, index, USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX);
        else if (index < usbd->conf_descriptors_hs->size)
            res = send_configuration_descriptor(usbd, &usbd->conf_descriptors_hs, index, USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX);
        else if (index < usbd->conf_descriptors_fs->size)
            res = send_configuration_descriptor(usbd, &usbd->conf_descriptors_fs, index, USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX);
        break;
#endif
    }

    return res;
}

static inline int usbd_get_configuration(USBD* usbd)
{
#if (USBD_DEBUG_REQUESTS)
    printf("USB: get configuration\n\r");
#endif
    char configuration = (char)usbd->configuration;
    return safecpy_write(usbd, &configuration, 1);
}

static inline int usbd_set_configuration(USBD* usbd)
{
#if (USBD_DEBUG_REQUESTS)
    printf("USB: set configuration %d\n\r", usbd->setup.wValue);
#endif
    if (usbd->state == USBD_STATE_CONFIGURED)
    {
        usbd->configuration = 0;

        usbd->state = USBD_STATE_ADDRESSED;
        usbd_inform(usbd, USBD_ALERT_RESET, true);
        usbd_class_reset(usbd);
    }
    //USB 2.0 specification is required to set USB state addressed if it was already configured
    //However, due to virtualbox bug - if device is switching connection between VMs, it's issuing
    //SET CONFIGURATION without prior reset, causing logical reset, so this feature is disabled
    if (usbd->state == USBD_STATE_ADDRESSED && usbd->setup.wValue)
    {
        usbd->configuration = usbd->setup.wValue;
        usbd->state = USBD_STATE_CONFIGURED;

        usbd_class_configured(usbd);
    }
    return 0;
}

static inline int usbd_standart_device_request(USBD* usbd)
{
    int res = -1;
    switch (usbd->setup.bRequest)
    {
    case USB_REQUEST_GET_STATUS:
        res = usbd_device_get_status(usbd);
        break;
    case USB_REQUEST_SET_FEATURE:
        res = usbd_device_set_feature(usbd);
        break;
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

static inline int usbd_endpoint_get_status(USBD* usbd)
{
#if (USBD_DEBUG_REQUESTS)
    printf("USB: get endpoint status\n\r");
#endif
    uint16_t status = 0;
    if (get(usbd->usb, HAL_CMD(HAL_USB, USB_EP_IS_STALL), usbd->setup.wIndex, 0, 0))
        status |= 1 << 0;
    return safecpy_write(usbd, &status, sizeof(uint16_t));
}

static inline int usbd_endpoint_set_feature(USBD* usbd)
{
    unsigned int res = -1;
#if (USBD_DEBUG_REQUESTS)
    printf("USB: endpoint set feature\n\r");
#endif
    switch (usbd->setup.wValue)
    {
    case USBD_FEATURE_ENDPOINT_HALT:
        ack(usbd->usb, HAL_CMD(HAL_USB, USB_EP_SET_STALL), usbd->setup.wIndex, 0, 0);
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
#if (USBD_DEBUG_REQUESTS)
    printf("USB: endpoint clear feature\n\r");
#endif
    switch (usbd->setup.wValue)
    {
    case USBD_FEATURE_ENDPOINT_HALT:
        ack(usbd->usb, HAL_CMD(HAL_USB, USB_EP_CLEAR_STALL), usbd->setup.wIndex, 0, 0);
        res = 0;
        break;
    default:
        break;
    }
    return res;
}

static inline int usbd_interface_request(USBD* usbd, int iface)
{
    int res = -1;
    if (iface < usbd->ifacecnt)
        res = IFACE(usbd, iface)->usbd_class->usbd_class_setup(usbd, IFACE(usbd, iface)->param, &usbd->setup, usbd->block);
#if (USBD_DEBUG_ERRORS)
    else
        printf("USBD: Interface %d is not configured\n\r");
#endif
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

static void usbd_setup_response(USBD* usbd, int res)
{
    if (res > usbd->setup.wLength)
        res = usbd->setup.wLength;
    //success. start transfers
    if (res >= 0)
    {
        if ((usbd->setup.bmRequestType & BM_REQUEST_DIRECTION) == BM_REQUEST_DIRECTION_HOST_TO_DEVICE)
        {
            //data already received, sending status
            usbd->setup_state = USB_SETUP_STATE_STATUS_IN;
            fwrite_async(usbd->usb, HAL_USB, USB_EP_IN | 0, INVALID_HANDLE, 0);
        }
        else
        {
            //response less, than required and multiples of EP0SIZE - we need to send ZLP on end of transfers
            if (res < usbd->setup.wLength && ((res % usbd->ep0_size) == 0))
            {
                if (res)
                {
                    usbd->setup_state = USB_SETUP_STATE_DATA_IN_ZLP;
                    fwrite_async(usbd->usb, HAL_USB, USB_EP_IN | 0, usbd->block, res);
                }
                //if no data at all, but request success, we will send ZLP right now
                else
                {
                    usbd->setup_state = USB_SETUP_STATE_DATA_IN;
                    fwrite_async(usbd->usb, HAL_USB, USB_EP_IN | 0, INVALID_HANDLE, 0);
                }
            }
            else if (res)
            {
                usbd->setup_state = USB_SETUP_STATE_DATA_IN;
                fwrite_async(usbd->usb, HAL_USB, USB_EP_IN | 0, usbd->block, res);
            }
            //data stage is optional
            else
            {
                usbd->setup_state = USB_SETUP_STATE_STATUS_OUT;
                fread_async(usbd->usb, HAL_USB, 0, INVALID_HANDLE, 0);
            }
        }
    }
    else
    {
        if ((usbd->setup.bmRequestType & BM_REQUEST_RECIPIENT) == BM_REQUEST_RECIPIENT_ENDPOINT)
            ack(usbd->usb, HAL_CMD(HAL_USB, USB_EP_SET_STALL), usbd->setup.wIndex, 0, 0);
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
#if (USBD_DEBUG_ERRORS)
        printf("Unhandled ");
        switch (usbd->setup.bmRequestType & BM_REQUEST_RECIPIENT)
        {
        case BM_REQUEST_RECIPIENT_DEVICE:
            printf("DEVICE");
            break;
        case BM_REQUEST_RECIPIENT_INTERFACE:
            printf("INTERFACE");
            break;
        case BM_REQUEST_RECIPIENT_ENDPOINT:
            printf("ENDPOINT");
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

void usbd_setup_process(USBD* usbd)
{
    int res = -1;
    switch (usbd->setup.bmRequestType & BM_REQUEST_RECIPIENT)
    {
    case BM_REQUEST_RECIPIENT_DEVICE:
        if ((usbd->setup.bmRequestType & BM_REQUEST_TYPE) == BM_REQUEST_TYPE_STANDART)
            res = usbd_standart_device_request(usbd);
#if (USBD_VSR)
        else if (usbd->user != INVALID_HANDLE)
        {
            usbd->setup_state = USB_SETUP_STATE_VENDOR_REQUEST;
            block_send(usbd->block, usbd->user);
            ipc_post_inline(usbd->user, HAL_CMD(HAL_USBD, USBD_VENDOR_REQUEST), ((uint32_t*)(&usbd->setup))[0], ((uint32_t*)(&usbd->setup))[1], usbd->block);
            return;
        }
#endif
        break;
    case BM_REQUEST_RECIPIENT_INTERFACE:
        res = usbd_interface_request(usbd, usbd->setup.wIndex);
        break;
    case BM_REQUEST_RECIPIENT_ENDPOINT:
        res = usbd_endpoint_request(usbd);
        break;
    }
    usbd_setup_response(usbd, res);
}

static inline void usbd_setup_received(USBD* usbd)
{
    //Back2Back setup received
    if (usbd->setup_state != USB_SETUP_STATE_REQUEST)
    {
#if (USBD_DEBUG_REQUESTS)
        printf("USB B2B SETUP received, state: %d\n\r", usbd->setup_state);
#endif
        //reset control EP if transaction in progress
        switch (usbd->setup_state)
        {
        case USB_SETUP_STATE_DATA_IN:
        case USB_SETUP_STATE_DATA_IN_ZLP:
        case USB_SETUP_STATE_STATUS_IN:
            fflush(usbd->usb, HAL_USB, USB_EP_IN | 0);
            break;
        case USB_SETUP_STATE_DATA_OUT:
        case USB_SETUP_STATE_STATUS_OUT:
            fflush(usbd->usb, HAL_USB, 0);
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
            fread_async(usbd->usb, HAL_USB, 0, usbd->block, usbd->setup.wLength);
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
#if (USBD_DEBUG_ERRORS)
        printf("USBD invalid setup state on read: %d\n\r", usbd->setup_state);
#endif
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
        break;
    }
}

void usbd_write_complete(USBD* usbd)
{
    switch (usbd->setup_state)
    {
    case USB_SETUP_STATE_DATA_IN_ZLP:
        //TX ZLP and switch to normal state
        usbd->setup_state = USB_SETUP_STATE_DATA_IN;
        fwrite_async(usbd->usb, HAL_USB, USB_EP_IN | 0, INVALID_HANDLE, 0);
        break;
    case USB_SETUP_STATE_DATA_IN:
        usbd->setup_state = USB_SETUP_STATE_STATUS_OUT;
        fread_async(usbd->usb, HAL_USB, 0, INVALID_HANDLE, 0);
        break;
    case USB_SETUP_STATE_STATUS_IN:
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
        break;
    default:
#if (USBD_DEBUG_ERRORS)
        printf("USBD invalid state on write: %d\n\r", usbd->setup_state);
#endif
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
        break;
    }
}

static inline void usbd_init(USBD* usbd)
{
    int i;
    usbd->user = INVALID_HANDLE;
    usbd->usb = object_get(SYS_OBJ_USB);
    usbd->dev_descriptor_fs = NULL;
    usbd->block = INVALID_HANDLE;
#if (USB_2_0)
    usbd->dev_descriptor_hs = NULL;
#endif //USB_2_0
    array_create(&usbd->conf_descriptors_fs, sizeof(void*), 1);
#if (USB_2_0)
    array_create(&usbd->conf_descriptors_hs, sizeof(void*), 1);
#endif //USB_2_0
    array_create(&usbd->ifaces, sizeof(USBD_IFACE_ENTRY), 1);
    //at least 3: manufacturer, product, string 0
    array_create(&usbd->string_descriptors, sizeof(USBD_STRING), 3);

    usbd->ifacecnt = 0;
    for (i = 0; i < USB_EP_COUNT_MAX; ++i)
        usbd->ep_iface[i] = USBD_INVALID_INTERFACE;
}

static inline void usbd_register_user(USBD* usbd, HANDLE process)
{
    if (usbd->user != INVALID_HANDLE)
        error(ERROR_ALREADY_CONFIGURED);
    else
        usbd->user = process;
}

static inline void usbd_unregister_user(USBD* usbd, HANDLE process)
{
    if (usbd->user != process)
        error(ERROR_ACCESS_DENIED);
    else
        usbd->user = INVALID_HANDLE;
}

#if (USBD_VSR)
static inline void usbd_vendor_response(USBD* usbd, int res)
{
    if (usbd->setup_state == USB_SETUP_STATE_VENDOR_REQUEST)
    {
        usbd->setup_state = USB_SETUP_STATE_REQUEST;
        usbd_setup_response(usbd, res);
    }
#if (USBD_DEBUG_ERRORS)
    else
        printf("USBD: Unexpected vendor response\n\r");
#endif
}
#endif //USBD_VSR

static inline bool usbd_device_request(USBD* usbd, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case USBD_REGISTER_DESCRIPTOR:
        usbd_register_descriptor(usbd, ipc->param1, ipc->param2, ipc->process);
        need_post = true;
        break;
    case USBD_UNREGISTER_DESCRIPTOR:
        usbd_unregister_descriptor(usbd, ipc->param1, ipc->param2, ipc->param3);
        need_post = true;
        break;
    case USBD_REGISTER_HANDLER:
        usbd_register_user(usbd, ipc->process);
        need_post = true;
        break;
    case USBD_UNREGISTER_HANDLER:
        usbd_unregister_user(usbd, ipc->process);
        need_post = true;
        break;
    case USBD_GET_STATE:
        ipc->param2 = usbd->state;
        need_post = true;
        break;
    case USBD_VENDOR_REQUEST:
        usbd_vendor_response(usbd, ipc->param1);
        break;
    case IPC_OPEN:
        usbd_open(usbd);
        need_post = true;
        break;
    case IPC_CLOSE:
        usbd_close(usbd);
        need_post = true;
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

static inline bool usbd_driver_event(USBD* usbd, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case USB_RESET:
        usbd_reset(usbd, ipc->param2);
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
        ((uint32_t*)(&usbd->setup))[0] = ipc->param2;
        ((uint32_t*)(&usbd->setup))[1] = ipc->param3;
        usbd_setup_received(usbd);
        break;
    case IPC_READ:
        usbd_read_complete(usbd);
        break;
    case IPC_WRITE:
        usbd_write_complete(usbd);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

bool usbd_class_interface_request(USBD* usbd, IPC* ipc, unsigned int iface)
{
    if (iface >= usbd->ifacecnt)
    {
        error(ERROR_INVALID_PARAMS);
        return true;
    }
    return IFACE(usbd, iface)->usbd_class->usbd_class_request(usbd, IFACE(usbd, iface)->param, ipc);
}

bool usbd_class_endpoint_request(USBD *usbd, IPC* ipc, unsigned int num)
{
    if (num >= USB_EP_COUNT_MAX || usbd->ep_iface[num] >= usbd->ifacecnt)
    {
        error(ERROR_INVALID_PARAMS);
        return true;
    }
    return IFACE(usbd, usbd->ep_iface[num])->usbd_class->usbd_class_request(usbd, IFACE(usbd, usbd->ep_iface[num])->param, ipc);
}

void usbd()
{
    USBD usbd;
    IPC ipc;
    bool need_post;

#if (USBD_DEBUG)
    open_stdout();
#endif
    object_set_self(SYS_OBJ_USBD);

    usbd_init(&usbd);
    for (;;)
    {
        ipc_read_ms(&ipc, 0, ANY_HANDLE);
        error(ERROR_OK);
        need_post = false;
        if (ipc.cmd == HAL_CMD(HAL_SYSTEM, IPC_PING))
            need_post = true;
        else
            switch (HAL_GROUP(ipc.cmd))
            {
            case HAL_USB:
                if (USB_EP_NUM(ipc.param1) == 0 || ipc.param1 == USB_HANDLE_DEVICE)
                    usbd_driver_event(&usbd, &ipc);
                //decode endpoint interface
                else
                    usbd_class_endpoint_request(&usbd, &ipc, USB_EP_NUM(ipc.param1));
                 break;
            case HAL_USBD:
                need_post = usbd_device_request(&usbd, &ipc);
                break;
            case HAL_USBD_IFACE:
                need_post = usbd_class_interface_request(&usbd, &ipc, USBD_IFACE_NUM(ipc.param1));
                break;
            default:
                error(ERROR_NOT_SUPPORTED);
                need_post = true;
            }
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}

bool usbd_register_interface(USBD* usbd, unsigned int iface, const USBD_CLASS* usbd_class, void* param)
{
    if (iface >= usbd->ifacecnt)
        return false;
    if (IFACE(usbd, iface)->usbd_class != &__USBD_STUB_CLASS)
        return false;
    IFACE(usbd, iface)->usbd_class = usbd_class;
    IFACE(usbd, iface)->param = param;
    return true;
}

bool usbd_unregister_interface(USBD* usbd, unsigned int iface, const USBD_CLASS* usbd_class)
{
    if (iface >= usbd->ifacecnt)
        return false;
    if (IFACE(usbd, iface)->usbd_class != usbd_class)
        return false;
    IFACE(usbd, iface)->usbd_class = &__USBD_STUB_CLASS;
    return true;
}

bool usbd_register_endpoint(USBD* usbd, unsigned int iface, unsigned int num)
{
    if (iface >= usbd->ifacecnt || num >= USB_EP_COUNT_MAX)
        return false;
    if (IFACE(usbd, iface)->usbd_class == &__USBD_STUB_CLASS)
        return false;
    if (usbd->ep_iface[num] != USBD_INVALID_INTERFACE)
        return false;
    usbd->ep_iface[num] = iface;
    return true;
}

bool usbd_unregister_endpoint(USBD* usbd, unsigned int iface, unsigned int num)
{
    if (iface >= usbd->ifacecnt || num >= USB_EP_COUNT_MAX)
        return false;
    if (IFACE(usbd, iface)->usbd_class == &__USBD_STUB_CLASS)
        return false;
    if (usbd->ep_iface[num] != iface)
        return false;
    usbd->ep_iface[num] = USBD_INVALID_INTERFACE;
    return true;
}

void usbd_post_user(USBD* usbd, unsigned int iface, unsigned int num, unsigned int cmd, unsigned int param2, unsigned int param3)
{
    if (usbd->user == INVALID_HANDLE)
        return;
    IPC ipc;
    ipc.cmd = HAL_CMD(HAL_USBD_IFACE, cmd);
    ipc.process = usbd->user;
    ipc.param1 = USBD_IFACE(iface, num);
    ipc.param2 = param2;
    ipc.param3 = param3;
    ipc_post(&ipc);
}

HANDLE usbd_user(USBD* usbd)
{
    return usbd->user;
}

HANDLE usbd_usb(USBD* usbd)
{
    return usbd->usb;
}

#if (USBD_DEBUG)
void usbd_dump(const uint8_t* buf, unsigned int size, const char* header)
{
    int i;
    printf("%s: ", header);
    for (i = 0; i < size; ++i)
        printf("%02X ", buf[i]);
    printf("\n\r");
}
#endif
