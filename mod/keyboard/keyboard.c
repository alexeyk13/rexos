/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "keyboard.h"
#include "mem_private.h"
#include "../../rexos/userspace/queue.h"
#include "event.h"
#include "thread.h"
#include "dbg.h"
#include "string.h"
#include "kernel_config.h"
#include "../../userspace/queue.h"

const char *const KEYBOARD_TEXT =					"KEYBOARD";

typedef struct {
	unsigned int keys_count;
	unsigned int flags;
	GPIO_CLASS* keys;
	char* debouncing_keys;
	char* active_keys;
	HANDLE thread;
	HANDLE messages;
	HANDLE key_event;
}KEYBOARD;

void keyboard_thread(void* param)
{
	KEYBOARD* keyboard = (KEYBOARD*)param;
	int i;
	bool pressed, debouncing;
	for (;;)
	{
		for (i = 0; i < keyboard->keys_count; ++i)
		{
			pressed = gpio_get_pin(keyboard->keys[i]);
			if (keyboard->flags & KEYBOARD_FLAGS_ACTIVE_LO)
				pressed = !pressed;
			//debouncing?
			if (keyboard->debouncing_keys[i / 8] & (1 << (i & 7)))
			{
				keyboard->debouncing_keys[i / 8] &= ~(1 << (i & 7));
				if (pressed)
				{
					keyboard->active_keys[i / 8] |= (1 << (i & 7));
///					if (!messages_is_full(keyboard->messages))
///						messages_post_ms(keyboard->messages, KEY_SET_VALID(KEY_SET_PRESSED(i)), INFINITE);
					event_set(keyboard->key_event);
				}
			}
			//start debouncing
			else if (pressed)
			{
				if ((keyboard->active_keys[i / 8] & (1 << (i & 7))) == 0)
				{
					keyboard->debouncing_keys[i / 8] |= (1 << (i & 7));
					debouncing = true;
				}
			}
			//key released
			else if (keyboard->active_keys[i / 8] & (1 << (i & 7)))
			{
				keyboard->active_keys[i / 8] &= ~(1 << (i & 7));
///				if (!messages_is_full(keyboard->messages))
///					messages_post_ms(keyboard->messages, KEY_SET_VALID(i), INFINITE);
				event_set(keyboard->key_event);
			}
		}
		sleep_ms(debouncing ? KEYBOARD_DEBOUNCE_MS : KEYBOARD_POLL_MS);
	}
}

HANDLE keyboard_create(KEYBOARD_CREATE_PARAMS *params, unsigned int priority)
{
	if (params->keys_count == 0)
	  return NULL;
	unsigned int key_buf_size = params->keys_count / 8;
	if (key_buf_size & 7)
		++key_buf_size;
	KEYBOARD* keyboard = (KEYBOARD*)sys_alloc(sizeof(KEYBOARD) + params->keys_count * sizeof(GPIO_CLASS) + key_buf_size * 2);
	if (keyboard)
	{
		keyboard->keys_count = params->keys_count;
		keyboard->flags = params->flags;
		keyboard->keys = (GPIO_CLASS*)((char*)keyboard + sizeof(KEYBOARD));
		memcpy(keyboard->keys, params->keys, keyboard->keys_count * sizeof(GPIO_CLASS));
		keyboard->debouncing_keys = ((char*)keyboard->keys + params->keys_count * sizeof(GPIO_CLASS));
		keyboard->active_keys = keyboard->debouncing_keys + key_buf_size;
		memset(keyboard->debouncing_keys, 0, key_buf_size);
		memset(keyboard->active_keys, 0, key_buf_size);
		int i;
		for (i = 0; i < keyboard->keys_count; ++i)
			gpio_enable_pin(keyboard->keys[i], keyboard->flags & KEYBOARD_FLAGS_PULL ? keyboard->flags & KEYBOARD_FLAGS_ACTIVE_LO ? PIN_MODE_IN_PULLUP : PIN_MODE_IN_PULLDOWN : PIN_MODE_IN);
		keyboard->messages = messages_create(params->queue_size);
		keyboard->key_event = event_create();
		keyboard->thread = thread_create_and_run(KEYBOARD_TEXT, 32, priority, keyboard_thread, keyboard);
	}
	else
		error(ERROR_MEM_OUT_OF_SYSTEM_MEMORY, KEYBOARD_TEXT);
	return (HANDLE)keyboard;
}

void keyboard_destroy(HANDLE handle)
{
	KEYBOARD* keyboard = (KEYBOARD*)handle;
	thread_destroy(keyboard->thread);
	event_destroy(keyboard->key_event);
///	messages_destroy(keyboard->messages);
	int i;
	for (i = 0; i < keyboard->keys_count; ++i)
		gpio_disable_pin(keyboard->keys[i]);
	sys_free(keyboard);
}

bool keyboard_is_pressed(HANDLE handle, KEY key)
{
	KEYBOARD* keyboard = (KEYBOARD*)handle;
	return (keyboard->active_keys[KEY_ROW(key) / 8] & (1 << (KEY_ROW(key) & 7))) ? true : false;
}

bool keyboard_has_messages(HANDLE handle)
{
///	KEYBOARD* keyboard = (KEYBOARD*)handle;
///	return !messages_is_empty(keyboard->messages);
    return 1;///!!!
}

KEY keyboard_read(HANDLE handle, unsigned int timeout_ms)
{
	KEYBOARD* keyboard = (KEYBOARD*)handle;
	return (KEY)messages_peek_ms(keyboard->messages, timeout_ms);
}

bool keyboard_wait_for_key(HANDLE handle, unsigned int timeout_ms)
{
	KEYBOARD* keyboard = (KEYBOARD*)handle;
	if (!keyboard_has_messages(handle))
	{
		event_clear(keyboard->key_event);
		return event_wait_ms(keyboard->key_event, timeout_ms);
	}
	return true;
}
