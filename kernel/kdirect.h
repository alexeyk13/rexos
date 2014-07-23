/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KDIRECT_H
#define KDIRECT_H

#include "kprocess.h"

//called from process
void kdirect_init(PROCESS* process);

//called from svc
void kdirect_read(PROCESS* other, void* addr, int size);
void kdirect_write(PROCESS* other, void* addr, int size);

#endif // KDIRECT_H
