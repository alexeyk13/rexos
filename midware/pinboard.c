/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "pinboard.h"
#include "../userspace/array.h"
#include "../userspace/time.h"
#include "../userspace/timer.h"
#include "../userspace/sys.h"
#include "../userspace/gpio.h"
#include "sys_config.h"
#if (SYS_INFO)
#include "../../userspace/stdio.h"
#endif //SYS_INFO

typedef struct {
    HANDLE process;
    unsigned int pin;
    unsigned int mode;
    unsigned int long_ms;
    TIME press_time;
    bool pressed, long_press;
} KEY;

void pinboard();

const REX __PINBOARD = {
    //name
    "Pinboard",
    //size
    PINBOARD_PROCESS_SIZE,
    //priority - midware priority
    151,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    3,
    //function
    pinboard
};

#define KEYS_COUNT(pins)                            (array_size(pins) / sizeof(KEY))
#define KEY_GET(pins, i)                            (((KEY*)array_data(pins))[i])

static inline void pinboard_event(KEY* key, unsigned int event)
{
    IPC ipc;
    ipc.process = key->process;
    ipc.cmd = event;
    ipc.param1 = HAL_HANDLE(HAL_PINBOARD, key->pin);
    ipc_post(&ipc);
}

void poll_key(KEY* key)
{
    bool pressed = gpio_get_pin(key->pin);
    if (key->mode & PINBOARD_FLAG_INVERTED)
        pressed = !pressed;
    if (pressed != key->pressed)
    {
        key->pressed = pressed;
        if (pressed)
        {
            if (key->mode & PINBOARD_FLAG_DOWN_EVENT)
                pinboard_event(key, PINBOARD_KEY_DOWN);
            get_uptime(&key->press_time);
            key->long_press = false;
        }
        else
        {
            if (key->mode & PINBOARD_FLAG_UP_EVENT)
                pinboard_event(key, PINBOARD_KEY_UP);
            if (!key->long_press && (key->mode & PINBOARD_FLAG_PRESS_EVENT))
                pinboard_event(key, PINBOARD_KEY_PRESS);
        }
    }
    else if (pressed && !key->long_press && key->long_ms && time_elapsed_ms(&key->press_time) >= key->long_ms)
    {
        pinboard_event(key, PINBOARD_KEY_LONG_PRESS);
        key->long_press = true;
    }
}

static inline void pinboard_poll(ARRAY** pins)
{
    int i;
    for (i = 0; i < KEYS_COUNT(*pins); ++i)
        poll_key(&KEY_GET(*pins, i));
}

static inline void pinboard_open(ARRAY** pins, unsigned int pin, unsigned int mode, unsigned int long_ms, HANDLE process)
{
    int i;
    KEY* key;
    for (i = 0; i < KEYS_COUNT(*pins); ++i)
        if (KEY_GET(*pins, i).pin == pin)
            error(ERROR_ALREADY_CONFIGURED);
    if (array_add(pins, sizeof(KEY)) == NULL)
        return;
    if (mode & PINBOARD_FLAG_PULL)
    {
        if (mode & PINBOARD_FLAG_INVERTED)
            gpio_enable_pin(pin, GPIO_MODE_IN_PULLDOWN);
        else
            gpio_enable_pin(pin, GPIO_MODE_IN_PULLUP);
    }
    else
        gpio_enable_pin(pin, GPIO_MODE_IN_FLOAT);
    key = &KEY_GET(*pins, KEYS_COUNT(*pins) - 1);
    key->pin = pin;
    key->mode = mode;
    key->long_ms = long_ms;
    key->pressed = false;
    key->process = process;
    poll_key(key);
}

static inline void pinboard_close(ARRAY** pins, unsigned int pin, HANDLE process)
{
    int i;
    for (i = 0; i < KEYS_COUNT(*pins); ++i)
        if (KEY_GET(*pins, i).pin == pin)
        {
            if (KEY_GET(*pins, i).process == process)
                array_remove(pins, i * sizeof(KEY), sizeof(KEY));
            else
                error(ERROR_ACCESS_DENIED);
            return;
        }
    error(ERROR_NOT_CONFIGURED);
}

static inline bool pinboard_get_key_state(ARRAY** pins, unsigned int pin)
{
    int i;
    for (i = 0; i < KEYS_COUNT(*pins); ++i)
        if (KEY_GET(*pins, i).pin == pin)
            return KEY_GET(*pins, i).pressed;
    error(ERROR_NOT_CONFIGURED);
    return false;
}

static inline void pinboard_init(ARRAY** pins)
{
    array_create(pins, sizeof(KEY));
}

static inline bool pinboard_request(ARRAY** pins, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
    case IPC_OPEN:
        pinboard_open(pins, HAL_ITEM(ipc->param1), ipc->param2, ipc->param3, ipc->process);
        need_post = true;
        break;
    case IPC_CLOSE:
        pinboard_close(pins, HAL_ITEM(ipc->param1), ipc->process);
        need_post = true;
        break;
    case PINBOARD_GET_KEY_STATE:
        ipc->param2 = pinboard_get_key_state(pins, HAL_ITEM(ipc->param1));
        need_post = true;
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

#if (SYS_INFO)
void sys_info(ARRAY** pins)
{
    int i;
    printf("Pinboard info\n\r\n\r");
    printf("Active keys: ");
    for (i = 0; i < KEYS_COUNT(*pins); ++i)
        printf("%d, %s ", KEY_GET(*pins, i).pin, KEY_GET(*pins, i).pressed ? "pressed" : "released");
    printf("\n\r");
}
#endif

void pinboard()
{
    ARRAY* pins;
    IPC ipc;
    pinboard_init(&pins);

    object_set_self(SYS_OBJ_PINBOARD);
#if (SYS_INFO)
    open_stdout();
#endif
    bool need_post;
    for (;;)
    {
        error(ERROR_OK);
        need_post = false;
        if (ipc_read_ms(&ipc, PINBOARD_POLL_TIME_MS, ANY_HANDLE))
        {
            switch (ipc.cmd)
            {
            case IPC_PING:
                need_post = true;
                break;
            default:
                need_post = pinboard_request(&pins, &ipc);
                break;
            }
            if (need_post)
                ipc_post_or_error(&ipc);
        }
        else
            pinboard_poll(&pins);
    }
}
