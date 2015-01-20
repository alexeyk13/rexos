/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "kblock.h"
#include "kernel.h"
#include "kmalloc.h"
#include "kipc.h"
#include "kprocess.h"

void kblock_create(BLOCK** block, unsigned int size)
{
    PROCESS* process = kprocess_get_current();
    CHECK_ADDRESS(process, block, sizeof(void*));
    *block = kmalloc(sizeof(BLOCK));
    if (*block != NULL)
    {
        DO_MAGIC((*block), MAGIC_BLOCK);
        (*block)->owner = (*block)->granted = process;
        (*block)->size = size;
        (*block)->open = false;
        //allocate stream data
        (*block)->data = kmalloc(size);
        if ((*block)->data == NULL)
        {
            kfree(*block);
            (*block) = NULL;
            kprocess_error(process, ERROR_OUT_OF_PAGED_MEMORY);
        }
    }
    else
        kprocess_error(process, ERROR_OUT_OF_SYSTEM_MEMORY);
}

void kblock_destroy(BLOCK* block)
{
    if ((HANDLE)block == INVALID_HANDLE)
        return;
    CHECK_HANDLE(block, sizeof(BLOCK));
    CHECK_MAGIC(block, MAGIC_BLOCK);
    CLEAR_MAGIC(block);
    disable_interrupts();
    if (block->open)
        kprocess_block_close(block->granted, block);
    enable_interrupts();
    kfree(block->data);
    kfree(block);
}

static inline void kblock_open_internal(BLOCK* block, PROCESS* process)
{
    disable_interrupts();
    if (!block->open)
    {
        block->open = true;
        kprocess_block_open(process, block);
    }
    enable_interrupts();
}

static inline void kblock_close_internal(BLOCK* block, PROCESS* process)
{
    disable_interrupts();
    if (block->open)
    {
        kprocess_block_close(process, block);
        block->open = false;
    }
    enable_interrupts();
}

void kblock_open(BLOCK* block, void **ptr)
{
    PROCESS* process = kprocess_get_current();
    CHECK_HANDLE(block, sizeof(BLOCK));
    CHECK_MAGIC(block, MAGIC_BLOCK);
    CHECK_ADDRESS(process, ptr, sizeof(void*));

    if (block->granted == process)
    {
        kblock_open_internal(block, process);
        *ptr = block->data;
    }
    else
    {
        kprocess_error(process, ERROR_ACCESS_DENIED);
        *ptr = NULL;
    }
}

void kblock_close(BLOCK* block)
{
    PROCESS* process = kprocess_get_current();
    CHECK_HANDLE(block, sizeof(BLOCK));
    CHECK_MAGIC(block, MAGIC_BLOCK);
    if (block->granted == process)
        kblock_close_internal(block, process);
    else
        kprocess_error(process, ERROR_ACCESS_DENIED);
}

void kblock_get_size(BLOCK* block, unsigned int* size)
{
    PROCESS* process = kprocess_get_current();
    CHECK_HANDLE(block, sizeof(BLOCK));
    CHECK_MAGIC(block, MAGIC_BLOCK);
    CHECK_ADDRESS(process, size, sizeof(unsigned int));
    if (block->granted == process)
        *size = block->size;
    else
    {
        kprocess_error(process, ERROR_ACCESS_DENIED);
        *size = 0;
    }
}

void kblock_send(BLOCK* block, PROCESS* receiver)
{
    PROCESS* process = kprocess_get_current();
    CHECK_HANDLE(block, sizeof(BLOCK));
    CHECK_HANDLE(receiver, sizeof(PROCESS));
    CHECK_MAGIC(block, MAGIC_BLOCK);
    if (block->granted == process)
    {
        kblock_close_internal(block, process);
        block->granted = receiver;
    }
    else
        kprocess_error(process, ERROR_ACCESS_DENIED);
}

void kblock_send_ipc(BLOCK* block, PROCESS* receiver, IPC* ipc)
{
    PROCESS* process = kprocess_get_current();
    kblock_send(block, receiver);
    kipc_post_process(ipc, (HANDLE)process);
}

void kblock_return(BLOCK* block)
{
    PROCESS* process = kprocess_get_current();
    CHECK_HANDLE(block, sizeof(BLOCK));
    CHECK_MAGIC(block, MAGIC_BLOCK);
    if (block->owner == process)
    {
        kblock_close_internal(block, block->granted);
        block->granted = block->owner;
    }
    else
        kprocess_error(process, ERROR_ACCESS_DENIED);
}
