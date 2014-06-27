/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "core.h"
#include "sys_calls.h"

//TODO: refactor this
void sys_call(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3)
{
    if (get_context() <= SUPERVISOR_CONTEXT)
        //raise context
        do_sys_call(num, param1, param2, param3);
    else
        //enough context to call directly
        ((GLOBAL*)(SRAM_BASE))->sys_handler_direct(num, param1, param2, param3);
}
