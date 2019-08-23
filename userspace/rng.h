/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RL (jam_roma@yahoo.com)
*/

#ifndef _RNG_H_
#define _RNG_H_

#include "ipc.h"
#include "io.h"
#include "cc_macro.h"

__STATIC_INLINE int rng_get(IO* io, unsigned int size)
{
    return io_read_sync_exo(HAL_IO_REQ(HAL_RNG, IPC_READ), 0, io, size);
}

#endif /* _RNG_H_ */
