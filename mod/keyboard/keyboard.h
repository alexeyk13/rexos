/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"
#include "gpio.h"

#define KEY									unsigned int
#define KEY_PRESSED(key)				((key) & (1 << 16))
#define KEY_SET_PRESSED(key)			((key) | (1 << 16))
#define KEY_VALID(key)					((key) & (1 << 17))
#define KEY_SET_VALID(key)				((key) | (1 << 17))
#define KEY_CODE(key)					((key) & 0xffff)
#define KEY_ROW(key)						((key) & 0xff)
#define KEY_LINE(key)					(((key) >> 8) & 0xff)

#define KEYBOARD_FLAGS_ACTIVE_LO		(1 << 0)
#define KEYBOARD_FLAGS_PULL			(1 << 1)

typedef struct {
	const GPIO_CLASS* keys;
	unsigned int keys_count;
	unsigned int queue_size;
	unsigned int flags;
}KEYBOARD_CREATE_PARAMS;

HANDLE keyboard_create(KEYBOARD_CREATE_PARAMS* params, unsigned int priority);
void keyboard_destroy(HANDLE handle);
bool keyboard_is_pressed(HANDLE handle, KEY key);
bool keyboard_has_messages(HANDLE handle);
KEY keyboard_read(HANDLE handle, unsigned int timeout_ms);
bool keyboard_wait_for_key(HANDLE handle, unsigned int timeout_ms);

#endif // KEYBOARD_H
