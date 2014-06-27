/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "dbg.h"
#include "../userspace/core/sys_calls.h"
#include "../userspace/error.h"
#include "kernel.h"

void printf_handler(void* param, const char *const buf, unsigned int size)
{
    dbg_write(buf, size);
}

static inline unsigned int stack_used_max(unsigned int top, unsigned int cur)
{
    unsigned int i;
    unsigned int last = cur;
    for (i = cur - sizeof(unsigned int); i >= top; i -= 4)
        if (*(unsigned int*)i != MAGIC_UNINITIALIZED)
            last = i;
    return last;
}

#if (KERNEL_PROFILING)
/*
void thread_print_stat(THREAD* thread)
{
    char thread_name[THREAD_NAME_PRINT_SIZE + 1];
    TIME thread_uptime;
    //format name
    int i;
    thread_name[THREAD_NAME_PRINT_SIZE] = 0;
    memset(thread_name, ' ', THREAD_NAME_PRINT_SIZE);
    strncpy(thread_name, PROCESS_NAME(thread->heap), THREAD_NAME_PRINT_SIZE);
    for (i = 0; i < THREAD_NAME_PRINT_SIZE; ++i)
        if (thread_name[i] == 0)
            thread_name[i] = ' ';
    printf("%s ", thread_name);

    //format priority
    if (thread->current_priority == IDLE_PRIORITY)
        printf("-idle-   ");
    else
        printf("%2d(%2d)   ", thread->current_priority, thread->base_priority);

    //stack size
    unsigned int current_stack, max_stack;
    current_stack = thread->heap_size - ((unsigned int)thread->sp - (unsigned int)thread->heap) / sizeof(unsigned int);
    max_stack = thread->heap_size - (stack_used_max((unsigned int)thread->heap, (unsigned int)thread->sp) - (unsigned int)thread->heap) / sizeof(unsigned int);
    printf("%3d/%3d/%3d   ", current_stack, max_stack, thread->heap_size);

    //uptime, including time for current thread
    if (thread == __KERNEL->current_thread)
    {
//        get_uptime(&thread_uptime);
        time_sub(&thread->uptime_start, &thread_uptime, &thread_uptime);
        time_add(&thread_uptime, &thread->uptime, &thread_uptime);
    }
    else
    {
        thread_uptime.sec = thread->uptime.sec;
        thread_uptime.usec = thread->uptime.usec;
    }
    printf("%3d:%02d.%03d\n\r", thread_uptime.sec / 60, thread_uptime.sec % 60, thread->uptime.usec / 1000);
}

static inline void svc_thread_stat()
{
    int active_threads_count = 0;
    int i;
    DLIST_ENUM de;
    THREAD* cur;
    printf("    name        priority      stack        uptime\n\r");
    printf("----------------------------------------------------\n\r");
    //current
    thread_print_stat(__KERNEL->current_thread);
    ++active_threads_count;
    //in cache
    for (i = 0; i < __KERNEL->thread_list_size; ++i)
    {
        dlist_enum_start((DLIST**)&__KERNEL->active_threads[i], &de);
        while (dlist_enum(&de, (DLIST**)&cur))
        {
            thread_print_stat(cur);
            ++active_threads_count;
        }
    }
    //out of cache
    dlist_enum_start((DLIST**)&__KERNEL->threads_uncached, &de);
    while (dlist_enum(&de, (DLIST**)&cur))
    {
        thread_print_stat(cur);
        ++active_threads_count;
    }
    printf("total %d threads active\n\r", active_threads_count);
}*/
#endif //KERNEL_PROFILING
