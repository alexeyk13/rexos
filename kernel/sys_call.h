/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SVC_H
#define SVC_H

void sys_handler_direct(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3);
void sys_handler(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3);

#endif // SVC_H
