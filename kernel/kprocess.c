/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "kprocess.h"
#include "karray.h"
#include "kstdlib.h"
#include "string.h"
#include "kstream.h"
#include "kernel.h"
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
    if (kprocess != NULL)
        __GLOBAL->process = kprocess->process;
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

void kprocess_wakeup_internal(KPROCESS* kprocess)
{
    if  (kprocess->flags & PROCESS_FLAGS_WAITING)
    {
        //if timer is still active, kill him
        ksystime_timer_stop_internal(&kprocess->timer);
        kprocess->flags &= ~PROCESS_SYNC_MASK;

        switch (kprocess->flags & PROCESS_MODE_MASK)
        {
        case PROCESS_MODE_WAITING:
            kprocess_add_to_active_list(kprocess);
        case PROCESS_MODE_WAITING_FROZEN:
            kprocess->flags &= ~PROCESS_FLAGS_WAITING;
            break;
        }
    }
}

void kprocess_timeout(void* param)
{
    KPROCESS* kprocess = param;
    disable_interrupts();
    //because timeout is not atomic anymore
    if (kprocess->flags & PROCESS_FLAGS_WAITING)
        kprocess_wakeup_internal(kprocess);
    enable_interrupts();
}

void kprocess_abnormal_exit()
{
#if (KERNEL_DEBUG)
    printk("Warning: abnormal process termination: %s\n", kprocess_name(kprocess_get_current()));
#endif
    kprocess_destroy_current();
}

void kprocess_create(const REX* rex, KPROCESS** kprocess)
{
    unsigned int sys_size;
    disable_interrupts();
    *kprocess = kmalloc(sizeof(KPROCESS));
    enable_interrupts();
    memset((*kprocess), 0, sizeof(KPROCESS));
    //allocate kprocess object
    if (*kprocess != NULL)
    {
        sys_size = sizeof(PROCESS) + KERNEL_IPC_COUNT * sizeof(IPC);
        if ((rex->flags & REX_FLAG_PERSISTENT_NAME) == 0)
            sys_size += strlen(rex->name) + 1;
        sys_size = (sys_size + 3) & ~3;
        disable_interrupts();
        (*kprocess)->process = kmalloc(rex->size + sys_size);
        enable_interrupts();
        if ((*kprocess)->process)
        {
#if (KERNEL_PROFILING)
            memset((*kprocess)->process, MAGIC_UNINITIALIZED_BYTE, rex->size);
#endif
            DO_MAGIC((*kprocess), MAGIC_PROCESS);
            (*kprocess)->flags = 0;
            (*kprocess)->base_priority = rex->priority;
            (*kprocess)->sp = (void*)((unsigned int)(*kprocess)->process + rex->size + sys_size);
            ksystime_timer_init_internal(&(*kprocess)->timer, kprocess_timeout, (*kprocess));
            (*kprocess)->size = rex->size + sys_size;
            kipc_init(*kprocess);
            (*kprocess)->process->stdout = (*kprocess)->process->stdin = INVALID_HANDLE;

            if (rex->flags & REX_FLAG_PERSISTENT_NAME)
                (*kprocess)->process->name = rex->name;
            else
            {
                strcpy(((char*)((*kprocess)->process)) + sizeof(PROCESS) + KERNEL_IPC_COUNT * sizeof(IPC), rex->name);
                (*kprocess)->process->name = (((const char*)((*kprocess)->process)) + sizeof(PROCESS)) + KERNEL_IPC_COUNT * sizeof(IPC);
            }
            pool_init(&(*kprocess)->process->pool, (void*)((*kprocess)->process) + sys_size);

            process_setup_context((*kprocess), rex->fn);

#if (KERNEL_PROCESS_STAT)
            dlist_add_tail((DLIST**)&__KERNEL->wait_processes, (DLIST*)(*kprocess));
#endif
            if ((rex->flags & 1) == PROCESS_FLAGS_ACTIVE)
            {
                (*kprocess)->flags |= PROCESS_MODE_ACTIVE;
                disable_interrupts();
                kprocess_add_to_active_list(*kprocess);
                enable_interrupts();
            }
        }
        else
        {
            disable_interrupts();
            kfree(*kprocess);
            enable_interrupts();
            (*kprocess) = NULL;
            error(ERROR_OUT_OF_PAGED_MEMORY);
        }
    }
    else
        error(ERROR_OUT_OF_SYSTEM_MEMORY);
}

void kprocess_get_flags(KPROCESS* kprocess, unsigned int* flags)
{
    CHECK_HANDLE(kprocess, sizeof(KPROCESS));
    CHECK_MAGIC(kprocess, MAGIC_PROCESS);
    CHECK_ADDRESS(kprocess, flags, sizeof(unsigned int));
    (*flags) = kprocess->flags;
}

void kprocess_set_flags(KPROCESS* kprocess, unsigned int flags)
{
    CHECK_HANDLE(kprocess, sizeof(KPROCESS));
    CHECK_MAGIC(kprocess, MAGIC_PROCESS);
    disable_interrupts();
    if ((flags & 1) == PROCESS_FLAGS_ACTIVE)
    {
        switch (kprocess->flags & PROCESS_MODE_MASK)
        {
        case PROCESS_MODE_FROZEN:
            kprocess_add_to_active_list(kprocess);
        case PROCESS_MODE_WAITING_FROZEN:
            kprocess->flags |= PROCESS_MODE_ACTIVE;
            break;
        }
    }
    else
    {
        switch (kprocess->flags & PROCESS_MODE_MASK)
        {
        case PROCESS_MODE_ACTIVE:
            kprocess_remove_from_active_list(kprocess);
        case PROCESS_MODE_WAITING:
            kprocess->flags &= ~PROCESS_MODE_ACTIVE;
            break;
        }
    }
    enable_interrupts();
}

void kprocess_set_priority(KPROCESS* kprocess, unsigned int priority)
{
    CHECK_HANDLE(kprocess, sizeof(KPROCESS));
    CHECK_MAGIC(kprocess, MAGIC_PROCESS);
    disable_interrupts();
    if (kprocess->base_priority != priority)
    {
        kprocess->base_priority = priority;
        if ((kprocess->flags & PROCESS_MODE_MASK) == PROCESS_MODE_ACTIVE)
        {
            kprocess_remove_from_active_list(kprocess);
            kprocess_add_to_active_list(kprocess);
        }
    }
    enable_interrupts();
}

void kprocess_get_priority(KPROCESS* kprocess, unsigned int* priority)
{
    CHECK_HANDLE(kprocess, sizeof(KPROCESS));
    CHECK_MAGIC(kprocess, MAGIC_PROCESS);
    CHECK_ADDRESS(kprocess, priority, sizeof(unsigned int));
    *priority = kprocess->base_priority;
}

void kprocess_get_current_svc(KPROCESS** var)
{
    KPROCESS* kprocess = kprocess_get_current();
    CHECK_ADDRESS(kprocess, var, sizeof(KPROCESS*));
    *var = kprocess;
}

const char* kprocess_name(KPROCESS* kprocess)
{
    return kprocess->process->name;
}

void kprocess_destroy(KPROCESS* kprocess)
{
    if ((HANDLE)kprocess == INVALID_HANDLE)
        return;
    CHECK_HANDLE(kprocess, sizeof(KPROCESS));
    CHECK_MAGIC(kprocess, MAGIC_PROCESS);
    disable_interrupts();
    CLEAR_MAGIC(kprocess);
    //if kprocess is running, freeze it first
    if ((kprocess->flags & PROCESS_MODE_MASK) == PROCESS_MODE_ACTIVE)
    {
        kprocess_remove_from_active_list(kprocess);
        //we don't need to save context on exit
        if (__KERNEL->active_process == kprocess)
            __KERNEL->active_process = NULL;
    }
    //if kprocess is owned by any sync object, release them
    if  (kprocess->flags & PROCESS_FLAGS_WAITING)
    {
        //if timer is still active, kill him
        ksystime_timer_stop_internal(&kprocess->timer);
        //say sync object to release us
        switch (kprocess->flags & PROCESS_SYNC_MASK)
        {
        case PROCESS_SYNC_TIMER_ONLY:
            break;
        case PROCESS_SYNC_IPC:
            kipc_lock_release(kprocess);
            break;
        case PROCESS_SYNC_STREAM:
            kstream_lock_release((STREAM_HANDLE*)kprocess->sync_object, kprocess);
            break;
        default:
            ASSERT(false);
        }
    }
#if (KERNEL_PROCESS_STAT)
    dlist_remove((DLIST**)&__KERNEL->wait_processes, (DLIST*)kprocess);
#endif
    enable_interrupts();
    //release memory, occupied by kprocess
    disable_interrupts();
    kfree(kprocess->process);
    kfree(kprocess);
    enable_interrupts();
}

void kprocess_sleep(KPROCESS* kprocess, SYSTIME* time, PROCESS_SYNC_TYPE sync_type, void *sync_object)
{
    CHECK_HANDLE(kprocess, sizeof(KPROCESS));
    CHECK_MAGIC(kprocess, MAGIC_PROCESS);
    disable_interrupts();
    ASSERT ((kprocess->flags & PROCESS_FLAGS_WAITING) == 0);
    kprocess_remove_from_active_list(kprocess);
    kprocess->flags |= PROCESS_FLAGS_WAITING | sync_type;
    kprocess->sync_object = sync_object;

    enable_interrupts();
    //create timer if not infinite
    if (time && (time->sec || time->usec))
        ksystime_timer_start_internal(&kprocess->timer, time);
}

void kprocess_wakeup(KPROCESS* kprocess)
{
    CHECK_HANDLE(kprocess, sizeof(KPROCESS));
    CHECK_MAGIC(kprocess, MAGIC_PROCESS);
    disable_interrupts();
    kprocess_wakeup_internal(kprocess);
    enable_interrupts();
}

void kprocess_error(KPROCESS* kprocess, int error)
{
    CHECK_HANDLE(kprocess, sizeof(KPROCESS));
    CHECK_MAGIC(kprocess, MAGIC_PROCESS);
    kprocess->process->error = error;
}

void kprocess_error_current(int error)
{
    KPROCESS* kprocess = kprocess_get_current();
    if (kprocess != NULL)
        kprocess_error(kprocess, error);
}

void kprocess_destroy_current()
{
    KPROCESS* kprocess = kprocess_get_current();
    if (kprocess != NULL)
        kprocess_destroy(kprocess);
}

KPROCESS* kprocess_get_current()
{
    return (__KERNEL->context >= 0) ?  __KERNEL->irqs[__KERNEL->context]->process : __KERNEL->active_process;
}

bool kprocess_check_address(KPROCESS* kprocess, void* addr, unsigned int size)
{
    //don't check on IRQ or kernel post
    if ((HANDLE)kprocess == KERNEL_HANDLE || __KERNEL->context >= 0)
        return true;
    //check PROCESS
    if ((unsigned int)addr >= (unsigned int)kprocess->process && (unsigned int)addr + size < (unsigned int)kprocess->process + kprocess->size)
        return true;

    return false;
}

void kprocess_init(const REX* rex)
{
    KPROCESS* init;
    __KERNEL->next_process = NULL;
    __KERNEL->active_process = NULL;
    dlist_clear((DLIST**)&__KERNEL->processes);
#if (KERNEL_PROCESS_STAT)
    dlist_clear((DLIST**)&__KERNEL->wait_processes);
#endif
    //create and activate first kprocess
    kprocess_create(rex, &init);
}

#if (KERNEL_PROFILING)
void kprocess_switch_test()
{
    KPROCESS* kprocess = kprocess_get_current();
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
    POOL_STAT stat;
    LIB_ENTER;
    ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_stat(&kprocess->process->pool, &stat, kprocess->sp);
    LIB_EXIT;

    printk("%-20.20s ", kprocess_name(kprocess));

    printk("%03d     ", kprocess->base_priority);
    printk("%4b  ", stack_used((unsigned int)pool_free_ptr(&kprocess->process->pool), (unsigned int)kprocess->process + kprocess->size));
    printk("%4b ", kprocess->size);

    if (__KERNEL->error != ERROR_OK)
    {
        printk(DAMAGED);
        __KERNEL->error = ERROR_OK;
    }
    else
    {
        printk(" %3b(%02d)   ", stat.used, stat.used_slots);
        printk("%3b/%3b(%02d) ", stat.free, stat.largest_free, stat.free_slots);
    }

#if (KERNEL_PROCESS_STAT)
    printk("%3d:%02d.%03d", kprocess->uptime.sec / 60, kprocess->uptime.sec % 60, kprocess->uptime.usec / 1000);
#endif
    printk("\n");
}

static inline void kernel_stat()
{
    int i;
    KPOOL* kpool;
    unsigned int total_size;
    bool damaged;
#if (KERNEL_PROCESS_STAT)
    SYSTIME uptime;
#endif
    POOL_STAT stat, total;
    memset(&total, 0x00, sizeof(POOL_STAT));
    total_size = 0;
    damaged = false;
    for (i = 0; i < karray_size_internal(__KERNEL->pools); ++i)
    {
        kpool = kpool_at(i);
        kpool_stat(i, &stat);
        printk("%#08X                         ", kpool->base);
        printk("%4b ", kpool->size);

        if (__KERNEL->error != ERROR_OK)
        {
            printk(DAMAGED);
            __KERNEL->error = ERROR_OK;
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
