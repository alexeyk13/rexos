/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KBLOCK_H
#define KBLOCK_H

#include "kipc.h"
#include "../userspace/lib/dlist.h"
#include "dbg.h"

typedef struct _PROCESS PROCESS;

typedef struct {
    DLIST list;
    MAGIC;
    void* data;
    unsigned int size;
    PROCESS* owner;
    PROCESS* granted;
    bool open;
}BLOCK;

//called from svc
void kblock_create(BLOCK** block, unsigned int size);
void kblock_open(BLOCK* block, void **ptr);
void kblock_close(BLOCK* block);
void kblock_send(BLOCK* block, PROCESS* receiver);
void kblock_send_ipc(BLOCK* block, PROCESS* receiver, IPC* ipc);
void kblock_return(BLOCK* block);
void kblock_destroy(BLOCK* block);

#endif // KBLOCK_H
