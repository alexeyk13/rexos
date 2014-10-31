#include "../../rexos/userspace/process.h"
#include "../../rexos/userspace/lib/time.h"
#include "../../rexos/userspace/lib/stdlib.h"
#include "../../rexos/userspace/lib/stdio.h"
#include "../../rexos/userspace/lib/heap.h"
#include "../../rexos/userspace/timer.h"
#include "../../rexos/userspace/ipc.h"
#include "../../rexos/userspace/stream.h"
#include "../../rexos/userspace/direct.h"
#include "../../rexos/userspace/object.h"
#include "string.h"
#include "../../rexos/sys/sys.h"
#include "../../rexos/sys/gpio.h"
#include "../../rexos/sys/rtc.h"
#include "../../rexos/sys/wdt.h"
#include "../../rexos/sys/drv/stm32_uart.h"
#include "../../rexos/sys/drv/stm32_analog.h"
#include "../../rexos/sys/drv/stm32_usb.h"
#include "../../rexos/sys/midware/cdc.h"
#include "../../rexos/sys/midware/usbd.h"
#include "../../rexos/sys/usb.h"
#include "../../rexos/sys/file.h"
#include "usb_desc.h"

#define TEST_ROUNDS                                 10000

void app();
void test_thread3();

const REX __APP = {
    //name
    "App main",
    //size
    512,
    //priority
    120,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    10,
    //function
    app
};

const REX test3 = {
    //name
    "test3",
    //size
    512,
    //priority
    119,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    10,
    //function
    test_thread3
};

void test_thread3()
{
    TIME uptime;
    open_stdout();
    printf("test thread3 started\n\r");
    printf("my name is: %s\n\r", process_name());

    gpio_enable_pin(B9, PIN_MODE_OUT);

    wdt_kick();
    process_info();
    bool set = true;
    for (;;)
    {
        get_uptime(&uptime);
        printf("uptime: %02d:%02d.%06d\n\r", uptime.sec / 60, uptime.sec % 60, uptime.usec);
        printf("rtc: %d\n\r", rtc_get());
        gpio_set_pin(B9, set);
        wdt_kick();
        set = !set;
        sleep_ms(700);
    }
}

void te_usb_disable_power()
{
    gpio_enable_pin(C9, PIN_MODE_OUT);
    gpio_set_pin(C9, true);
}

HANDLE usb_on(HANDLE core)
{
    HANDLE usb, usbd, cdc;
    CDC_OPEN_STRUCT cos;
    te_usb_disable_power();

#if (MONOLITH_USB)
    usb = core;
#else
    usb = process_create(&__STM32_USB);
#endif

    //setup usbd
    usbd = process_create(&__USBD);

    libusb_register_persistent_descriptor(usbd, USB_DESCRIPTOR_DEVICE_FS, 0, 0, &__DEVICE_DESCRIPTOR);
    libusb_register_persistent_descriptor(usbd, USB_DESCRIPTOR_CONFIGURATION_FS, 0, 0, &__CONFIGURATION_DESCRIPTOR);
    libusb_register_persistent_descriptor(usbd, USB_DESCRIPTOR_STRING, 0, 0, &__STRING_WLANGS);
    libusb_register_persistent_descriptor(usbd, USB_DESCRIPTOR_STRING, 1, 0x0409, &__STRING_MANUFACTURER);
    libusb_register_persistent_descriptor(usbd, USB_DESCRIPTOR_STRING, 2, 0x0409, &__STRING_PRODUCT);
    libusb_register_persistent_descriptor(usbd, USB_DESCRIPTOR_STRING, 3, 0x0409, &__STRING_SERIAL);
    libusb_register_persistent_descriptor(usbd, USB_DESCRIPTOR_STRING, 4, 0x0409, &__STRING_DEFAULT);
    
    fopen(usbd, usb);

    //setup cdc
    cdc = process_create(&__CDC);
    cos.data_ep = 1;
    cos.control_ep = 2;
    cos.data_ep_size = 64;
    cos.control_ep_size = 16;
    cos.rx_stream_size = cos.tx_stream_size = 32;
    fopen_ex(cdc, usbd, (void*)&cos, sizeof(CDC_OPEN_STRUCT));

    //turn USB on
    USB_OPEN uo;
    uo.device = usbd;

    fopen_ex(usb, HAL_HANDLE(HAL_USB, USB_HANDLE_DEVICE), &uo, sizeof(USB_OPEN));

    HANDLE rx_stream = get(cdc, IPC_GET_RX_STREAM, 0, 0, 0);
    return stream_open(rx_stream);
}

void dac_on(HANDLE core, HANDLE analog)
{
    //TE pin enable power
    gpio_enable_pin(C13, PIN_MODE_OUT);
    gpio_set_pin(C13, false);

    STM32_DAC_ENABLE de;
    de.value = 1000;
    de.flags = DAC_FLAGS_TIMER;
    de.timer = TIM_6;
    fopen_ex(analog, STM32_DAC1, &de, sizeof(STM32_DAC_ENABLE));
}

void dac_test(HANDLE analog)
{
    HANDLE block = block_create(1024);
    int i;
    uint32_t *ptr = block_open(block);
    for (i = 0; i < 1024; ++i)
        ptr[i] = 0xfff * (i & 1);
    fwrite(analog, STM32_DAC1, block, 1024);
    fwrite(analog, STM32_DAC1, block, 1024);
}

void app()
{
    HANDLE core, analog, usb_rx;

    core = process_create(&__STM32_CORE);
#if !(MONOLITH_UART)
    HANDLE uart = process_create(&__STM32_UART);
#endif

    TIME uptime;
    int i;
    unsigned int diff;
    open_stdout();
    open_stdin();

    printf("App started\n\r");
    wdt_kick();

    char c;

#if (MONOLITH_ANALOG)
    analog = core;
#else
    analog = process_create(&__STM32_ANALOG);
#endif
    fopen(analog, HAL_HANDLE(HAL_ADC, 0));

    usb_rx = usb_on(core);

    //first second signal may go faster
    sleep_ms(1000);
    wdt_kick();

    get_uptime(&uptime);
    for (i = 0; i < TEST_ROUNDS; ++i)
        svc_test();
    wdt_kick();
    diff = time_elapsed_us(&uptime);
    printf("average kernel call time: %d.%dus\n\r", diff / TEST_ROUNDS, (diff / (TEST_ROUNDS / 10)) % 10);

    get_uptime(&uptime);
    for (i = 0; i < TEST_ROUNDS; ++i)
        process_switch_test();
    wdt_kick();
    diff = time_elapsed_us(&uptime);
    printf("average switch time: %d.%dus\n\r", diff / TEST_ROUNDS, (diff / (TEST_ROUNDS / 10)) % 10);

    process_create(&test3);

    ack(core, IPC_GET_INFO, 0, 0, 0);
    //uart info in direct mode. Make sure all data is sent before
    sleep_ms(50);
#if !(MONOLITH_UART)
    ack(uart, IPC_GET_INFO, 0, 0, 0);
#endif

    dac_on(core, analog);
    dac_test(analog);

    int temp = get(analog, STM32_ADC_TEMP, 0, 0, 0);
    printf("Temp (raw): %d.%d\n\r", temp / 10, temp % 10);

    for (;;)
    {
        stream_read(usb_rx, &c, 1);
        putc(c);
    }
}
