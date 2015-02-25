/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LIB_H
#define LIB_H

#include "cc_macro.h"
#include "svc.h"
#include "error.h"

typedef enum {
    LIB_ID_STD = 0,
    LIB_ID_STDIO,
    LIB_ID_TIME,
    LIB_ID_HEAP,
    LIB_ID_ARRAY,
    LIB_ID_GPIO,
    LIB_ID_GUI,
    LIB_ID_MAX
} LIB_ID;

#if (LIB_CHECK_PRESENCE)

#define LIB_CHECK(id)                                       if (__GLOBAL->lib[(id)] == NULL) { \
                                                                error(ERROR_STUB_CALLED); \
                                                                return;}

#define LIB_CHECK_RET(id)                                   if (__GLOBAL->lib[(id)] == NULL) { \
                                                                error(ERROR_STUB_CALLED); \
                                                                return 0;}

#else

#define LIB_CHECK(id)                                       ;
#define LIB_CHECK_RET(id)                                   ;

#endif

#endif // LIB_H
