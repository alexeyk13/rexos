/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KOBJECT_H
#define KOBJECT_H

#include "../userspace/lib/types.h"

void kobject_init();
void kobject_set(int idx, HANDLE handle);
void kobject_get(int idx, HANDLE* handle);

#endif // KOBJECT_H
