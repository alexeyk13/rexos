/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LIB_H
#define LIB_H

#include "svc.h"
#include "error.h"

typedef enum {
    LIB_ID_STD = 0,
    LIB_ID_STDIO,
    LIB_ID_SYSTIME,
    LIB_ID_ARRAY,
    LIB_ID_SO,
    LIB_ID_MAX
} LIB_ID;

#endif // LIB_H
