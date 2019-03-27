/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef APP_PRIVATE_H
#define APP_PRIVATE_H

#include "app.h"
#include "comm.h"
#include "net.h"
#include <stdint.h>

typedef struct _APP {
    COMM comm;
    NET net;
    HANDLE timer;
    HANDLE usbd;
} APP;

#endif // APP_PRIVATE_H
