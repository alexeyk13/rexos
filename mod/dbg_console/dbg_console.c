/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "dbg_console.h"
#include "core.h"
#include "sys_calls.h"
#include "types.h"
#include "printf.h"


//temporaily for test
static void print_pool_stat(POOL* pool, void* sp)
{
    POOL_STAT stat;
    pool_stat_ex(pool, &stat, sp);
    printf("                ");
    print_size_in_bytes(stat.used, 5);
    printf("(%d)   ", stat.used_slots);
    print_size_in_bytes(stat.free, 5);
    printf("/");
    print_size_in_bytes(stat.largest_free, 0);
    printf("(%d)\n\r", stat.free_slots);
}

void new_mem_stat(POOL *pool, void* sp)
{
    printf("name              used          free       \n\r");
    printf("------------------------------------------\n\r");
    //TODO: browse all threads then core
    print_pool_stat(pool, sp);
}
