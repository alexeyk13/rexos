/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef WEB_NODE_H
#define WEB_NODE_H

#include "../../userspace/types.h"
#include "../../userspace/so.h"

typedef struct {
    HANDLE child, next;
    HANDLE self;
    char* name;
    unsigned int flags;
} WEB_NODE_ITEM;

typedef struct {
    HANDLE root;
    SO items;
} WEB_NODE;

void web_node_create(WEB_NODE* web_node);
void web_node_destroy(WEB_NODE* web_node);
HANDLE web_node_allocate(WEB_NODE* web_node, HANDLE parent_handle, char* name, unsigned int flags);
void web_node_free(WEB_NODE* web_node, HANDLE handle);
HANDLE web_node_find_path(WEB_NODE* web_node, char* url, unsigned int url_size);
bool web_node_check_flag(WEB_NODE* web_node, HANDLE handle, unsigned int flag);

#endif // WEB_NODE_H
