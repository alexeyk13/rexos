/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef COMM_H
#define COMM_H

// USB communication process

#include "../../rexos/userspace/sys.h"
#include "../../rexos/userspace/usb.h"
#include <stdbool.h>
#include "app.h"

typedef struct {
    HANDLE tx, rx, rx_stream;
    bool active;
}COMM;

void comm_init(APP* app);
void comm_request(APP* app, IPC* ipc);

#endif // COMM_H
