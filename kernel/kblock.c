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
    if (block->open)
        kprocess_block_close(block->granted, block->data);
    kfree(block->data);
    kfree(block);
}

void kblock_open(BLOCK* block, void **ptr)
{
    bool open;
    PROCESS* process = kprocess_get_current();
    CHECK_HANDLE(block, sizeof(BLOCK));
    CHECK_MAGIC(block, MAGIC_BLOCK);
    CHECK_ADDRESS(process, ptr, sizeof(void*));

    if (block->granted == process)
    {
        disable_interrupts();
        open = block->open;
        block->open = true;
        enable_interrupts();
        if (!open)
            kprocess_block_open(process, block->data, block->size);
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
    bool open;
    PROCESS* process = kprocess_get_current();
    CHECK_HANDLE(block, sizeof(BLOCK));
    CHECK_MAGIC(block, MAGIC_BLOCK);
    if (block->granted == process)
    {
        disable_interrupts();
        open = block->open;
        block->open = false;
        enable_interrupts();
        if (open)
            kprocess_block_close(process, block->data);
    }
    else
        kprocess_error(process, ERROR_ACCESS_DENIED);
}

void kblock_send(BLOCK* block, PROCESS* receiver)
{
    bool open;
    PROCESS* process = kprocess_get_current();
    CHECK_HANDLE(block, sizeof(BLOCK));
    CHECK_HANDLE(receiver, sizeof(PROCESS));
    CHECK_MAGIC(block, MAGIC_BLOCK);
    if (block->granted == process)
    {
        disable_interrupts();
        open = block->open;
        block->open = false;
        enable_interrupts();
        if (open)
            kprocess_block_close(process, block->data);
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
    bool open;
    PROCESS* process = kprocess_get_current();
    CHECK_HANDLE(block, sizeof(BLOCK));
    CHECK_MAGIC(block, MAGIC_BLOCK);
    if (block->owner == process)
    {
        disable_interrupts();
        open = block->open;
        block->open = false;
        enable_interrupts();
        if (open)
            kprocess_block_close(process, block->data);
        block->granted = block->owner;
    }
    else
        kprocess_error(process, ERROR_ACCESS_DENIED);
}
