/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KERROR_H
#define KERROR_H

#include "../userspace/error.h"

int kget_last_error();
void kerror(int kerror);

#endif // KERROR_H
