/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef KEXO_H
#define KEXO_H

#include "../userspace/io.h"

//interface for exodrivers
void kexo_post(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3);


#define kexo_io(process, cmd, handle, io)                                   kexo_post((process), (cmd), (handle), (unsigned int)(io), (io)->data_size)
#define kexo_io_ex(process, cmd, handle, io, param3)                        kexo_post((process), (cmd), (handle), (unsigned int)(io), (unsigned int)(param3))

#endif // KEXO_H
