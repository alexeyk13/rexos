#include "comm.h"
#include "../../rexos/userspace/stdio.h"
#include "../../rexos/userspace/file.h"
#include "../../rexos/userspace/sys.h"
#include "../../rexos/userspace/stream.h"
#include "../../rexos/sys/midware/usbd.h"
#include "../../rexos/userspace/gpio.h"
#include "../../rexos/userspace/stm32_driver.h"
#include "../../rexos/sys/drv/stm32/stm32_usb.h"
#include "usb_desc.h"
#include "sys_config.h"
#include "stm32_config.h"

typedef struct {
    HANDLE tx, rx, rx_stream;
}COMM;

void comm();

const REX __COMM = {
    //name
    "USB comm",
    //size
    512,
    //priority
    201,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    3,
    //function
    comm
};


static inline void comm_usb_configured(COMM* comm)
{
    HANDLE usbd, tx_stream;
    usbd = object_get(SYS_OBJ_USBD);
    tx_stream = get(usbd, IPC_GET_TX_STREAM, HAL_HANDLE(HAL_USBD, USBD_HANDLE_INTERFACE + 1), 0, 0);
    comm->rx_stream = get(usbd, IPC_GET_RX_STREAM, HAL_HANDLE(HAL_USBD, USBD_HANDLE_INTERFACE + 1), 0, 0);

    comm->tx = stream_open(tx_stream);
    comm->rx = stream_open(comm->rx_stream);

    stream_listen(comm->rx_stream, NULL);

    printf("USB configured\n\r");
}

static inline void comm_usb_reset(COMM* comm)
{
    stream_stop_listen(comm->rx_stream);
    stream_close(comm->tx);
    stream_close(comm->rx);
    comm->rx = comm->tx = comm->rx_stream = INVALID_HANDLE;
    printf("USB reset\n\r");
}

static inline void comm_usb_suspend(COMM* comm)
{
    stream_stop_listen(comm->rx_stream);
    printf("USB suspend\n\r");
}

static inline void comm_usb_resume(COMM* comm)
{
    stream_listen(comm->rx_stream, NULL);
    printf("USB resume\n\r");
}

void comm_usbd_alert(COMM* comm, USBD_ALERTS alert)
{
    switch (alert)
    {
    case USBD_ALERT_CONFIGURED:
        comm_usb_configured(comm);
        break;
    case USBD_ALERT_RESET:
        comm_usb_reset(comm);
        break;
    case USBD_ALERT_SUSPEND:
        comm_usb_suspend(comm);
        break;
    case USBD_ALERT_RESUME:
        comm_usb_resume(comm);
        break;
    }
}

void comm_usbd_stream_rx(COMM* comm, unsigned int size)
{
    printf("rx: ");
    char c;
    unsigned int i;
    for (i = 0; i < size; ++i)
    {
        stream_read(comm->rx, &c, 1);
        //echo
        stream_write(comm->tx, &c, 1);
        if ((uint8_t)c >= ' ' && (uint8_t)c <= '~')
            printf("%c", c);
        else
            printf("\\x%u", (uint8_t)c);
    }
    printf("\n\r");
    stream_listen(comm->rx_stream, NULL);
}

void te_usb_disable_power()
{
    gpio_enable_pin(C9, GPIO_MODE_OUT);
    gpio_set_pin(C9);
}


void comm_init(COMM *comm)
{
    open_stdout();

    printf("Comm started\n\r");

    HANDLE usbd;
#if !(MONOLITH_USB)
    process_create(&__STM32_USB);
#endif
    comm->rx = comm->tx = comm->rx_stream = INVALID_HANDLE;

    //setup usbd
    usbd = process_create(&__USBD);

    usb_register_persistent_descriptor(USB_DESCRIPTOR_DEVICE_FS, 0, 0, &__DEVICE_DESCRIPTOR);
    usb_register_persistent_descriptor(USB_DESCRIPTOR_CONFIGURATION_FS, 0, 0, &__CONFIGURATION_DESCRIPTOR);
    usb_register_persistent_descriptor(USB_DESCRIPTOR_STRING, 0, 0, &__STRING_WLANGS);
    usb_register_persistent_descriptor(USB_DESCRIPTOR_STRING, 1, 0x0409, &__STRING_MANUFACTURER);
    usb_register_persistent_descriptor(USB_DESCRIPTOR_STRING, 2, 0x0409, &__STRING_PRODUCT);
    usb_register_persistent_descriptor(USB_DESCRIPTOR_STRING, 3, 0x0409, &__STRING_SERIAL);
    usb_register_persistent_descriptor(USB_DESCRIPTOR_STRING, 4, 0x0409, &__STRING_DEFAULT);

    te_usb_disable_power();
    //turn USB on
    fopen(usbd, HAL_HANDLE(HAL_USBD, USBD_HANDLE_DEVICE), 0);

    ack(usbd, USBD_REGISTER_HANDLER, 0, 0, 0);

    printf("USB activated\n\r");
}

void comm()
{
    COMM comm;
    comm_init(&comm);
    IPC ipc;
    bool need_post;

    for (;;)
    {
        error(ERROR_OK);
        ipc_read_ms(&ipc, 0, ANY_HANDLE);
        need_post = false;
        switch (ipc.cmd)
        {
        case IPC_PING:
            need_post = true;
            break;
        case IPC_CALL_ERROR:
            break;
        case USBD_ALERT:
            comm_usbd_alert(&comm, ipc.param1);
            break;
        case IPC_STREAM_WRITE:
            comm_usbd_stream_rx(&comm, ipc.param2);
            break;
        default:
            need_post = true;
            break;
        }
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}
