/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "kblock.h"
#include "kernel.h"
#include "kmalloc.h"
#include "kipc.h"

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
        (*block)->index = -1;
        //allocate stream data
        if (((*block)->data = paged_alloc(size)) == NULL)
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
    if (block->index >= 0)
        kprocess_block_close(block->granted, block->index);
    paged_free(block->data);
    kfree(block);
}

void kblock_open(BLOCK* block, void **ptr)
{
    PROCESS* process = kprocess_get_current();
    CHECK_HANDLE(block, sizeof(BLOCK));
    CHECK_MAGIC(block, MAGIC_BLOCK);
    CHECK_ADDRESS(process, ptr, sizeof(void*));

    if (block->index < 0 && block->granted == process)
    {
        block->index = kprocess_block_open(process, block->data, block->size);
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
    if (block->index >= 0 && block->granted == process)
    {
        kprocess_block_close(process, block->index);
        block->index = -1;
    }
    else
        kprocess_error(process, ERROR_ACCESS_DENIED);
}

void kblock_send(BLOCK* block, PROCESS* receiver)
{
    PROCESS* process = kprocess_get_current();
    CHECK_HANDLE(block, sizeof(BLOCK));
    CHECK_HANDLE(receiver, sizeof(PROCESS));
    CHECK_MAGIC(block, MAGIC_BLOCK);
    if (block->granted == process)
    {
        //close first if open
        if (block->index >= 0)
        {
            kprocess_block_close(process, block->index);
            block->index = -1;
        }
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
    if (block->granted == process)
    {
        //close first if open
        if (block->index >= 0)
        {
            kprocess_block_close(process, block->index);
            block->index = -1;
        }
        block->granted = block->owner;
    }
    else
        kprocess_error(process, ERROR_ACCESS_DENIED);
}
