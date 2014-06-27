/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SVC_H
#define SVC_H

void svc(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3);
void svc_irq(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3);

#endif // SVC_H
