/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "kprocess.h"
#include "kprocess_private.h"
#include "karray.h"
#include "kstdlib.h"
#include "string.h"
#include "kstream.h"
#include "kio.h"
#include "kernel.h"
#include "ksystime.h"
#if (KERNEL_BD)
#include "kdirect.h"
#endif //KERNEL_BD
#include "../userspace/error.h"
#include "../lib/pool.h"
#include "../userspace/ipc.h"
#include "../lib/lib_lib.h"

#if (KERNEL_PROFILING)
#if (KERNEL_PROCESS_STAT)
const char *const STAT_LINE="-------------------------------------------------------------------------\n";
#else
const char *const STAT_LINE="---------------------------------------------------------------\n";
#endif
const char *const DAMAGED="     !!!DAMAGED!!!     ";
#endif //(KERNEL_PROFILING)

static inline void switch_to_process(KPROCESS* kprocess)
{
    __KERNEL->next_process = kprocess;
    pend_switch_context();
}

void kprocess_add_to_active_list(KPROCESS* kprocess)
{
    bool found = false;
    KPROCESS* kprocess_to_save = kprocess;
    DLIST_ENUM de;
    KPROCESS* cur;
#if (KERNEL_PROCESS_STAT)
    ksystime_get_uptime_internal(&kprocess->uptime_start);
    dlist_remove((DLIST**)&__KERNEL->wait_processes, (DLIST*)kprocess);
#endif
    //return from core HALT
    if (__KERNEL->processes == NULL)
    {
        dlist_add_head((DLIST**)&__KERNEL->processes, (DLIST*)kprocess);
        switch_to_process(kprocess);
        return;
    }
    if (kprocess->base_priority < __KERNEL->processes->base_priority)
    {
        kprocess_to_save = __KERNEL->processes;
        dlist_remove_head((DLIST**)&__KERNEL->processes);
        dlist_add_head((DLIST**)&__KERNEL->processes, (DLIST*)kprocess);
        switch_to_process(kprocess);
    }

    //find place for saved kprocess
    dlist_enum_start((DLIST**)&__KERNEL->processes, &de);
    while (dlist_enum(&de, (DLIST**)&cur))
        if (kprocess_to_save->base_priority < cur->base_priority)
        {
            dlist_add_before((DLIST**)&__KERNEL->processes, (DLIST*)cur, (DLIST*)kprocess_to_save);
            found = true;
            break;
        }
    if (!found)
        dlist_add_tail((DLIST**)&__KERNEL->processes, (DLIST*)kprocess_to_save);
}

void kprocess_remove_from_active_list(KPROCESS* kprocess)
{
    //freeze active task
    if (kprocess == __KERNEL->processes)
    {
        dlist_remove_head((DLIST**)&__KERNEL->processes);
        switch_to_process(__KERNEL->processes);
    }
    else
        dlist_remove((DLIST**)&__KERNEL->processes, (DLIST*)kprocess);
#if (KERNEL_PROCESS_STAT)
    dlist_add_tail((DLIST**)&__KERNEL->wait_processes, (DLIST*)kprocess);
    SYSTIME time;
    ksystime_get_uptime_internal(&time);
    systime_sub(&(kprocess->uptime_start), &time, &time);
    systime_add(&time, &(kprocess->uptime), &(kprocess->uptime));
#endif
}

void kprocess_wakeup(HANDLE p)
{
    KPROCESS* process = (KPROCESS*)p;
    if  (process->flags & PROCESS_FLAGS_WAITING)
    {
        //if timer is still active, kill him
        ksystime_timer_stop_internal(&process->timer);
        process->flags &= ~PROCESS_SYNC_MASK;

        switch (process->flags & PROCESS_MODE_MASK)
        {
        case PROCESS_MODE_WAITING:
            kprocess_add_to_active_list(process);
        case PROCESS_MODE_WAITING_FROZEN:
            process->flags &= ~PROCESS_FLAGS_WAITING;
            break;
        }
    }
}

void kprocess_timeout(void* param)
{
    KPROCESS* process = param;
    disable_interrupts();
    //because timeout is not atomic anymore
    if (process->flags & PROCESS_FLAGS_WAITING)
        kprocess_wakeup((HANDLE)process);
    enable_interrupts();
}

void kprocess_abnormal_exit()
{
    HANDLE p = kprocess_get_current();
#if (KERNEL_DEBUG)
    printk("Warning: abnormal process termination: %s\n", kprocess_name(p));
#endif
    kprocess_destroy(p);
}

HANDLE kprocess_create(const REX* rex)
{
    unsigned int sys_size;
    KPROCESS* process = kmalloc(sizeof(KPROCESS));
    //allocate kprocess object
    if (process != NULL)
    {
        memset(process, 0, sizeof(KPROCESS));
        sys_size = sizeof(PROCESS) + KERNEL_IPC_COUNT * sizeof(IPC);
        if ((rex->flags & REX_FLAG_PERSISTENT_NAME) == 0)
            sys_size += strlen(rex->name) + 1;
        sys_size = (sys_size + 3) & ~3;
        process->process = kmalloc(rex->size + sys_size);
        if (process->process)
        {
#if (KERNEL_PROFILING)
            memset(process->process, MAGIC_UNINITIALIZED_BYTE, rex->size + sys_size);
#endif
            DO_MAGIC(process, MAGIC_PROCESS);
            process->flags = 0;
            process->base_priority = rex->priority;
            process->sp = (void*)((unsigned int)process->process + rex->size + sys_size);
            ksystime_timer_init_internal(&process->timer, kprocess_timeout, process);
            process->size = rex->size + sys_size;
            kipc_init(process);
            process->process->stdout = process->process->stdin = INVALID_HANDLE;
            process->process->error = ERROR_OK;

            if (rex->flags & REX_FLAG_PERSISTENT_NAME)
                process->process->name = rex->name;
            else
            {
                strcpy(((char*)(process->process)) + sizeof(PROCESS) + KERNEL_IPC_COUNT * sizeof(IPC), rex->name);
                process->process->name = (((const char*)(process->process)) + sizeof(PROCESS)) + KERNEL_IPC_COUNT * sizeof(IPC);
            }
            pool_init(&process->process->pool, (void*)(process->process) + sys_size);

            process_setup_context(process, rex->fn);

#if (KERNEL_PROCESS_STAT)
            dlist_add_tail((DLIST**)&__KERNEL->wait_processes, (DLIST*)process);
#endif
            if ((rex->flags & 1) == PROCESS_FLAGS_ACTIVE)
            {
                process->flags |= PROCESS_MODE_ACTIVE;
                disable_interrupts();
                kprocess_add_to_active_list(process);
                enable_interrupts();
            }
        }
        else
            kfree(process);
    }
    return (HANDLE)process;
}

unsigned int kprocess_get_flags(HANDLE p)
{
    KPROCESS* process = (KPROCESS*)p;
    CHECK_MAGIC(process, MAGIC_PROCESS);
    return process->flags;
}

void kprocess_set_flags(HANDLE p, unsigned int flags)
{
    KPROCESS* process = (KPROCESS*)p;
    CHECK_MAGIC(process, MAGIC_PROCESS);
    disable_interrupts();
    if ((flags & 1) == PROCESS_FLAGS_ACTIVE)
    {
        switch (process->flags & PROCESS_MODE_MASK)
        {
        case PROCESS_MODE_FROZEN:
            kprocess_add_to_active_list(process);
        case PROCESS_MODE_WAITING_FROZEN:
            process->flags |= PROCESS_MODE_ACTIVE;
            break;
        }
    }
    else
    {
        switch (process->flags & PROCESS_MODE_MASK)
        {
        case PROCESS_MODE_ACTIVE:
            kprocess_remove_from_active_list(process);
        case PROCESS_MODE_WAITING:
            process->flags &= ~PROCESS_MODE_ACTIVE;
            break;
        }
    }
    enable_interrupts();
}

void kprocess_set_priority(HANDLE p, unsigned int priority)
{
    KPROCESS* process = (KPROCESS*)p;
    CHECK_MAGIC(process, MAGIC_PROCESS);
    disable_interrupts();
    if (process->base_priority != priority)
    {
        process->base_priority = priority;
        if ((process->flags & PROCESS_MODE_MASK) == PROCESS_MODE_ACTIVE)
        {
            kprocess_remove_from_active_list(process);
            kprocess_add_to_active_list(process);
        }
    }
    enable_interrupts();
}

unsigned int kprocess_get_priority(HANDLE p)
{
    KPROCESS* process = (KPROCESS*)p;
    CHECK_MAGIC(process, MAGIC_PROCESS);
    return process->base_priority;
}

const char* kprocess_name(HANDLE p)
{
    KPROCESS* process = (KPROCESS*)p;
    return process->process->name;
}

void kprocess_destroy(HANDLE p)
{
    KPROCESS* process = (KPROCESS*)p;
    if (p == INVALID_HANDLE)
        return;
    CHECK_MAGIC(process, MAGIC_PROCESS);
    disable_interrupts();
    CLEAR_MAGIC(process);
    //if kprocess is running, freeze it first
    if ((process->flags & PROCESS_MODE_MASK) == PROCESS_MODE_ACTIVE)
    {
        kprocess_remove_from_active_list(process);
        //we don't need to save context on exit
        if (__KERNEL->active_process == process)
            __KERNEL->active_process = NULL;
    }
    //if kprocess is owned by any sync object, release them
    if  (process->flags & PROCESS_FLAGS_WAITING)
    {
        //if timer is still active, kill him
        ksystime_timer_stop_internal(&process->timer);
        //say sync object to release us
        switch (process->flags & PROCESS_SYNC_MASK)
        {
        case PROCESS_SYNC_TIMER_ONLY:
            break;
        case PROCESS_SYNC_IPC:
            kipc_lock_release(process);
            break;
        case PROCESS_SYNC_STREAM:
            kstream_lock_release(process->sync_object, p);
            break;
        }
    }
#if (KERNEL_PROCESS_STAT)
    dlist_remove((DLIST**)&__KERNEL->wait_processes, (DLIST*)process);
#endif
    enable_interrupts();
    //release memory, occupied by kprocess
    kfree(process->process);
    kfree(process);
}

void kprocess_sleep(HANDLE p, SYSTIME* time, PROCESS_SYNC_TYPE sync_type, HANDLE sync_object)
{
    KPROCESS* process = (KPROCESS*)p;
    CHECK_MAGIC(process, MAGIC_PROCESS);
    disable_interrupts();
    kprocess_remove_from_active_list(process);
    process->flags |= PROCESS_FLAGS_WAITING | sync_type;
    process->sync_object = sync_object;

    enable_interrupts();
    //create timer if not infinite
    if (time && (time->sec || time->usec))
        ksystime_timer_start_internal(&process->timer, time);
}

void kprocess_error(HANDLE p, int error)
{
    KPROCESS* process = (KPROCESS*)p;
    process->process->error = error;
}

HANDLE kprocess_get_current()
{
    return (__KERNEL->context >= 0) ?  __KERNEL->irqs[__KERNEL->context]->process : (HANDLE)__KERNEL->active_process;
}

bool kprocess_check_address(HANDLE p, void* addr, unsigned int size)
{
    KPROCESS* process = (KPROCESS*)p;
    //don't check on IRQ or kernel post
    if (p == KERNEL_HANDLE || __KERNEL->context >= 0)
        return true;
    //check PROCESS
    if ((unsigned int)addr >= (unsigned int)process->process && (unsigned int)addr + size < (unsigned int)process->process + process->size)
        return true;

    return false;
}

void kprocess_init(const REX* rex)
{
    __KERNEL->next_process = NULL;
    __KERNEL->active_process = NULL;
    __KERNEL->kerror = ERROR_OK;
    dlist_clear((DLIST**)&__KERNEL->processes);
#if (KERNEL_PROCESS_STAT)
    dlist_clear((DLIST**)&__KERNEL->wait_processes);
#endif
    //create and activate first kprocess
    kprocess_create(rex);
}

#if (KERNEL_PROFILING)
void kprocess_switch_test()
{
    KPROCESS* kprocess = (KPROCESS*)kprocess_get_current();
    disable_interrupts();
    kprocess_remove_from_active_list(kprocess);
    kprocess_add_to_active_list(kprocess);
    enable_interrupts();
    //next kprocess is now same as active kprocess, it will simulate context switching
}

static unsigned int stack_used(unsigned int top, unsigned int end)
{
    unsigned int cur;
    for (cur = top; cur < end && *((unsigned int*)cur) == MAGIC_UNINITIALIZED; cur += 4) {}
    return end - cur;
}

void process_stat(KPROCESS* kprocess)
{
    void* saved;
    int err = ERROR_OK;
    saved = __GLOBAL->process;
    __GLOBAL->process = (PROCESS*)&err;
    POOL_STAT stat;
    ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_stat(&kprocess->process->pool, &stat, kprocess->sp);

    printk("%-20.20s ", kprocess_name((HANDLE)kprocess));

    printk("%03d     ", kprocess->base_priority);
    printk("%4b  ", stack_used((unsigned int)pool_free_ptr(&kprocess->process->pool), (unsigned int)kprocess->process + kprocess->size));
    printk("%4b ", kprocess->size);

    if (err != ERROR_OK)
        printk(DAMAGED);
    else
    {
        printk(" %3b(%02d)   ", stat.used, stat.used_slots);
        printk("%3b/%3b(%02d) ", stat.free, stat.largest_free, stat.free_slots);
    }

#if (KERNEL_PROCESS_STAT)
    printk("%3d:%02d.%03d", kprocess->uptime.sec / 60, kprocess->uptime.sec % 60, kprocess->uptime.usec / 1000);
#endif
    printk("\n");
    __GLOBAL->process = saved;
}

static inline void kernel_stat()
{
    int i;
    KPOOL* kpool;
    unsigned int total_size;
    bool damaged;
    void* saved;
#if (KERNEL_PROCESS_STAT)
    SYSTIME uptime;
#endif
    POOL_STAT stat, total;
    int err = ERROR_OK;
    saved = __GLOBAL->process;
    __GLOBAL->process = (PROCESS*)&err;
    memset(&total, 0x00, sizeof(POOL_STAT));
    total_size = 0;
    damaged = false;
    for (i = 0; i < karray_size_internal(__KERNEL->pools); ++i)
    {
        kpool = kpool_at(i);
        kpool_stat(i, &stat);
        printk("%#08X                         ", kpool->base);
        printk("%4b ", kpool->size);

        if (err != ERROR_OK)
        {
            printk(DAMAGED);
            damaged = true;
        }
        else
        {
            printk("%3b(%02d)  ", stat.used, stat.used_slots);
            printk("%3b/%3b(%02d)", stat.free, stat.largest_free, stat.free_slots);
        }
        printk("\n");
        total_size += kpool->size;
        total.used += stat.used;
        total.used_slots += stat.used_slots;
        total.free += stat.free;
        total.free_slots += stat.free_slots;
        if (stat.largest_free > total.largest_free)
            total.largest_free = stat.largest_free;
    }

    printk("%-20.20s         ", __KERNEL_NAME);

    printk("%4b  ", stack_used(SRAM_BASE + SRAM_SIZE - KERNEL_STACK_MAX, SRAM_BASE + SRAM_SIZE));
    printk("%4b ", total_size);

    if (damaged)
        printk(DAMAGED);
    else
    {
        printk("%3b(%02d)  ", total.used, total.used_slots);
        printk("%3b/%3b(%02d)", total.free, total.largest_free, total.free_slots);
    }

#if (KERNEL_PROCESS_STAT)
    ksystime_get_uptime_internal(&uptime);
    printk("%3d:%02d.%03d", uptime.sec / 60, uptime.sec % 60, uptime.usec / 1000);
#endif
    printk("\n");
    __GLOBAL->process = saved;
}

void kprocess_info()
{
    int cnt = 0;
    DLIST_ENUM de;
    KPROCESS* cur;
#if (KERNEL_PROCESS_STAT)
    printk("\n    name           priority  stack  size   used       free        uptime\n");
#else
    printk("\n    name           priority  stack  size   used       free\n");
#endif
    printk(STAT_LINE);
    disable_interrupts();
    dlist_enum_start((DLIST**)&__KERNEL->processes, &de);
    while (dlist_enum(&de, (DLIST**)&cur))
    {
        process_stat(cur);
        ++cnt;
    }
#if (KERNEL_PROCESS_STAT)
    dlist_enum_start((DLIST**)&__KERNEL->wait_processes, &de);
    while (dlist_enum(&de, (DLIST**)&cur))
    {
        process_stat(cur);
        ++cnt;
    }
    printk("total %d processess\n", cnt);
#else
    printk("total %d active processess\n", cnt);
#endif
    printk(STAT_LINE);

    kernel_stat();
    printk(STAT_LINE);
    enable_interrupts();
}
#endif //KERNEL_PROFILING
