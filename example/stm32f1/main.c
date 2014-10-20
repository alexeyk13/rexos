#include "../../rexos/userspace/process.h"
#include "../../rexos/userspace/lib/time.h"
#include "../../rexos/userspace/lib/stdlib.h"
#include "../../rexos/userspace/lib/stdio.h"
#include "../../rexos/userspace/timer.h"
#include "../../rexos/userspace/ipc.h"
#include "../../rexos/userspace/stream.h"
#include "../../rexos/userspace/direct.h"
#include "../../rexos/userspace/object.h"
#include "string.h"
#include "../../rexos/sys/sys.h"
#include "../../rexos/sys/drv/stm32_gpio.h"
#include "../../rexos/sys/drv/stm32_uart.h"
#include "../../rexos/sys/drv/stm32_rtc.h"
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
    PROCESS_FLAGS_ACTIVE,
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
    PROCESS_FLAGS_ACTIVE,
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
    printf("my name is: %s\n\r", __PROCESS_NAME);

    HANDLE core = object_get(SYS_OBJ_CORE);
    ack(core, STM32_GPIO_ENABLE_PIN, B9, PIN_MODE_OUT, 0);

    ack(core, STM32_WDT_KICK, 0, 0, 0);
    process_info();
    bool set = true;
    for (;;)
    {
        get_uptime(&uptime);
        printf("uptime: %02d:%02d.%06d\n\r", uptime.sec / 60, uptime.sec % 60, uptime.usec);
        printf("rtc: %d\n\r", get(core, STM32_RTC_GET, 0, 0, 0));
        ack(core, STM32_GPIO_SET_PIN, B9, set, 0);
        ack(core, STM32_WDT_KICK, 0, 0, 0);
        set = !set;
        sleep_ms(700);
    }
}

void te_usb_disable_power()
{
    HANDLE core = object_get(SYS_OBJ_CORE);
    ack(core, STM32_GPIO_ENABLE_PIN, C9, PIN_MODE_OUT, 0);
    ack(core, STM32_GPIO_SET_PIN, C9, true, 0);
}

HANDLE usb_on()
{
    HANDLE usb, usbd, cdc;
    CDC_OPEN_STRUCT cos;
    te_usb_disable_power();
    usb = process_create(&__STM32_USB);

    //setup usbd
    usbd = process_create(&__USBD);

    fopen(usbd, usb);
    direct_enable_read(usbd, (void*)&__DESCRIPTORS, __DESCRIPTORS.header.total_size);
    ack(usbd, USBD_SETUP_DESCRIPTORS, USB_FULL_SPEED, __DESCRIPTORS.header.total_size, 0);
    direct_enable_read(usbd, (void*)&__STRINGS, __STRINGS.header.header.total_size);
    ack(usbd, USBD_SETUP_STRING_DESCRIPTORS, __STRINGS.header.header.total_size, 0, 0);

    //setup cdc
    cdc = process_create(&__CDC);
    cos.data_ep = 1;
    cos.control_ep = 2;
    cos.data_ep_size = 64;
    cos.control_ep_size = 16;
    cos.rx_stream_size = cos.tx_stream_size = 32;
    fopen_ex(cdc, usbd, (void*)&cos, sizeof(CDC_OPEN_STRUCT));

    //turn USB on
    fopen(usb, USB_HANDLE_DEVICE);

    HANDLE rx_stream = get(cdc, IPC_GET_RX_STREAM, 0, 0, 0);
    return stream_open(rx_stream);
}

void dac_on(HANDLE core, HANDLE analog)
{
    //TE pin enable power
    ack(core, STM32_GPIO_ENABLE_PIN, C13, PIN_MODE_OUT, 0);
    ack(core, STM32_GPIO_SET_PIN, C13, false, 0);

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
    HANDLE core, uart, analog, usb_rx;

    core = process_create(&__STM32_CORE);
    uart = process_create(&__STM32_UART);

    core = object_get(SYS_OBJ_CORE);
    TIME uptime;
    int i;
    unsigned int diff;
    open_stdout();
    open_stdin();

    printf("App started\n\r");
    ack(core, STM32_WDT_KICK, 0, 0, 0);

    char c;

    analog = process_create(&__STM32_ANALOG);
    fopen(analog, STM32_ADC);

    usb_rx = usb_on();

    //first second signal may go faster
    sleep_ms(1000);
    ack(core, STM32_WDT_KICK, 0, 0, 0);

    get_uptime(&uptime);
    for (i = 0; i < TEST_ROUNDS; ++i)
        svc_test();
    ack(core, STM32_WDT_KICK, 0, 0, 0);
    diff = time_elapsed_us(&uptime);
    printf("average kernel call time: %d.%dus\n\r", diff / TEST_ROUNDS, (diff / (TEST_ROUNDS / 10)) % 10);

    get_uptime(&uptime);
    for (i = 0; i < TEST_ROUNDS; ++i)
        process_switch_test();
    ack(core, STM32_WDT_KICK, 0, 0, 0);
    diff = time_elapsed_us(&uptime);
    printf("average switch time: %d.%dus\n\r", diff / TEST_ROUNDS, (diff / (TEST_ROUNDS / 10)) % 10);

    process_create(&test3);

    ack(core, IPC_GET_INFO, 0, 0, 0);
    //uart info in direct mode. Make sure all data is sent before
    sleep_ms(50);
    ack(uart, IPC_GET_INFO, 0, 0, 0);

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
