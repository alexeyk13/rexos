/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef FILE_PRIVATE_H
#define FILE_PRIVATE_H

#include "file.h"
#include "dbg.h"
#include "dlist.h"

struct _NODE;
struct _FILE_HANDLE;
struct _MOUNT_POINT;

#define NODE_GET_TYPE(node)				((node)->flags & 3)
#define NODE_SET_TYPE(node, type)		((node)->flags = (((node)->flags & ~3) | type))

typedef enum {
	NODE_TYPE_REGULAR = 0,
	NODE_TYPE_LINK,
	NODE_TYPE_MOUNT_POINT
} NODE_TYPE;

typedef struct _NODE {
	DLIST list;
	MAGIC;
	struct _NODE* parent;
	uint16_t flags;
	uint16_t ref_count;
	struct _MOUNT_POINT* mp;
	union {
		struct _FILE_HANDLE* handles;
		struct _NODE* childs;
		struct _NODE* link;
	} h;
	//fs-specific will follow in implementation
} NODE;

typedef struct _FILE_HANDLE {
	DLIST list;
	MAGIC;
	NODE* node;
	int mode;
	//fs-specific will follow in implementation
} FILE_HANDLE;

typedef struct {
	FILE_HANDLE h;
	HANDLE async_io_complete_event;
	int async_io_transferred;
	int async_io_error;
	//fs-specific will follow in implementation
} FILE_HANDLE_ASYNC;

//file system API
typedef void (*UNMOUNT_FN)(NODE* root);
//folder API
//allocate memory for NODE. Return true, if found, E_OK if not, error code on failure
typedef int (*NODE_FIND_FIRST_FN)(NODE* parent, HANDLE* find_helper, NODE** node);
typedef int (*NODE_FIND_NEXT_FN)(HANDLE find_helper, NODE** node);
typedef bool (*NODE_COMPARE_FN)(NODE* node1, NODE* node2);
typedef int (*MKDIR_FN)(NODE* parent, char* name);
typedef int (*RM_FN)(NODE* node);
typedef int (*MV_FN)(NODE* from, NODE* to);
typedef int (*GETATTR_FN)(NODE* node, FILE_ATTR* attr);
typedef int (*SETATTR_FN)(NODE* node, FILE_ATTR* attr);

//file API
typedef int (*OPEN_FN)(FILE_HANDLE* fh);
typedef void (*CLOSE_FN)(FILE_HANDLE* fh);
typedef int (*READ_FN)(FILE_HANDLE* fh, char* buf, size_t size);
typedef int (*WRITE_FN)(FILE_HANDLE* fh, char* buf, size_t size);
typedef int (*SEEK_FN)(FILE_HANDLE* fh, size_t pos);
typedef size_t (*POS_FN)(FILE_HANDLE* fh);
typedef int (*CANCEL_IO_FN)(FILE_HANDLE* fh);
typedef int (*IOCTL_FN)(FILE_HANDLE* fh, void* request_in, void* request_out);

typedef struct {
	//file system API
	UNMOUNT_FN unmount_fn;
	//folder api
	NODE_FIND_FIRST_FN node_find_first_fn;
	NODE_FIND_NEXT_FN node_find_next_fn;
	NODE_COMPARE_FN node_compare_fn;
	MKDIR_FN mkdir_fn;
	RM_FN rm_fn;
	MV_FN mv_fn;
	GETATTR_FN getattr_fn;
	SETATTR_FN setattr_fn;
	//file api
	OPEN_FN open_fn;
	CLOSE_FN close_fn;
	READ_FN read_fn;
	WRITE_FN write_fn;
	CANCEL_IO_FN cancel_io_fn;
	IOCTL_FN	ioctl_fn;
} FS_API;

typedef struct _MOUNT_POINT {
	MAGIC;
	FS_API* api;
	NODE* root;
} MOUNT_POINT;

//must be created before use
extern NODE* _root;

//file system API stub
void unmount_fn_stub(NODE* root);
//folder API stub
int node_find_first_fn_stub(NODE* parent, HANDLE* find_helper, NODE** node);
int node_find_next_fn_stub(HANDLE find_helper, NODE** node);
bool node_compare_fn_stub(NODE* node1, NODE* node2);
int mkdir_fn_stub(NODE* parent, char* name);
int rm_fn_stub(NODE* node);
int mv_fn_stub(NODE* from, NODE* to);
int getattr_fn_stub(NODE* node, FILE_ATTR* attr);
int setattr_fn_stub(NODE* node, FILE_ATTR* attr);

//file API stub
int open_fn_stub(FILE_HANDLE* fh);
void close_fn_stub(FILE_HANDLE *fh);
int read_fn_stub(FILE_HANDLE* fh, char* buf, size_t size);
int write_fn_stub(FILE_HANDLE *fh, char *buf, size_t size);
int seek_fn_stub(FILE_HANDLE* fh, size_t pos);
size_t pos_fn_stub(FILE_HANDLE* fh);
int cancel_io_fn_stub(FILE_HANDLE* fh);
int ioctl_fn_stub(FILE_HANDLE *fh, void* request_in, void* request_out);

//private file api
int node_mount(NODE** node, MOUNT_POINT* mp);
int node_unmount(NODE** node);

int node_get_attr(NODE *node, FILE_ATTR **attr);
int node_set_attr(NODE *node, FILE_ATTR *attr);

int node_mkdir(NODE* parent, char* name);
int node_rm(NODE* parent, char* name);
int node_mv(NODE* parent, char* name, NODE* to);

int node_find_first(NODE* parent, HANDLE* find_helper, NODE** node);
int node_find_next(HANDLE find_helper, NODE** node);
void node_find_close(HANDLE find_helper, NODE** node);

int get_next_slash_in_path(char* path, int from);
bool is_node_name_legal(char* name);

void node_cd_up(NODE** node);
int node_open_path(char* path, NODE* base_path, NODE** node);
void node_close_path(NODE* node);

//private file api
int node_open_file(NODE* node, int mode, FILE_HANDLE** handle);

#endif // FILE_PRIVATE_H
