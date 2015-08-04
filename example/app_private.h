/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef APP_PRIVATE_H
#define APP_PRIVATE_H

#include "app.h"
#include "comm.h"
#include <stdint.h>

typedef struct _APP {
    COMM comm;
    HANDLE timer;
    HANDLE usbd;
} APP;

#endif // APP_PRIVATE_H
