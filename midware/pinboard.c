/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
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

typedef struct {
    ARRAY* pins;
    HANDLE timer;
} PINBOARD;

void pinboard();

const REX __PINBOARD = {
    //name
    "Pinboard",
    //size
    PINBOARD_PROCESS_SIZE,
    //priority - midware priority
    151,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_FLAG_PERSISTENT_NAME,
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

static inline void pinboard_poll(PINBOARD* pinboard)
{
    int i;
    for (i = 0; i < array_size(pinboard->pins); ++i)
        poll_key(KEY_GET(pinboard->pins, i));
    timer_start_ms(pinboard->timer, PINBOARD_POLL_TIME_MS);
}

static inline void pinboard_open(PINBOARD* pinboard, unsigned int pin, unsigned int mode, unsigned int long_ms, HANDLE process)
{
    int i;
    KEY* key;
    for (i = 0; i < array_size(pinboard->pins); ++i)
        if (KEY_GET(pinboard->pins, i)->pin == pin)
            error(ERROR_ALREADY_CONFIGURED);
    if (array_append(&pinboard->pins) == NULL)
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
    key = KEY_GET(pinboard->pins, array_size(pinboard->pins) - 1);
    key->pin = pin;
    key->mode = mode;
    key->long_ms = long_ms;
    key->pressed = false;
    key->process = process;
    poll_key(key);
}

static inline void pinboard_close(PINBOARD* pinboard, unsigned int pin, HANDLE process)
{
    int i;
    for (i = 0; i < array_size(pinboard->pins); ++i)
        if (KEY_GET(pinboard->pins, i)->pin == pin)
        {
            if (KEY_GET(pinboard->pins, i)->process == process)
                array_remove(&pinboard->pins, i);
            else
                error(ERROR_ACCESS_DENIED);
            return;
        }
    error(ERROR_NOT_CONFIGURED);
}

static inline bool pinboard_get_key_state(PINBOARD* pinboard, unsigned int pin)
{
    int i;
    for (i = 0; i < array_size(pinboard->pins); ++i)
        if (KEY_GET(pinboard->pins, i)->pin == pin)
            return KEY_GET(pinboard->pins, i)->pressed;
    error(ERROR_NOT_CONFIGURED);
    return false;
}

static inline void pinboard_init(PINBOARD* pinboard)
{
    array_create(&pinboard->pins, sizeof(KEY), 1);
    pinboard->timer = timer_create(0, HAL_PINBOARD);
    timer_start_ms(pinboard->timer, PINBOARD_POLL_TIME_MS);
}

static inline void pinboard_request(PINBOARD* pinboard, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        pinboard_open(pinboard, ipc->param1, ipc->param2, ipc->param3, ipc->process);
        break;
    case IPC_CLOSE:
        pinboard_close(pinboard, ipc->param1, ipc->process);
        break;
    case PINBOARD_GET_KEY_STATE:
        ipc->param2 = pinboard_get_key_state(pinboard, ipc->param1);
        break;
    case IPC_TIMEOUT:
        pinboard_poll(pinboard);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

void pinboard()
{
    IPC ipc;
    PINBOARD pinboard;
    pinboard_init(&pinboard);

    object_set_self(SYS_OBJ_PINBOARD);
    for (;;)
    {
        ipc_read(&ipc);
        pinboard_request(&pinboard, &ipc);
        ipc_write(&ipc);
    }
}
