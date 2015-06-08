/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "comm.h"
#include "../../rexos/userspace/stdio.h"
#include "../../rexos/userspace/usb.h"
#include "../../rexos/userspace/file.h"
#include "../../rexos/userspace/sys.h"
#include "../../rexos/userspace/stream.h"
#include "../../rexos/midware/usbd.h"
#include "../../rexos/drv/stm32/stm32_usb.h"
#include "usb_desc.h"
#include "sys_config.h"
#include "stm32_config.h"
#include "app_private.h"

static inline void comm_usb_start(APP* app)
{
    HANDLE usbd, tx_stream;
    usbd = object_get(SYS_OBJ_USBD);
    tx_stream = get(usbd, HAL_CMD(HAL_USBD_IFACE, IPC_GET_TX_STREAM), USBD_IFACE(0, 0), 0, 0);
    app->comm.rx_stream = get(usbd, HAL_CMD(HAL_USBD_IFACE, IPC_GET_RX_STREAM), USBD_IFACE(0, 0), 0, 0);

    app->comm.tx = stream_open(tx_stream);
    app->comm.rx = stream_open(app->comm.rx_stream);
    app->comm.active = true;

    stream_listen(app->comm.rx_stream, 0, HAL_APP);

    printf("USB start\n\r");
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

void comm_usbd_alert(APP* app, USBD_ALERTS alert)
{
    switch (alert)
    {
    case USBD_ALERT_CONFIGURED:
    case USBD_ALERT_RESUME:
        comm_usb_start(app);
        break;
    case USBD_ALERT_RESET:
    case USBD_ALERT_SUSPEND:
        comm_usb_stop(app);
        break;
    default:
        break;
    }
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
    stream_listen(app->comm.rx_stream, 0, HAL_APP);
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
    ack(usbd, HAL_CMD(HAL_USBD, USBD_REGISTER_HANDLER), 0, 0, 0);

/*    libusb_register_descriptor(0, 0, &__DEVICE_DESCRIPTOR, sizeof(__DEVICE_DESCRIPTOR));
    libusb_register_descriptor(0, 0, &__CONFIGURATION_DESCRIPTOR, sizeof(__CONFIGURATION_DESCRIPTOR));
    libusb_register_descriptor(0, 0, &__STRING_WLANGS, __STRING_WLANGS[0]);
    libusb_register_descriptor(1, 0x0409, &__STRING_MANUFACTURER, __STRING_MANUFACTURER[0]);
    libusb_register_descriptor(2, 0x0409, &__STRING_PRODUCT, __STRING_PRODUCT[0]);
    libusb_register_descriptor(3, 0x0409, &__STRING_SERIAL, __STRING_SERIAL[0]);
    libusb_register_descriptor(4, 0x0409, &__STRING_DEFAULT, __STRING_DEFAULT[0]);
*/
    fopen(object_get(SYS_OBJ_USBD), HAL_USBD, 0, 0);

    printf("Comm init\n\r");
}
