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
#include "kdirect.h"
#if (KERNEL_MES)
#include "kmutex.h"
#include "kevent.h"
#include "ksem.h"
#endif
#include "../userspace/error.h"
#include "../lib/pool.h"
#include "../userspace/ipc.h"
#include "klib.h"

#define MAX_PROCESS_NAME_SIZE                                    128

#if (KERNEL_PROFILING)
#if (KERNEL_PROCESS_STAT)
const char *const STAT_LINE="--------------------------------------------------------------------------\n\r";
#else
const char *const STAT_LINE="----------------------------------------------------------------\n\r";
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
    dlist_remove((DLIST**)&__KERNEL->wait_processes, (DLIST*)process);
#endif
    //return from core HALT
    if (__KERNEL->processes == NULL)
    {
        dlist_add_head((DLIST**)&__KERNEL->processes, (DLIST*)process);
        switch_to_process(process);
        return;
    }
#if (KERNEL_MES)
    if (process->current_priority < __KERNEL->processes->current_priority)
#else
    if (process->base_priority < __KERNEL->processes->base_priority)
#endif //KERNEL_MES
    {
#if (KERNEL_PROCESS_STAT)
        if (__KERNEL->processes != NULL)
        {
            ktimer_get_uptime(&process->uptime_start);
            time_sub(&__KERNEL->processes->uptime_start, &process->uptime_start, &__KERNEL->processes->uptime_start);
            time_add(&__KERNEL->processes->uptime_start, &__KERNEL->processes->uptime, &__KERNEL->processes->uptime);
        }
#endif //KERNEL_PROCESS_STAT
        process_to_save = __KERNEL->processes;
        dlist_remove_head((DLIST**)&__KERNEL->processes);
        dlist_add_head((DLIST**)&__KERNEL->processes, (DLIST*)process);
        switch_to_process(process);
    }

    //find place for saved process
    dlist_enum_start((DLIST**)&__KERNEL->processes, &de);
    while (dlist_enum(&de, (DLIST**)&cur))
#if (KERNEL_MES)
        if (process_to_save->current_priority < cur->current_priority)
#else
        if (process_to_save->base_priority < cur->base_priority)
#endif //KERNEL_MES
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
#if (KERNEL_PROCESS_STAT)
        if (((DLIST*)__KERNEL->processes)->next != (DLIST*)__KERNEL->processes)
        {
            ktimer_get_uptime(&((PROCESS*)(((DLIST*)__KERNEL->processes)->next))->uptime_start);
            time_sub(&__KERNEL->processes->uptime_start, &((PROCESS*)(((DLIST*)__KERNEL->processes)->next))->uptime_start, &__KERNEL->processes->uptime_start);
            time_add(&__KERNEL->processes->uptime_start, &__KERNEL->processes->uptime, &__KERNEL->processes->uptime);
        }
#endif //KERNEL_PROCESS_STAT
        dlist_remove_head((DLIST**)&__KERNEL->processes);
        switch_to_process(__KERNEL->processes);
    }
    else
        dlist_remove((DLIST**)&__KERNEL->processes, (DLIST*)process);
#if (KERNEL_PROCESS_STAT)
    dlist_add_tail((DLIST**)&__KERNEL->wait_processes, (DLIST*)process);
#endif
}

void kprocess_wakeup_internal(PROCESS* process)
{
    if  (process->flags & PROCESS_FLAGS_WAITING)
    {
        //if timer is still active, kill him
        if (process->flags & PROCESS_FLAGS_TIMER_ACTIVE)
            ktimer_stop(&process->timer);
        process->flags &= ~(PROCESS_FLAGS_TIMER_ACTIVE | PROCESS_SYNC_MASK);

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
    process->flags &= ~PROCESS_FLAGS_TIMER_ACTIVE;
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
#if (KERNEL_MES)
    case PROCESS_SYNC_MUTEX:
        kmutex_lock_release((MUTEX*)process->sync_object, process);
        break;
    case PROCESS_SYNC_EVENT:
        kevent_lock_release((EVENT*)process->sync_object, process);
        break;
    case PROCESS_SYNC_SEM:
        ksem_lock_release((SEM*)process->sync_object, process);
        break;
#endif //KERNEL_MES
    default:
        ASSERT(false);
    }
    if ((process->flags & PROCESS_SYNC_MASK) != PROCESS_SYNC_TIMER_ONLY)
        process->heap->error =  ERROR_TIMEOUT;
    kprocess_wakeup_internal(process);
}

void kprocess_abnormal_exit()
{
#if (KERNEL_INFO)
    printk("Warning: abnormal process termination: %s\n\r", PROCESS_NAME(kprocess_get_current()->heap));
#endif
    kprocess_destroy_current();
}

void kprocess_create(const REX* rex, PROCESS** process)
{
    *process = kmalloc(sizeof(PROCESS) + rex->ipc_size * sizeof(IPC));
    memset((*process), 0, sizeof(PROCESS) + rex->ipc_size * sizeof(IPC));
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
#if (KERNEL_MES)
            (*process)->current_priority = rex->priority;
#endif //KERNEL_MES
            (*process)->sp = (void*)((unsigned int)(*process)->heap + rex->size);
            (*process)->timer.callback = kprocess_timeout;
            (*process)->timer.param = (*process);
            (*process)->size = rex->size;
            (*process)->blocks_count = 0;
            kipc_init((HANDLE)*process, rex->ipc_size);
            kdirect_init(*process);
            (*process)->heap->handle = (HANDLE)(*process);
            (*process)->heap->stdout = (*process)->heap->stdin = INVALID_HANDLE;

            unsigned int len = strlen(rex->name);
            if (len > MAX_PROCESS_NAME_SIZE)
                len = MAX_PROCESS_NAME_SIZE;
            memcpy(PROCESS_NAME((*process)->heap), rex->name, len);
            PROCESS_NAME((*process)->heap)[len] = '\x0';

            (*process)->heap->struct_size = sizeof(HEAP) + strlen(PROCESS_NAME((*process)->heap)) + 1;

            pool_init(&(*process)->heap->pool, (void*)(*process)->heap + (*process)->heap->struct_size);

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
#if (KERNEL_MES)
    process->base_priority = priority;
    kprocess_set_current_priority(process, kmutex_calculate_owner_priority(process));
#else
    if (process->base_priority != priority)
    {
        process->base_priority = priority;
        if ((process->flags & PROCESS_MODE_MASK) == PROCESS_MODE_ACTIVE)
        {
            kprocess_remove_from_active_list(process);
            kprocess_add_to_active_list(process);
        }
    }

#endif
    enable_interrupts();
}

void kprocess_get_priority(PROCESS* process, unsigned int* priority)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
    CHECK_ADDRESS(process, priority, sizeof(unsigned int));
    *priority = process->base_priority;
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
        if (process->flags & PROCESS_FLAGS_TIMER_ACTIVE)
            ktimer_stop(&process->timer);
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
#if (KERNEL_MES)
        case PROCESS_SYNC_MUTEX:
            kmutex_lock_release((MUTEX*)process->sync_object, process);
            break;
        case PROCESS_SYNC_EVENT:
            kevent_lock_release((EVENT*)process->sync_object, process);
            break;
        case PROCESS_SYNC_SEM:
            ksem_lock_release((SEM*)process->sync_object, process);
            break;
#endif //KERNEL_MES
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

void kprocess_sleep(PROCESS* process, TIME* time, PROCESS_SYNC_TYPE sync_type, void *sync_object)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
    disable_interrupts();
    ASSERT ((process->flags & PROCESS_FLAGS_WAITING) == 0);
    kprocess_remove_from_active_list(process);
    process->flags |= PROCESS_FLAGS_WAITING | sync_type;
    process->sync_object = sync_object;

#if (KERNEL_MES)
    //adjust owner priority
    if (sync_type == PROCESS_SYNC_MUTEX && ((MUTEX*)sync_object)->owner->current_priority > process->current_priority)
        kprocess_set_current_priority(((MUTEX*)sync_object)->owner, process->current_priority);
#endif //KERNEL_MES

    //create timer if not infinite
    if (time->sec || time->usec)
    {
        process->flags |= PROCESS_FLAGS_TIMER_ACTIVE;
        process->timer.time.sec = time->sec;
        process->timer.time.usec = time->usec;
        ktimer_start(&process->timer);
    }
    enable_interrupts();
}

void kprocess_wakeup(PROCESS* process)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
    disable_interrupts();
    kprocess_wakeup_internal(process);
    enable_interrupts();
}

#if (KERNEL_MES)
void kprocess_set_current_priority(PROCESS* process, unsigned int priority)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
    if (process->current_priority != priority)
    {
        switch (process->flags & PROCESS_MODE_MASK)
        {
        case PROCESS_MODE_ACTIVE:
            kprocess_remove_from_active_list(process);
            process->current_priority = priority;
            kprocess_add_to_active_list(process);
            break;
        //if we are waiting for mutex, adjusting priority can affect on mutex owner
        case PROCESS_MODE_WAITING:
        case PROCESS_MODE_WAITING_FROZEN:
            process->current_priority = priority;
            if ((process->flags & PROCESS_SYNC_MASK) == PROCESS_SYNC_MUTEX)
                kprocess_set_current_priority(((MUTEX*)process->sync_object)->owner, kmutex_calculate_owner_priority(((MUTEX*)process->sync_object)->owner));
            break;
        default:
            process->current_priority = priority;
        }
    }
}
#endif //KERNEL_MES

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

bool kprocess_block_open(PROCESS* process, void* data, unsigned int size)
{
    bool res = true;
    disable_interrupts();
    if (process->blocks_count < KERNEL_BLOCKS_COUNT)
    {
        process->blocks[process->blocks_count].data = data;
        process->blocks[process->blocks_count].size = size;
        process->blocks_count++;
    }
    else
        res = false;
    enable_interrupts();
    if (!res)
        kprocess_error(process, ERROR_TOO_MANY_OPEN_BLOCKS);
    return res;
}

void kprocess_block_close(PROCESS* process, void* data)
{
    int i;
    disable_interrupts();
    for (i = 0; i < process->blocks_count; ++i)
        if (process->blocks[i].data == data)
        {
            if (i < --process->blocks_count)
            {
                process->blocks[i].data = process->blocks[process->blocks_count - 1].data;
                process->blocks[i].size = process->blocks[process->blocks_count - 1].size;
            }
            break;
        }
    enable_interrupts();
}

bool kprocess_check_address(PROCESS* process, void* addr, unsigned int size)
{
    int i;
    //don't check on IRQ or kernel post
    if ((HANDLE)process == INVALID_HANDLE || __KERNEL->context >= 0)
        return true;
    //check HEAP
    if ((unsigned int)addr >= (unsigned int)process->heap && (unsigned int)addr + size < (unsigned int)process->heap + process->size)
        return true;
    //check open blocks
    for (i = 0; i < process->blocks_count; ++i)
        if ((unsigned int)addr >= (unsigned int)process->blocks[i].data && (unsigned int)addr + size < (unsigned int)process->blocks[i].data + process->blocks[i].size)
            return true;
    return false;
}

bool kprocess_check_address_read(PROCESS* process, void* addr, unsigned int size)
{
    int i;
    //don't check on IRQ or kernel post
    if ((HANDLE)process == INVALID_HANDLE || __KERNEL->context >= 0)
        return true;
    //check HEAP
    if ((unsigned int)addr >= (unsigned int)process->heap && (unsigned int)addr + size < (unsigned int)process->heap + process->size)
        return true;
    //check FLASH
    if ((unsigned int)addr >= FLASH_BASE && (unsigned int)addr + size < FLASH_BASE + FLASH_SIZE)
        return true;
    //check open blocks
    for (i = 0; i < process->blocks_count; ++i)
        if ((unsigned int)addr >= (unsigned int)process->blocks[i].data && (unsigned int)addr + size < (unsigned int)process->blocks[i].data + process->blocks[i].size)
            return true;
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

    printk("%-16.16s ", PROCESS_NAME(process->heap));

#if (KERNEL_MES)
    printk("%03d(%03d)", process->current_priority, process->base_priority);
#else
    printk("%03d     ", process->base_priority);
#endif
    printk("     %4b   ", stack_used((unsigned int)pool_free_ptr(&process->heap->pool), (unsigned int)process->heap + process->size));
    printk("%4b ", process->size);

    if (__KERNEL->error != ERROR_OK)
    {
        printk(DAMAGED);
        __KERNEL->error = ERROR_OK;
    }
    else
    {
        printk("%4b(%02d)  ", stat.used, stat.used_slots);
        printk("%4b/%4b(%02d)", stat.free, stat.largest_free, stat.free_slots);
    }

#if (KERNEL_PROCESS_STAT)
    printk("%3d:%02d.%03d", process->uptime.sec / 60, process->uptime.sec % 60, process->uptime.usec / 1000);
#endif
    printk("\n\r");
}

static inline void kernel_stat()
{
#if (KERNEL_PROCESS_STAT)
    TIME uptime;
#endif
    POOL_STAT stat;
    LIB_ENTER;
    ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_stat(&__KERNEL->paged, &stat, get_sp());
    LIB_EXIT;

    printk("%-16.16s         ", __KERNEL_NAME);

    printk("     %4b   ", stack_used(SRAM_BASE + SRAM_SIZE - KERNEL_STACK_MAX, SRAM_BASE + SRAM_SIZE));
    printk("%4b ", SRAM_SIZE - KERNEL_GLOBAL_SIZE - sizeof(KERNEL));

    if (__KERNEL->error != ERROR_OK)
    {
        printk(DAMAGED);
        __KERNEL->error = ERROR_OK;
    }
    else
    {
        printk("%4b(%02d)  ", stat.used, stat.used_slots);
        printk("%4b/%4b(%02d)", stat.free, stat.largest_free, stat.free_slots);
    }

#if (KERNEL_PROCESS_STAT)
    ktimer_get_uptime(&uptime);
    printk("%3d:%02d.%03d", uptime.sec / 60, uptime.sec % 60, uptime.usec / 1000);
#endif
    printk("\n\r");
}

void kprocess_info()
{
    int cnt = 0;
    DLIST_ENUM de;
    PROCESS* cur;
#if (KERNEL_PROCESS_STAT)
    printk("\n\r    name         priority  stack max size   used         free       uptime\n\r");
#else
    printk("\n\r    name         priority  stack max size   used         free\n\r");
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
    printk("total %d processess\n\r", cnt);
#else
    printk("total %d active processess\n\r", cnt);
#endif
    printk(STAT_LINE);

    kernel_stat();
    printk(STAT_LINE);
    enable_interrupts();
}
#endif //KERNEL_PROFILING
