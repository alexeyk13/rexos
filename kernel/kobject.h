/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KOBJECT_H
#define KOBJECT_H

#include "../userspace/types.h"

void kobject_init();
void kobject_set(HANDLE process, int idx, HANDLE handle);
HANDLE kobject_get(int idx);

#endif // KOBJECT_H
