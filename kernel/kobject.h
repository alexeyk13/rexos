/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef KOBJECT_H
#define KOBJECT_H

#include "../userspace/types.h"

void kobject_init();
void kobject_set(HANDLE process, int idx, HANDLE handle);
HANDLE kobject_get(int idx);

#endif // KOBJECT_H
