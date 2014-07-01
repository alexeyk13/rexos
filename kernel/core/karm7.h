/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KARM7_H
#define KARM7_H

#include "../../userspace/cc_macro.h"

#define SVC_MODE                    0x13
#define IRQ_MODE                    0x12
#define FIQ_MODE                    0x11
#define ABORT_MODE                  0x17
#define UNDEFINE_MODE               0x1b
#define SYS_MODE                    0x1f
#define USER_MODE                   0x10

#define I_BIT                       0x80
#define F_BIT                       0x40

//reset it's not common for ARM7 core, so, it must be defined int vendor-specific implementation.

#endif // KARM7_H
