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

bool comm_usbd_alert(APP* app, USBD_ALERTS alert);
void comm_usbd_stream_rx(APP* app, unsigned int size);

void comm_init(APP* app);

#endif // COMM_H
