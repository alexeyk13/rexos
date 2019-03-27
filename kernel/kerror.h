/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef KERROR_H
#define KERROR_H

#include "../userspace/error.h"

int kget_last_error();
void kerror(int kerror);

#endif // KERROR_H
