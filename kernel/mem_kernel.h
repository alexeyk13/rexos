/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MEM_KERNEL_H
#define MEM_KERNEL_H

//system memory pool. Only in supervisor context
void* sys_alloc(int size);
void* sys_alloc_aligned(int size, int align);
void sys_free(void* ptr);

//allocate stack for thread. Only in supervisor context
void* stack_alloc(int size);
void stack_free(void* ptr);

void svc_mem_handler(unsigned int num);

#endif // MEM_KERNEL_H
