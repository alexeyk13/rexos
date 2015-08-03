/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "kprocess.h"
#include "kmalloc.h"
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

#define MAX_PROCESS_NAME_SIZE                                    128

#if (KERNEL_PROFILING)
#if (KERNEL_PROCESS_STAT)
const char *const STAT_LINE="-------------------------------------------------------------------------\n";
#else
const char *const STAT_LINE="---------------------------------------------------------------\n";
#endif
const char *const DAMAGED="     !!!DAMAGED!!!     ";
#endif //(KERNEL_PROFILING)

static inline void switch_to_process(PROCESS* process)
{
    __KERNEL->next_process = process;
    if (process != NULL)
        __GLOBAL->heap = process->heap;
    pend_switch_context();
}

void kprocess_add_to_active_list(PROCESS* process)
{
    bool found = false;
    PROCESS* process_to_save = process;
    DLIST_ENUM de;
    PROCESS* cur;
#if (KERNEL_PROCESS_STAT)
    ksystime_get_uptime_internal(&process->uptime_start);
    dlist_remove((DLIST**)&__KERNEL->wait_processes, (DLIST*)process);
#endif
    //return from core HALT
    if (__KERNEL->processes == NULL)
    {
        dlist_add_head((DLIST**)&__KERNEL->processes, (DLIST*)process);
        switch_to_process(process);
        return;
    }
    if (process->base_priority < __KERNEL->processes->base_priority)
    {
        process_to_save = __KERNEL->processes;
        dlist_remove_head((DLIST**)&__KERNEL->processes);
        dlist_add_head((DLIST**)&__KERNEL->processes, (DLIST*)process);
        switch_to_process(process);
    }

    //find place for saved process
    dlist_enum_start((DLIST**)&__KERNEL->processes, &de);
    while (dlist_enum(&de, (DLIST**)&cur))
        if (process_to_save->base_priority < cur->base_priority)
        {
            dlist_add_before((DLIST**)&__KERNEL->processes, (DLIST*)cur, (DLIST*)process_to_save);
            found = true;
            break;
        }
    if (!found)
        dlist_add_tail((DLIST**)&__KERNEL->processes, (DLIST*)process_to_save);
}

void kprocess_remove_from_active_list(PROCESS* process)
{
    //freeze active task
    if (process == __KERNEL->processes)
    {
        dlist_remove_head((DLIST**)&__KERNEL->processes);
        switch_to_process(__KERNEL->processes);
    }
    else
        dlist_remove((DLIST**)&__KERNEL->processes, (DLIST*)process);
#if (KERNEL_PROCESS_STAT)
    dlist_add_tail((DLIST**)&__KERNEL->wait_processes, (DLIST*)process);
    SYSTIME time;
    ksystime_get_uptime_internal(&time);
    systime_sub(&(process->uptime_start), &time, &time);
    systime_add(&time, &(process->uptime), &(process->uptime));
#endif
}

void kprocess_wakeup_internal(PROCESS* process)
{
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
    PROCESS* process = param;
    disable_interrupts();
    //because timeout is not atomic anymore
    if (process->flags & PROCESS_FLAGS_WAITING)
    {
        //say sync object to release us
        switch (process->flags & PROCESS_SYNC_MASK)
        {
        case PROCESS_SYNC_TIMER_ONLY:
            break;
        case PROCESS_SYNC_IPC:
            kipc_lock_release((HANDLE)process);
            break;
        case PROCESS_SYNC_STREAM:
            kstream_lock_release((STREAM_HANDLE*)process->sync_object, process);
            break;
        default:
            ASSERT(false);
        }
        if ((process->flags & PROCESS_SYNC_MASK) != PROCESS_SYNC_TIMER_ONLY)
            process->heap->error =  ERROR_TIMEOUT;
        kprocess_wakeup_internal(process);
    }
    enable_interrupts();
}

void kprocess_abnormal_exit()
{
#if (KERNEL_DEBUG)
    printk("Warning: abnormal process termination: %s\n", kprocess_name(kprocess_get_current()));
#endif
    kprocess_destroy_current();
}

void kprocess_create(const REX* rex, PROCESS** process)
{
    *process = kmalloc(sizeof(PROCESS) + KERNEL_IPC_SIZE * sizeof(IPC));
    memset((*process), 0, sizeof(PROCESS) + KERNEL_IPC_SIZE * sizeof(IPC));
    //allocate process object
    if (*process != NULL)
    {
        (*process)->heap = kmalloc(rex->size);
        if ((*process)->heap)
        {
#if (KERNEL_PROFILING)
            memset((*process)->heap, MAGIC_UNINITIALIZED_BYTE, rex->size);
#endif
            DO_MAGIC((*process), MAGIC_PROCESS);
            (*process)->flags = 0;
            (*process)->base_priority = rex->priority;
            (*process)->sp = (void*)((unsigned int)(*process)->heap + rex->size);
            ksystime_timer_init_internal(&(*process)->timer, kprocess_timeout, (*process));
            (*process)->size = rex->size;
            kipc_init((HANDLE)*process);
#if (KERNEL_BD)
            dlist_clear((DLIST**)&((*process)->blocks));
            kdirect_init(*process);
#endif //KERNEL_BD
            (*process)->heap->stdout = (*process)->heap->stdin = INVALID_HANDLE;

            (*process)->heap->flags = rex->flags >> REX_HEAP_FLAGS_OFFSET;

            if ((*process)->heap->flags & HEAP_PERSISTENT_NAME)
                *((const char**)(kprocess_struct_ptr((*process), HEAP_STRUCT_NAME))) = rex->name;
            else
            {
                unsigned int len = strlen(rex->name);
                if (len > MAX_PROCESS_NAME_SIZE)
                    len = MAX_PROCESS_NAME_SIZE;
                memcpy(kprocess_struct_ptr((*process), HEAP_STRUCT_NAME), rex->name, len);
                ((char*)(kprocess_struct_ptr((*process), HEAP_STRUCT_NAME)))[len] = '\x0';
            }

            pool_init(&(*process)->heap->pool, kprocess_struct_ptr((*process), HEAP_STRUCT_FREE));

            process_setup_context((*process), rex->fn);

#if (KERNEL_PROCESS_STAT)
            dlist_add_tail((DLIST**)&__KERNEL->wait_processes, (DLIST*)(*process));
#endif
            if ((rex->flags & 1) == PROCESS_FLAGS_ACTIVE)
            {
                (*process)->flags |= PROCESS_MODE_ACTIVE;
                disable_interrupts();
                kprocess_add_to_active_list(*process);
                enable_interrupts();
            }
        }
        else
        {
            kfree(*process);
            (*process) = NULL;
            error(ERROR_OUT_OF_PAGED_MEMORY);
        }
    }
    else
        error(ERROR_OUT_OF_SYSTEM_MEMORY);
}

void kprocess_get_flags(PROCESS* process, unsigned int* flags)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
    CHECK_ADDRESS(process, flags, sizeof(unsigned int));
    (*flags) = process->flags;
}

void kprocess_set_flags(PROCESS* process, unsigned int flags)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
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

void kprocess_set_priority(PROCESS* process, unsigned int priority)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
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

void kprocess_get_priority(PROCESS* process, unsigned int* priority)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
    CHECK_ADDRESS(process, priority, sizeof(unsigned int));
    *priority = process->base_priority;
}

void kprocess_get_current_svc(PROCESS** var)
{
    PROCESS* process = kprocess_get_current();
    CHECK_ADDRESS(process, var, sizeof(PROCESS*));
    *var = process;
}

void kprocess_destroy(PROCESS* process)
{
    if ((HANDLE)process == INVALID_HANDLE)
        return;
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
    disable_interrupts();
    CLEAR_MAGIC(process);
    //if process is running, freeze it first
    if ((process->flags & PROCESS_MODE_MASK) == PROCESS_MODE_ACTIVE)
    {
        kprocess_remove_from_active_list(process);
        //we don't need to save context on exit
        if (__KERNEL->active_process == process)
            __KERNEL->active_process = NULL;
    }
    //if process is owned by any sync object, release them
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
            kipc_lock_release((HANDLE)process);
            break;
        case PROCESS_SYNC_STREAM:
            kstream_lock_release((STREAM_HANDLE*)process->sync_object, process);
            break;
        default:
            ASSERT(false);
        }
    }
#if (KERNEL_PROCESS_STAT)
    dlist_remove((DLIST**)&__KERNEL->wait_processes, (DLIST*)process);
#endif
    enable_interrupts();
    //release memory, occupied by process
    kfree(process->heap);
    kfree(process);
}

void kprocess_sleep(PROCESS* process, SYSTIME* time, PROCESS_SYNC_TYPE sync_type, void *sync_object)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
    disable_interrupts();
    ASSERT ((process->flags & PROCESS_FLAGS_WAITING) == 0);
    kprocess_remove_from_active_list(process);
    process->flags |= PROCESS_FLAGS_WAITING | sync_type;
    process->sync_object = sync_object;

    enable_interrupts();
    //create timer if not infinite
    if (time && (time->sec || time->usec))
        ksystime_timer_start_internal(&process->timer, time);
}

void kprocess_wakeup(PROCESS* process)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
    disable_interrupts();
    kprocess_wakeup_internal(process);
    enable_interrupts();
}

void kprocess_error(PROCESS* process, int error)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
    process->heap->error = error;
}

void kprocess_error_current(int error)
{
    PROCESS* process = kprocess_get_current();
    if (process != NULL)
        kprocess_error(process, error);
}

void kprocess_destroy_current()
{
    PROCESS* process = kprocess_get_current();
    if (process != NULL)
        kprocess_destroy(process);
}

PROCESS* kprocess_get_current()
{
    return (__KERNEL->context >= 0) ?  __KERNEL->irqs[__KERNEL->context]->process : __KERNEL->active_process;
}

bool kprocess_check_address(PROCESS* process, void* addr, unsigned int size)
{
    //don't check on IRQ or kernel post
    if ((HANDLE)process == KERNEL_HANDLE || __KERNEL->context >= 0)
        return true;
    //check HEAP
    if ((unsigned int)addr >= (unsigned int)process->heap && (unsigned int)addr + size < (unsigned int)process->heap + process->size)
        return true;

#if (KERNEL_BD)
    DLIST_ENUM de;
    BLOCK* cur;
    //check open blocks
    dlist_enum_start((DLIST**)&process->blocks, &de);
    while (dlist_enum(&de, (DLIST**)&cur))
    {
        if ((unsigned int)addr >= (unsigned int)cur->data && (unsigned int)addr + size < (unsigned int)cur->data + cur->size)
            return true;
    }
#endif //KERNEL_BD
    return false;
}

bool kprocess_check_address_read(PROCESS* process, void* addr, unsigned int size)
{
    //don't check on IRQ or kernel post
    if ((HANDLE)process == KERNEL_HANDLE || __KERNEL->context >= 0)
        return true;
    //check HEAP
    if ((unsigned int)addr >= (unsigned int)process->heap && (unsigned int)addr + size < (unsigned int)process->heap + process->size)
        return true;
    //check FLASH
    if ((unsigned int)addr >= FLASH_BASE && (unsigned int)addr + size < FLASH_BASE + FLASH_SIZE)
        return true;
#if (KERNEL_BD)
    DLIST_ENUM de;
    BLOCK* cur;
    //check open blocks
    dlist_enum_start((DLIST**)&process->blocks, &de);
    while (dlist_enum(&de, (DLIST**)&cur))
    {
        if ((unsigned int)addr >= (unsigned int)cur->data && (unsigned int)addr + size < (unsigned int)cur->data + cur->size)
            return true;
    }
#endif //KERNEL_BD
    return false;
}

void kprocess_init(const REX* rex)
{
    PROCESS* init;
    __KERNEL->next_process = NULL;
    __KERNEL->active_process = NULL;
    dlist_clear((DLIST**)&__KERNEL->processes);
#if (KERNEL_PROCESS_STAT)
    dlist_clear((DLIST**)&__KERNEL->wait_processes);
#endif
    //create and activate first process
    kprocess_create(rex, &init);
}

#if (KERNEL_PROFILING)
void kprocess_switch_test()
{
    PROCESS* process = kprocess_get_current();
    disable_interrupts();
    kprocess_remove_from_active_list(process);
    kprocess_add_to_active_list(process);
    enable_interrupts();
    //next process is now same as active process, it will simulate context switching
}

static unsigned int stack_used(unsigned int top, unsigned int end)
{
    unsigned int cur;
    for (cur = top; cur < end && *((unsigned int*)cur) == MAGIC_UNINITIALIZED; cur += 4) {}
    return end - cur;
}

void process_stat(PROCESS* process)
{
    POOL_STAT stat;
    LIB_ENTER;
    ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_stat(&process->heap->pool, &stat, process->sp);
    LIB_EXIT;

    printk("%-20.20s ", kprocess_name(process));

    printk("%03d     ", process->base_priority);
    printk("%4b  ", stack_used((unsigned int)pool_free_ptr(&process->heap->pool), (unsigned int)process->heap + process->size));
    printk("%4b ", process->size);

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
    printk("%3d:%02d.%03d", process->uptime.sec / 60, process->uptime.sec % 60, process->uptime.usec / 1000);
#endif
    printk("\n");
}

static inline void kernel_stat()
{
#if (KERNEL_PROCESS_STAT)
    SYSTIME uptime;
#endif
    POOL_STAT stat;
    LIB_ENTER;
    ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_stat(&__KERNEL->paged, &stat, get_sp());
    LIB_EXIT;

    printk("%-20.20s         ", __KERNEL_NAME);

    printk("%4b  ", stack_used(SRAM_BASE + SRAM_SIZE - KERNEL_STACK_MAX, SRAM_BASE + SRAM_SIZE));
    printk("%4b ", SRAM_SIZE - KERNEL_GLOBAL_SIZE - sizeof(KERNEL));

    if (__KERNEL->error != ERROR_OK)
    {
        printk(DAMAGED);
        __KERNEL->error = ERROR_OK;
    }
    else
    {
        printk("%3b(%02d)  ", stat.used, stat.used_slots);
        printk("%3b/%3b(%02d)", stat.free, stat.largest_free, stat.free_slots);
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
    PROCESS* cur;
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
