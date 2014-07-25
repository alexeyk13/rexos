/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KBLOCK_H
#define KBLOCK_H

#include "kprocess.h"

typedef struct {
    MAGIC;
    void* data;
    unsigned int size;
    PROCESS* owner;
    PROCESS* granted;
    int index;
}BLOCK;

//called from svc
void kblock_create(BLOCK** block, unsigned int size);
void kblock_open(BLOCK* block);
void kblock_close(BLOCK* block);
void kblock_send(BLOCK* block, PROCESS* receiver);
void kblock_return(BLOCK* block);
void kblock_destroy(BLOCK* block);


#endif // KBLOCK_H
