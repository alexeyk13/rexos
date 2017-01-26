/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
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
