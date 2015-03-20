#include "../../rexos/userspace/process.h"
#include "../../rexos/userspace/time.h"
#include "../../rexos/userspace/stdlib.h"
#include "../../rexos/userspace/stdio.h"
#include "../../rexos/userspace/heap.h"
#include "../../rexos/userspace/timer.h"
#include "../../rexos/userspace/ipc.h"
#include "../../rexos/userspace/stream.h"
#include "../../rexos/userspace/direct.h"
#include "../../rexos/userspace/object.h"
#include "../../rexos/userspace/sys.h"
#include "../../rexos/userspace/gpio.h"
#include "../../rexos/userspace/rtc.h"
#include "../../rexos/userspace/wdt.h"
#include "../../rexos/userspace/adc.h"
#include "../../rexos/userspace/stm32_driver.h"
#include "../../rexos/drv/stm32/stm32_uart.h"
#include "../../rexos/drv/stm32/stm32_analog.h"
#include "../../rexos/userspace/file.h"
#include "comm.h"

#define TEST_ROUNDS                                 10000

void app();
void test_thread3();
void test_thread4();
void test_thread5();
void test_thread6();
void test_thread7();

const REX __APP = {
    //name
    "App main",
    //size
    512,
    //priority
    200,
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
    202,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    10,
    //function
    test_thread3
};

const REX test4 = {
    //name
    "test4",
    //size
    512,
    //priority
    203,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    10,
    //function
    test_thread4
};

const REX test5 = {
    //name
    "test5",
    //size
    512,
    //priority
    204,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    10,
    //function
    test_thread5
};

const REX test6 = {
    //name
    "test6",
    //size
    512,
    //priority
    205,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    10,
    //function
    test_thread6
};

const REX test7 = {
    //name
    "test7",
    //size
    512,
    //priority
    206,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    10,
    //function
    test_thread7
};


void test_thread4()
{
    for (;;)
    {
        sleep_us(123);
    }
}

void test_thread5()
{
    for (;;)
    {
        sleep_us(456);
    }
}

void test_thread6()
{
    for (;;)
    {
        sleep_us(789);
    }
}

void test_thread7()
{
    for (;;)
    {
        sleep_us(1012);
    }
}

void test_thread3()
{
    TIME uptime;
    open_stdout();
    printf("test thread3 started\n\r");
    printf("my name is: %s\n\r", process_name());

    gpio_enable_pin(B9, GPIO_MODE_OUT);

    wdt_kick();
    process_info();

    process_create(&test4);
    process_create(&test5);
    process_create(&test6);
    process_create(&test7);

    bool set = true;
    for (;;)
    {
        get_uptime(&uptime);
        printf("uptime: %02d:%02d.%06d\n\r", uptime.sec / 60, uptime.sec % 60, uptime.usec);
        printf("rtc: %d\n\r", rtc_get());
        set ? gpio_set_pin(B9) : gpio_reset_pin(B9);
        wdt_kick();
        set = !set;
        sleep_ms(1700);
        process_info();
    }
}


void dac_on(HANDLE analog)
{
    //TE pin enable power
    gpio_enable_pin(C13, GPIO_MODE_OUT);
    gpio_reset_pin(C13);

    STM32_DAC_ENABLE de;
    de.value = 1000;
    de.flags = DAC_FLAGS_TIMER;
    de.timer = TIM_6;
    fopen_ex(analog, HAL_HANDLE(HAL_DAC, STM32_DAC1), 0, &de, sizeof(STM32_DAC_ENABLE));
}

void dac_test(HANDLE analog)
{
    HANDLE block = block_create(256);
    int i;
    uint32_t *ptr = block_open(block);
    for (i = 0; i < 256 / 4; ++i)
        ptr[i] = 0xfff * (i & 1);
    fwrite(analog, HAL_HANDLE(HAL_DAC, STM32_DAC1), block, 256);
    fwrite(analog, HAL_HANDLE(HAL_DAC, STM32_DAC1), block, 256);
    fwrite(analog, HAL_HANDLE(HAL_DAC, STM32_DAC1), block, 256);
    fwrite(analog, HAL_HANDLE(HAL_DAC, STM32_DAC1), block, 256);
    fwrite(analog, HAL_HANDLE(HAL_DAC, STM32_DAC1), block, 256);
    fwrite(analog, HAL_HANDLE(HAL_DAC, STM32_DAC1), block, 256);
}

void app()
{
    HANDLE core, analog;

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

#if (MONOLITH_ANALOG)
    analog = core;
#else
    analog = process_create(&__STM32_ANALOG);
#endif
    fopen(analog, HAL_HANDLE(HAL_ADC, 0), 0);


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

    process_create(&__COMM);

    dac_on(analog);
    dac_test(analog);

    fopen(object_get(SYS_OBJ_ADC), HAL_HANDLE(HAL_ADC, STM32_ADC_DEVICE), 0);
    fopen(object_get(SYS_OBJ_ADC), HAL_HANDLE(HAL_ADC, STM32_ADC_TEMP), STM32_ADC_SMPR_239_5);
    int temp = stm32_adc_temp(3300, 12);
    printf("Temp: %d.%d\n\r", temp / 10, temp % 10);

    for (;;)
    {
        sleep_ms(1000);
    }
}
