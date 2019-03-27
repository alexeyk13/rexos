/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#include "kerror.h"
#include "kernel.h"

int kget_last_error()
{
    return __KERNEL->kerror;
}

void kerror(int kerror)
{
    __disable_irq();
    __KERNEL->kerror = kerror;
    __enable_irq();
}
