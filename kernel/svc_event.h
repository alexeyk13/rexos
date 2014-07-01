/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SVC_EVENT_H
#define SVC_EVENT_H

#include "svc_process.h"
#include "dbg.h"

typedef struct {
    MAGIC;
    bool set;
    PROCESS* waiters;
}EVENT;

//called from process_private.c on destroy or timeout
void svc_event_lock_release(EVENT* event, PROCESS* process);

//called from svc
void svc_event_create(EVENT** event);
void svc_event_pulse(EVENT* event);
void svc_event_set(EVENT* event);
void svc_event_is_set(EVENT* event, bool* is_set);
void svc_event_clear(EVENT* event);
void svc_event_wait(EVENT* event, TIME* time);
void svc_event_destroy(EVENT* event);

#endif // SVC_EVENT_H
