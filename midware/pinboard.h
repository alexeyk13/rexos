/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef PINBOARD_H
#define PINBOARD_H

#include "../userspace/process.h"
#include "../userspace/sys.h"
#include "../userspace/ipc.h"

typedef enum {
    PINBOARD_KEY_DOWN = IPC_USER,
    PINBOARD_KEY_UP,
    PINBOARD_KEY_PRESS,
    PINBOARD_KEY_LONG_PRESS,
    PINBOARD_GET_KEY_STATE,

    PINBOARD_MAX
}PINBOARD_IPCS;

#define PINBOARD_FLAG_PULL                              (1 << 0)
#define PINBOARD_FLAG_INVERTED                          (1 << 1)

#define PINBOARD_FLAG_DOWN_EVENT                        (1 << 2)
#define PINBOARD_FLAG_UP_EVENT                          (1 << 3)
#define PINBOARD_FLAG_PRESS_EVENT                       (1 << 4)
#define PINBOARD_FLAG_LONG_PRESS_EVENT                  (1 << 5)

extern const REX __PINBOARD;

#endif // PINBOARD_H
