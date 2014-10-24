/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LIB_H
#define LIB_H

#include "types.h"
#include "kernel_config.h"

typedef struct {
    //stdlib.h
    const void* const p_lib_std;
    //sdtio.h
    const void* const p_lib_stdio;
    //time.h
    const void* const p_lib_time;
    //array.h
    const void* const p_lib_array;
} LIB;

void lib_stub ();

#endif // LIB_H
