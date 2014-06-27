/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SYS_CALL_H
#define SYS_CALL_H

void sys_handler_direct(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3);
void sys_handler(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3);

#endif // SYS_CALL_H
