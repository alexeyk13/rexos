/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef KIO_H
#define KIO_H

#include "kipc.h"
#include "kprocess.h"
#include "../userspace/dlist.h"
#include "../userspace/io.h"
#include "dbg.h"
#include <stdbool.h>

IO* kio_create(unsigned int size);
void kio_destroy(IO* io);

//internally called from kipc
bool kio_send(HANDLE process, IO* io, HANDLE receiver);

#endif // KIO_H
