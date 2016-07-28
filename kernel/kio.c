/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "kio.h"
#include "kprocess.h"
#include "kstdlib.h"
#include "kernel_config.h"

static void kio_destroy_internal(KIO* kio)
{
    CLEAR_MAGIC(kio);
    kfree(kio->io);
    kfree(kio);
}

void kio_create(IO** io, unsigned int size)
{
    KIO* kio;
    KPROCESS* kprocess = kprocess_get_current();
    CHECK_ADDRESS(kprocess, io, sizeof(void*));
    disable_interrupts();
    kio = (KIO*)kmalloc(sizeof(KIO));
    enable_interrupts();
    if (kio != NULL)
    {
        DO_MAGIC(kio, MAGIC_KIO);
        kio->owner = kio->granted = kprocess;
        kio->kill_flag = false;
        disable_interrupts();
        (*io) = kio->io = (IO*)kmalloc(size + sizeof(IO));
        enable_interrupts();
        if ((kio->io) == NULL)
        {
            disable_interrupts();
            kio_destroy_internal(kio);
            enable_interrupts();
            kprocess_error(kprocess, ERROR_OUT_OF_PAGED_MEMORY);
            return;
        }
        kio->io->kio = (HANDLE)kio;
        kio->io->size = size + sizeof(IO);
    }
    else
        kprocess_error(kprocess, ERROR_OUT_OF_SYSTEM_MEMORY);
}

bool kio_send(KIO* kio, KPROCESS* receiver)
{
    IPC ipc;
    KPROCESS* kprocess = kprocess_get_current();
    if (kprocess != kio->granted)
    {
        kprocess_error(kprocess, ERROR_ACCESS_DENIED);
        return false;
    }
    //user released IO
    if ((kio->kill_flag) && (receiver == kio->owner))
    {
        disable_interrupts();
        kio_destroy_internal(kio);
        enable_interrupts();
        ipc.process = (HANDLE)receiver;
        ipc.cmd = HAL_CMD(HAL_SYSTEM, IPC_SYNC);
        kipc_post_process(&ipc, (KPROCESS*)KERNEL_HANDLE);
        return false;
    }
    kio->granted = receiver;
    return true;
}

void kio_destroy(IO *io)
{
    bool kill_flag = false;
    if (io == NULL)
        return;
    KIO* kio = (KIO*)io->kio;
    KPROCESS* kprocess = kprocess_get_current();
    CHECK_HANDLE(kio, sizeof(KIO));
    CHECK_MAGIC(kio, MAGIC_KIO);

    if (kprocess != kio->owner)
    {
        kprocess_error(kprocess, ERROR_ACCESS_DENIED);
        return;
    }
    disable_interrupts();
    if (kio->granted == kio->owner)
        kio_destroy_internal(kio);
    else
    {
        kio->kill_flag = true;
        kill_flag = true;
    }
    enable_interrupts();
    if (kill_flag)
        kprocess_error(kprocess, ERROR_BUSY);
}
