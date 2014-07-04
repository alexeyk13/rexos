/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

/*
    init.c - default init file for RExOS SYS. Kernel can be compiled independently, thus totally rewritting following file
 */

#include "sys_config.h"
#include "../userspace/process.h"
#include "../userspace/core/core.h"

#if (SYS_APP)
extern const REX __APP;
#endif
extern const REX __SYS;


void init(void);

const REX __INIT = {
    //name
    "INIT",
    //size
    256,
    //priority - init priority
    ((unsigned int)-1),
    //flags - init must be called frozen)
    0,
    //ipc size - no ipc
    0,
    //function
    init
};

void init()
{
    //start the system
    process_create(&__SYS);

    //start user application, if required
#if (SYS_APP)
    process_create(&__APP);
#endif

    for (;;)
    {
        //TODO WFE here
    }
}
