#include "HALFS.h"
#include "utils.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

char *strdup(const char *str);
char *strndup(const char *str, size_t n);

int HALFS_default_size(HALResource *path)
{
    return 0;
}

int HALFS_default_trunc(HALResource *path)
{
    return 0;
}

int HALFS_default_read(HALResource *path, char *buf, size_t size, off_t offset)
{
    return 0;
}

int HALFS_default_write(HALResource *path, const char *buf, size_t size, off_t offset)
{
    return 0;
}

HALFS *HALFS_create(const char *name)
{
	HALFS *res = calloc(1, sizeof(HALFS));
	if (! res)
		return NULL;
	res->name = (const char *) strdup(name);
	res->ops.size = HALFS_default_size;
	res->ops.read = HALFS_default_read;
	res->ops.write = HALFS_default_write;
	res->ops.trunc = HALFS_default_trunc;
	return res;
}

static HALFS *HALFS_createFromSubstring(const char *name, size_t len)
{
	char buf[128];
	strncpy(buf, name, len);
	buf[len] = '\0';
	return HALFS_create(buf);
}

void HALFS_destroy(HALFS *self)
{
	for (HALFS *it=self->first_child; it != NULL;){
		HALFS *next = it->next_sibling;
		HALFS_destroy(it);
		it = next;
	}
	free((void*)self->name);
	free(self);
}

void HALFS_addChild(HALFS *self, HALFS *child)
{
	HALFS **anchor = &(self->first_child);
	while (*anchor){
		anchor = &((*anchor)->next_sibling);
	}
	*anchor = child;
}

HALFS *HALFS_find(HALFS *root, const char *full_path)
{
	assert(full_path[0] == '/');
	const char *next_part = strchr(full_path+1, '/');

	if (next_part == NULL)
		next_part = full_path+strlen(full_path);

	if (full_path[1] == '\0')
		return root;

	HALFS *cur = root->first_child;
	while (cur){
		if (strncmp(cur->name, full_path+1, next_part-full_path-1) == 0){
			if (*next_part == '\0')
				return cur;
			return HALFS_find(cur, next_part);
		}
		cur = cur->next_sibling;
	}
	return NULL;
}

HALFS *HALFS_findParent(HALFS *root, const char *full_path)
{
    HALFS *res = NULL;
    char *copy = strdup(full_path);
    char *last_slash = strrchr(copy, '/');

    if (last_slash > copy){
        *last_slash = '\0';
        res = HALFS_find(root, copy);
    }

    free(copy);
    return res;
}

HALFS *HALFS_insert(HALFS *root, const char *full_path)
{
	assert(full_path[0] == '/');
	const char *next_part = strchr(full_path+1, '/');

	if (next_part == NULL)
		next_part = full_path+strlen(full_path);

	if (full_path[1] == '\0')
		return root;

	HALFS **anchor = &(root->first_child);
	HALFS *cur = root->first_child;
	while (cur){
		if (strncmp(cur->name, full_path+1, next_part-full_path-1) == 0){
			if (*next_part == '\0')
				return cur; /* Element already exists */
			else
				return HALFS_insert(cur, next_part);
		}
		anchor = &(cur->next_sibling);
		cur = cur->next_sibling;
	}

	*anchor = HALFS_createFromSubstring(full_path+1, next_part-full_path-1);
	if (*next_part != '\0')
		return HALFS_insert(*anchor, next_part);
	return *anchor;
}

int HALFS_mode(HALFS *node)
{
	int mode = node->ops.mode;
	if (node->ops.read != HALFS_default_read)
		mode |= 0444;
	if (node->ops.write != HALFS_default_write)
		mode |= 0222;
	if (node->first_child)
		mode |= 0555;
	return mode;
}
