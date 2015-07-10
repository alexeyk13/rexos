/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "pinboard.h"
#include "../userspace/array.h"
#include "../userspace/time.h"
#include "../userspace/systime.h"
#include "../userspace/sys.h"
#include "../userspace/gpio.h"
#include "sys_config.h"

typedef struct {
    HANDLE process;
    unsigned int pin;
    unsigned int mode;
    unsigned int long_ms;
    SYSTIME press_time;
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

#define KEY_GET(pins, i)                            ((KEY*)array_at((pins), (i)))

static void pinboard_event(KEY* key, unsigned int cmd)
{
    IPC ipc;
    ipc.process = key->process;
    ipc.cmd = cmd;
    ipc.param1 = key->pin;
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
                pinboard_event(key, HAL_CMD(HAL_PINBOARD, PINBOARD_KEY_DOWN));
            get_uptime(&key->press_time);
            key->long_press = false;
        }
        else
        {
            if (key->mode & PINBOARD_FLAG_UP_EVENT)
                pinboard_event(key, HAL_CMD(HAL_PINBOARD, PINBOARD_KEY_UP));
            if (!key->long_press && (key->mode & PINBOARD_FLAG_PRESS_EVENT))
                pinboard_event(key, HAL_CMD(HAL_PINBOARD, PINBOARD_KEY_PRESS));
        }
    }
    else if (pressed && !key->long_press && (key->mode & PINBOARD_FLAG_LONG_PRESS_EVENT) && systime_elapsed_ms(&key->press_time) >= key->long_ms)
    {
        pinboard_event(key, HAL_CMD(HAL_PINBOARD, PINBOARD_KEY_LONG_PRESS));
        key->long_press = true;
    }
}

static inline void pinboard_poll(ARRAY** pins)
{
    int i;
    for (i = 0; i < array_size(*pins); ++i)
        poll_key(KEY_GET(*pins, i));
}

static inline void pinboard_open(ARRAY** pins, unsigned int pin, unsigned int mode, unsigned int long_ms, HANDLE process)
{
    int i;
    KEY* key;
    for (i = 0; i < array_size(*pins); ++i)
        if (KEY_GET(*pins, i)->pin == pin)
            error(ERROR_ALREADY_CONFIGURED);
    if (array_append(pins) == NULL)
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
    key = KEY_GET(*pins, array_size(*pins) - 1);
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
    for (i = 0; i < array_size(*pins); ++i)
        if (KEY_GET(*pins, i)->pin == pin)
        {
            if (KEY_GET(*pins, i)->process == process)
                array_remove(pins, i);
            else
                error(ERROR_ACCESS_DENIED);
            return;
        }
    error(ERROR_NOT_CONFIGURED);
}

static inline bool pinboard_get_key_state(ARRAY** pins, unsigned int pin)
{
    int i;
    for (i = 0; i < array_size(*pins); ++i)
        if (KEY_GET(*pins, i)->pin == pin)
            return KEY_GET(*pins, i)->pressed;
    error(ERROR_NOT_CONFIGURED);
    return false;
}

static inline void pinboard_init(ARRAY** pins)
{
    array_create(pins, sizeof(KEY), 1);
}

static inline bool pinboard_request(ARRAY** pins, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        pinboard_open(pins, ipc->param1, ipc->param2, ipc->param3, ipc->process);
        need_post = true;
        break;
    case IPC_CLOSE:
        pinboard_close(pins, ipc->param1, ipc->process);
        need_post = true;
        break;
    case PINBOARD_GET_KEY_STATE:
        ipc->param2 = pinboard_get_key_state(pins, ipc->param1);
        need_post = true;
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

void pinboard()
{
    ARRAY* pins;
    IPC ipc;
    pinboard_init(&pins);

    object_set_self(SYS_OBJ_PINBOARD);
    bool need_post;
    for (;;)
    {
        error(ERROR_OK);
        need_post = false;
        if (ipc_read_ms(&ipc, PINBOARD_POLL_TIME_MS, ANY_HANDLE))
        {
            if (ipc.cmd == HAL_CMD(HAL_SYSTEM, IPC_PING))
                need_post = true;
            else
                switch (HAL_GROUP(ipc.cmd))
                {
                case HAL_PINBOARD:
                    need_post = pinboard_request(&pins, &ipc);
                    break;
                default:
                    error(ERROR_NOT_SUPPORTED);
                    need_post = true;
                }
            if (need_post)
                ipc_post_or_error(&ipc);
        }
        else
            pinboard_poll(&pins);
    }
}
