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
    CHECK_HANDLE(block, sizeof(BLOCK));
    CHECK_MAGIC(block, MAGIC_BLOCK);
    CLEAR_MAGIC(block);
    if (block->index >= 0)
        kprocess_block_close(block->granted, block->index);
    paged_free(block->data);
    kfree(block);
}

void kblock_open(BLOCK* block)
{
    PROCESS* process = kprocess_get_current();
    CHECK_HANDLE(block, sizeof(BLOCK));
    CHECK_MAGIC(block, MAGIC_BLOCK);
    if (block->index < 0 && block->granted == process)
        block->index = kprocess_block_open(process, block->data, block->size);
    else
        kprocess_error(process, ERROR_ACCESS_DENIED);
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
    IPC ipc;
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

        ipc.process = (HANDLE)receiver;
        ipc.cmd = IPC_BLOCK_SENT;
        ipc.param1 = (HANDLE)block;
        ipc.param2 = (unsigned int)block->data;
        ipc.param3 = (unsigned int)block->size;
        kipc_post_process(&ipc, INVALID_HANDLE);
    }
    else
        kprocess_error(process, ERROR_ACCESS_DENIED);
}

void kblock_return(BLOCK* block)
{
    IPC ipc;
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

        ipc.process = (HANDLE)block->owner;
        ipc.cmd = IPC_BLOCK_SENT;
        ipc.param1 = (HANDLE)block;
        ipc.param2 = (unsigned int)block->data;
        ipc.param3 = (unsigned int)block->size;
        kipc_post_process(&ipc, INVALID_HANDLE);
    }
    else
        kprocess_error(process, ERROR_ACCESS_DENIED);
}
