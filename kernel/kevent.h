/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KEVENT_H
#define KEVENT_H

#include "kprocess.h"
#include "dbg.h"

typedef struct {
    MAGIC;
    bool set;
    PROCESS* waiters;
}EVENT;

//called from process_private.c on destroy or timeout
void kevent_lock_release(EVENT* event, PROCESS* process);

//called from svc
void kevent_create(EVENT** event);
void kevent_pulse(EVENT* event);
void kevent_set(EVENT* event);
void kevent_is_set(EVENT* event, bool* is_set);
void kevent_clear(EVENT* event);
void kevent_wait(EVENT* event, TIME* time);
void kevent_destroy(EVENT* event);

#endif // SVC_EVENT_H
