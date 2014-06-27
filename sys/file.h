/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef FILE_H
#define FILE_H

#include "types.h"
#include "time.h"

//file open modes
#define O_RDONLY							(0x1 << 0)
#define O_WRONLY							(0x1 << 1)
#define O_RD_WR							(O_RDONLY | O_WRONLY)
#define O_APPEND							(0x1 << 2)
//option create file
#define O_CREAT							(0x1 << 3)
//option exclusive access
#define O_EXCL								(0x1 << 4)
//asynchronous access
#define O_NONBLOCK						(0x1 << 5)
//block incomplete async (in other case - return error)
#define O_ASYNC_BLOCK					(0x1 << 6)

//file attributes
#define F_READ_ANY						(0x1 << 0)
#define F_WRITE_ANY						(0x1 << 1)
#define F_EXEC_ANY						(0x1 << 2)
#define F_READ_GROUP						(0x1 << 3)
#define F_WRITE_GROUP					(0x1 << 4)
#define F_EXEC_GROUP						(0x1 << 5)
#define F_READ_USER						(0x1 << 6)
#define F_WRITE_USER						(0x1 << 7)
#define F_EXEC_USER						(0x1 << 8)
#define F_IS_FOLDER						(0x1 << 9)

typedef struct {
	uint16_t struct_size;
	uint16_t uid, gid;
	uint16_t flags;
	size_t size;
	time_t t_create, t_modify, t_last_access;
} FILE_ATTR_FLAGS;

typedef struct {
	FILE_ATTR_FLAGS f;
	char name[65535];
} FILE_ATTR;

#endif // FILE_H
