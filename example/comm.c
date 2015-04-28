/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "comm.h"
#include "../../rexos/userspace/stdio.h"
#include "../../rexos/userspace/libusb.h"
#include "../../rexos/userspace/file.h"
#include "../../rexos/userspace/sys.h"
#include "../../rexos/userspace/stream.h"
#include "../../rexos/midware/usbd.h"
#include "../../rexos/drv/stm32/stm32_usbl.h"
#include "usb_desc.h"
#include "sys_config.h"
#include "stm32_config.h"
#include "app_private.h"

static inline void comm_usb_start(APP* app)
{
/*    HANDLE usbd, tx_stream;
    usbd = object_get(SYS_OBJ_USBD);
    tx_stream = get(usbd, IPC_GET_TX_STREAM, HAL_USBD_INTERFACE(0, 0), 0, 0);
    app->comm.rx_stream = get(usbd, IPC_GET_RX_STREAM, HAL_USBD_INTERFACE(0, 0), 0, 0);

    app->comm.tx = stream_open(tx_stream);
    app->comm.rx = stream_open(app->comm.rx_stream);
    app->comm.active = true;

    stream_listen(app->comm.rx_stream, NULL);
*/
    printf("USB start\n\r");
    sleep_ms(1000);
    fclose(object_get(SYS_OBJ_USBD), HAL_HANDLE(HAL_USBD, USBD_HANDLE_DEVICE));
    sleep_ms(1000);

    printf("USB test start\n\r");
    fopen(object_get(SYS_OBJ_USBD), HAL_HANDLE(HAL_USBD, USBD_HANDLE_DEVICE), 0);

}

static void comm_usb_stop(APP* app)
{
    if (app->comm.active)
    {
        stream_stop_listen(app->comm.rx_stream);
        stream_close(app->comm.tx);
        stream_close(app->comm.rx);
        app->comm.rx = app->comm.tx = app->comm.rx_stream = INVALID_HANDLE;
        app->comm.active = false;
        printf("USB stop\n\r");
    }
}

bool comm_usbd_alert(APP* app, USBD_ALERTS alert)
{
    bool need_post = false;
    switch (alert)
    {
    case USBD_ALERT_CONFIGURED:
    case USBD_ALERT_RESUME:
        comm_usb_start(app);
        break;
    case USBD_ALERT_RESET:
    case USBD_ALERT_SUSPEND:
        comm_usb_stop(app);
        need_post = true;
        break;
    default:
        break;
    }
    return need_post;
}

void comm_usbd_stream_rx(APP* app, unsigned int size)
{
    printf("rx: %d ", size);
    char c;
    unsigned int i;
    for (i = 0; i < size; ++i)
    {
        stream_read(app->comm.rx, &c, 1);
        //echo
        stream_write(app->comm.tx, &c, 1);
        if ((uint8_t)c >= ' ' && (uint8_t)c <= '~')
            printf("%c", c);
        else
            printf("\\x%u", (uint8_t)c);
    }
    printf("\n\r");
    stream_listen(app->comm.rx_stream, NULL);
}

void comm_init(APP *app)
{
    HANDLE usbd;
#if !(MONOLITH_USB)
    process_create(&__STM32_USBL);
#endif
    app->comm.rx = app->comm.tx = app->comm.rx_stream = INVALID_HANDLE;

    //setup usbd
    usbd = process_create(&__USBD);
    ack(usbd, USBD_REGISTER_HANDLER, 0, 0, 0);

    libusb_register_descriptor(USB_DESCRIPTOR_DEVICE_FS, 0, 0, &__DEVICE_DESCRIPTOR, sizeof(__DEVICE_DESCRIPTOR), USBD_FLAG_PERSISTENT_DESCRIPTOR);
    libusb_register_descriptor(USB_DESCRIPTOR_CONFIGURATION_FS, 0, 0, &__CONFIGURATION_DESCRIPTOR, sizeof(__CONFIGURATION_DESCRIPTOR), USBD_FLAG_PERSISTENT_DESCRIPTOR);
    libusb_register_descriptor(USB_DESCRIPTOR_STRING, 0, 0, &__STRING_WLANGS, __STRING_WLANGS[0], USBD_FLAG_PERSISTENT_DESCRIPTOR);
    libusb_register_descriptor(USB_DESCRIPTOR_STRING, 1, 0x0409, &__STRING_MANUFACTURER, __STRING_MANUFACTURER[0], USBD_FLAG_PERSISTENT_DESCRIPTOR);
    libusb_register_descriptor(USB_DESCRIPTOR_STRING, 2, 0x0409, &__STRING_PRODUCT, __STRING_PRODUCT[0], USBD_FLAG_PERSISTENT_DESCRIPTOR);
    libusb_register_descriptor(USB_DESCRIPTOR_STRING, 3, 0x0409, &__STRING_SERIAL, __STRING_SERIAL[0], USBD_FLAG_PERSISTENT_DESCRIPTOR);
    libusb_register_descriptor(USB_DESCRIPTOR_STRING, 4, 0x0409, &__STRING_DEFAULT, __STRING_DEFAULT[0], USBD_FLAG_PERSISTENT_DESCRIPTOR);

    fopen(object_get(SYS_OBJ_USBD), HAL_HANDLE(HAL_USBD, USBD_HANDLE_DEVICE), 0);

    printf("Comm init\n\r");
}
