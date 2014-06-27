/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "file_private.h"
#include "error.h"
#include <string.h>
#include "mem.h"

#define SIZE_8N3									13

MAGIC_TEXT("FILE NODE", FILE_NODE_TEXT);
MAGIC_TEXT("MOUNT POINT", MOUNT_POINT_TEXT);

MOUNT_POINT* _root_mp =							NULL;

//file system API stub
void unmount_fn_stub(NODE* root)
{

}

//folder API stub
int node_find_first_fn_stub(NODE* parent, HANDLE* find_helper, NODE** node)
{
	*node = NULL;
	return E_NOT_SUPPORTED;
}

int node_find_next_fn_stub(HANDLE find_helper, NODE** node)
{
	*node = NULL;
	return E_NOT_SUPPORTED;
}

bool node_compare_fn_stub(NODE* node1, NODE* node2)
{
	return false;
}

int mkdir_fn_stub(NODE* parent, char* name)
{
	return E_NOT_SUPPORTED;
}

int rm_fn_stub(NODE* node)
{
	return E_NOT_SUPPORTED;
}

int mv_fn_stub(NODE* from, NODE* to)
{
	return E_NOT_SUPPORTED;
}

int getattr_fn_stub(NODE* node, FILE_ATTR *attr)
{
	return E_NOT_SUPPORTED;
}

int setattr_fn_stub(NODE* node, FILE_ATTR *attr)
{
	return E_NOT_SUPPORTED;
}

//file API stub
int open_fn_stub(FILE_HANDLE* fh)
{
	return E_NOT_SUPPORTED;
}

void close_fn_stub(FILE_HANDLE* fh)
{
}

int read_fn_stub(FILE_HANDLE* fh, char *buf, size_t size)
{
	return E_NOT_SUPPORTED;
}

int write_fn_stub(FILE_HANDLE* fh, char* buf, size_t size)
{
	return E_NOT_SUPPORTED;
}

int seek_fn_stub(FILE_HANDLE* fh, size_t pos)
{
	return E_NOT_SUPPORTED;
}

size_t pos_fn_stub(FILE_HANDLE* fh)
{
	return 0;
}

int cancel_io_fn_stub(FILE_HANDLE *fh)
{
	return E_NOT_SUPPORTED;
}

int ioctl_fn_stub(FILE_HANDLE* fh, void* request_in, void* request_out)
{
	return E_NOT_SUPPORTED;
}

//internal calls only
static void node_get_root(NODE** root)
{
	CHECK_MAGIC(_root, MAGIC_NODE, FILE_NODE_TEXT);
	CHECK_MAGIC(_root->mp, MAGIC_MOUNT_POINT, MOUNT_POINT_TEXT);
	*root = _root;
	(*root)->ref_count++;
}

static void node_ref(NODE* parent, NODE** node)
{
	//first, check is node already referenced
	DLIST_ENUM de;
	NODE* ref;
	bool found = false;
	dlist_enum_start((DLIST**)&parent->h.childs, &de);
	while (dlist_enum(&de, (DLIST**)&ref))
	{
		if (parent->mp->api->node_compare_fn(ref, *node))
		{
			found = true;
			free(*node);
			*node = ref;
			++(*node)->ref_count;
			break;
		}
	}

	//not found? add reference
	if (!found)
	{
		DO_MAGIC(*node, MAGIC_NODE);
		(*node)->parent = parent;
		NODE_SET_TYPE(*node, NODE_TYPE_REGULAR);
		(*node)->ref_count = 1;
		(*node)->mp = parent->mp;
		(*node)->h.childs = NULL;
		dlist_add_tail((DLIST**)(&parent->h.childs), (DLIST*)(*node));
	}
}

static void node_ref_path(NODE* node)
{
	NODE* cur = node;
	while (cur)
	{
		CHECK_MAGIC(cur, MAGIC_NODE, FILE_NODE_TEXT);
		cur->ref_count++;
		cur = cur->parent;
	}
}

static void node_unref(NODE** node)
{
	if (--(*node)->ref_count == 0)
	{
		dlist_remove((DLIST**)(&(*node)->parent->h.childs), (DLIST*)(*node));
		free(*node);
		*node = NULL;
	}
}

//follow links and mounted fs entry points
static void node_resolve_links_down(NODE** node)
{
	CHECK_MAGIC(*node, MAGIC_NODE, FILE_NODE_TEXT);
	int node_type;
	while ((node_type = NODE_GET_TYPE(*node)) != NODE_TYPE_REGULAR)
	{
		switch (node_type)
		{
		//on mount point - just follow root node and reference it
		case NODE_TYPE_MOUNT_POINT:
			*node = (*node)->h.link;
			CHECK_MAGIC(*node, MAGIC_NODE, FILE_NODE_TEXT);
			(*node)->ref_count++;
			break;
		//in case of symlink - resolve path to real one
		case NODE_TYPE_LINK:
			node_close_path(*node);
			*node = (*node)->h.link;
			CHECK_MAGIC(*node, MAGIC_NODE, FILE_NODE_TEXT);
			node_ref_path(*node);
			break;
		default:
			break;
		}
	}
}

static void node_resolve_links_up(NODE** node)
{
	CHECK_MAGIC(*node, MAGIC_NODE, FILE_NODE_TEXT);
	while ((*node)->mp->root == *node && (*node)->parent)
	{
		NODE* parent = (*node)->parent;
		node_unref(node);
		*node = parent;
		CHECK_MAGIC(*node, MAGIC_NODE, FILE_NODE_TEXT);
	}
}

int node_mount(NODE** node, MOUNT_POINT* mp)
{
	CHECK_MAGIC(*node, MAGIC_NODE, FILE_NODE_TEXT);
	CHECK_MAGIC(mp, MAGIC_MOUNT_POINT, MOUNT_POINT_TEXT);
	int res = E_OK;
	if (NODE_GET_TYPE(*node) == NODE_TYPE_REGULAR)
	{
		if ((*node)->h.childs == NULL)
		{
			node_ref_path(*node);
			node_ref(*node, &mp->root);
			NODE_SET_TYPE(*node, NODE_TYPE_MOUNT_POINT);
			node_resolve_links_down(node);
		}
		else
			res = E_FILE_PATH_IN_USE;
	}
	else
		res = E_ALREADY_MOUNTED;
	return res;
}

int node_unmount(NODE** node)
{
	CHECK_MAGIC(*node, MAGIC_NODE, FILE_NODE_TEXT);
	int res = E_OK;
	NODE* cur = *node;
	if (cur == cur->mp->root)
	{
		if (cur->ref_count == 2)
		{
			node_resolve_links_up(node);
			NODE_SET_TYPE(*node, NODE_TYPE_REGULAR);
			cur->mp->api->unmount_fn(cur);
			node_close_path(cur);
		}
		else
			res = E_FILE_PATH_IN_USE;
	}
	else
		res = E_NOT_MOUNTED;
	return res;
}

int node_get_attr(NODE* node, FILE_ATTR **attr)
{
	CHECK_MAGIC(node, MAGIC_NODE, FILE_NODE_TEXT);
	int res = E_OK;
	do {
		//if NULL, allocate default size
		if (*attr == NULL)
		{
			*attr = (FILE_ATTR*)malloc(sizeof(FILE_ATTR_FLAGS) + SIZE_8N3);
			if (*attr == NULL)
			{
				res = E_OUT_OF_MEMORY;
				break;
			}
			(*attr)->f.struct_size = sizeof(FILE_ATTR_FLAGS) + SIZE_8N3;
		}
		res = node->mp->api->getattr_fn(node, *attr);
		//if call is failed with OUT_OF_MEMORY error, reallocate memory to bigger size
		if (res == E_OUT_OF_MEMORY)
		{
			int need_size = (*attr)->f.struct_size;
			free(*attr);
			*attr = (FILE_ATTR*)malloc(need_size);
			if (*attr == NULL)
			{
				res = E_OUT_OF_MEMORY;
				break;
			}
			(*attr)->f.struct_size = need_size;
			res = node->mp->api->getattr_fn(node, *attr);
		}
	} while(false);
	if (res < 0)
	{
		free(*attr);
		*attr = NULL;
	}
	return res;
}

int node_set_attr(NODE *node, FILE_ATTR *attr)
{
	CHECK_MAGIC(node, MAGIC_NODE, FILE_NODE_TEXT);
	return node->mp->api->setattr_fn(node, attr);
}

int node_mkdir(NODE* parent, char* name)
{
	CHECK_MAGIC(parent, MAGIC_NODE, FILE_NODE_TEXT);
	NODE* node;
	int res;
	if (is_node_name_legal(name))
	{
		if ((res = node_open_path(name, parent, &node)) == E_FILE_PATH_NOT_FOUND)
			res = parent->mp->api->mkdir_fn(parent, name);
		else if (res == E_OK)
		{
			node_close_path(node);
			res = E_FILE_PATH_ALREADY_EXISTS;
		}
	}
	else
		res = E_FILE_INVALID_NAME;
	return res;
}

int node_rm(NODE* parent, char* name)
{
	CHECK_MAGIC(parent, MAGIC_NODE, FILE_NODE_TEXT);
	NODE* node;
	NODE* child;
	FILE_ATTR* attr;
	HANDLE find_helper;
	int res;
	if (is_node_name_legal(name))
	{
		if ((res = node_open_path(name, parent, &node)) == E_OK)
		{
			if (node->ref_count == 1)
			{
				if ((res = node_get_attr(node, &attr)) == E_OK)
				{
					if (node->flags & F_IS_FOLDER)
					{
						if ((res = node_find_first(node, &find_helper, &child) > 0))
						{
							node_find_close(find_helper, &child);
							res = E_FOLDER_NOT_EMPTY;
						}
					}
					free(attr);
					if (res == E_OK)
						res = parent->mp->api->rm_fn(node);
				}
			}
			else
				res = E_FILE_PATH_IN_USE;
			node_close_path(node);
		}
	}
	else
		res = E_FILE_INVALID_NAME;

	return res;
}

int node_mv(NODE* parent, char* name, NODE* to)
{
	//TODO: check from and to have same mount point
	CHECK_MAGIC(parent, MAGIC_NODE, FILE_NODE_TEXT);
	CHECK_MAGIC(to, MAGIC_NODE, FILE_NODE_TEXT);
	NODE* node;
	int res;
	if (is_node_name_legal(name))
	{
		if ((res = node_open_path(name, parent, &node)) == E_OK)
		{
			if (node->ref_count == 1)
				res = parent->mp->api->mv_fn(node, to);
			else
				res = E_FILE_PATH_IN_USE;
			node_close_path(node);
		}
	}
	else
		res = E_FILE_INVALID_NAME;

	return res;
}

int node_find_next(HANDLE find_helper, NODE **node)
{
	int res;
	CHECK_MAGIC(*node, MAGIC_NODE, FILE_NODE_TEXT);
	NODE* parent = (*node)->parent;
	CHECK_MAGIC(parent, MAGIC_NODE, FILE_NODE_TEXT);
	node_unref(node);
	*node = NULL;
	if ((res = parent->mp->api->node_find_next_fn(find_helper, node)) > 0)
		node_ref(parent, node);
	return res;
}

int node_find_first(NODE* parent, HANDLE* find_helper, NODE **node)
{
	int res;
	FILE_ATTR* attr = NULL;
	*node = NULL;
	if ((res = node_get_attr(parent, &attr)) == E_OK)
	{
		//is it folder?
		if (attr->f.flags & F_IS_FOLDER)
		{
			if ((res = parent->mp->api->node_find_first_fn(parent, find_helper, node)) > 0)
				node_ref(parent, node);
		}
		else
			res = E_FILE_PATH_NOT_FOUND;
		free(attr);
	}
	return res;
}

void node_find_close(HANDLE find_helper, NODE **node)
{
	free((void*)find_helper);
	node_unref(node);
}

bool is_root_path(char* path)
{
	return path[0] && path[0] == '/';
}

int get_next_slash_in_path(char* path, int from)
{
	int to;
	for (to = from; path[to] && path[to] != '/'; ++to) {}
	return to;
}

bool is_node_name_legal(char* name)
{
	int i;
	bool res = true;
	for (i = 0; name[i]; ++i)
		if (name[i] == '/')
		{
			res = false;
			break;
		}
	return res;
}

void node_close_path(NODE* node)
{
	NODE* cur = node;
	while (cur)
	{
		CHECK_MAGIC(cur, MAGIC_NODE, FILE_NODE_TEXT);
		NODE* parent = cur->parent;
		node_unref(&cur);
		cur = parent;
	}
}

void node_cd_up(NODE** node)
{
	CHECK_MAGIC(*node, MAGIC_NODE, FILE_NODE_TEXT);
	node_resolve_links_up(node);
	if ((*node)->parent)
	{
		NODE* parent = (*node)->parent;
		node_unref(node);
		*node = parent;
		CHECK_MAGIC(*node, MAGIC_NODE, FILE_NODE_TEXT);
	}
}

int node_open_path(char* path, NODE *base_path, NODE **node)
{
	int from = 0;
	int to = 0;
	bool found;
	NODE* parent;
	FILE_ATTR* attr;
	int res = E_FILE_PATH_NOT_FOUND;
	HANDLE find_helper;

	if (base_path == NULL)
	{
		node_get_root(node);
	}
	else if (path[0] == '/')
	{
		node_get_root(node);
		++from;
	}
	else
	{
		CHECK_MAGIC(base_path, MAGIC_NODE, FILE_NODE_TEXT);
		node_ref_path(base_path);
		*node = base_path;
	}

	for (; path[to]; from = to + 1)
	{
		//extract current folder in path
		to = get_next_slash_in_path(path, from + 1);
		if (strncmp(path + from, ".", to - from) == 0)
			continue;
		else if (strncmp(path + from, "..", to - from) == 0)
			node_cd_up(node);
		else
		{
			parent = *node;
			found = false;
			//browse all file names in folder and compare
			for (res = node_find_first(parent, &find_helper, node); res > 0; res = node_find_next(find_helper, node))
			{
				//get name, compare
				if ((res = node_get_attr(*node, &attr)) < 0)
				{
					//release fs-specific incomplete search result
					node_find_close(find_helper, node);
					break;
				}
				//compare path name
				found = strncmp(path + from, attr->name, to - from) == 0;
				free(attr);
				if (found)
				{
					res = E_OK;
					node_resolve_links_down(node);
					break;
				}
			}
			//not found, invalid path
			if (!found && res == E_OK)
			{
				res = E_FILE_PATH_NOT_FOUND;
				break;
			}
		}
	}
	if (res < 0)
	{
		node_close_path(*node);
		*node = NULL;
	}
	return res;
}

int node_open_file(NODE* node, int mode, FILE_HANDLE** handle)
{
	CHECK_MAGIC(*node, MAGIC_NODE, FILE_NODE_TEXT);
	int res = E_OK;
	//file
	return res;
}
