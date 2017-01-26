/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LIB_ARRAY_H
#define LIB_ARRAY_H

#include "../userspace/array.h"

extern const LIB_ARRAY __LIB_ARRAY;

//ONLY for internal LIB layer linking. Use __GLOBAL for KERNEL/USERSPACE
ARRAY* lib_array_create(ARRAY** ar, const STD_MEM* std_mem, unsigned int data_size, unsigned int reserved);
void lib_array_destroy(ARRAY **ar, const STD_MEM* std_mem);
void* lib_array_at(ARRAY* ar, const STD_MEM* std_mem, unsigned int index);
unsigned int lib_array_size(ARRAY* ar, const STD_MEM* std_mem);
void* lib_array_append(ARRAY **ar, const STD_MEM* std_mem);
void* lib_array_insert(ARRAY **ar, const STD_MEM* std_mem, unsigned int index);
ARRAY* lib_array_clear(ARRAY **ar, const STD_MEM* std_mem);
ARRAY* lib_array_remove(ARRAY** ar, const STD_MEM* std_mem, unsigned int index);
ARRAY* lib_array_squeeze(ARRAY** ar, const STD_MEM* std_mem);


#endif // LIB_ARRAY_H
