#include "dirtree.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

char *strdup(const char *str);
char *strndup(const char *str, size_t n);

DirTree *DirTree_create(const char *name)
{
	DirTree *res = malloc(sizeof(DirTree));
	if (! res)
		return NULL;
	res->name = (const char *) strdup(name);
	res->first_child = res->next_sibling = NULL;
	return res;
}

static DirTree *DirTree_createFromSubstring(const char *name, size_t len)
{
	DirTree *res = malloc(sizeof(DirTree));
	if (! res)
		return NULL;
	res->name = (const char *) strndup(name, len);
	res->first_child = res->next_sibling = NULL;
	return res;
}

void DirTree_destroy(DirTree *self)
{
	for (DirTree *it=self->first_child; it != NULL;){
		DirTree *next = it->next_sibling;
		DirTree_destroy(it);
		it = next;
	}
	free((void*)self->name);
	free(self);
}

void DirTree_addChild(DirTree *self, DirTree *child)
{
	DirTree **anchor = &(self->first_child);
	while (*anchor){
		anchor = &((*anchor)->next_sibling);
	}
	*anchor = child;
}

DirTree *DirTree_find(DirTree *root, const char *full_path)
{
	assert(full_path[0] == '/');
	const char *next_part = strchr(full_path+1, '/');

	if (next_part == NULL)
		next_part = full_path+strlen(full_path);

	if (full_path[1] == '\0')
		return root;

	DirTree *cur = root->first_child;
	while (cur){
		if (strncmp(cur->name, full_path+1, next_part-full_path-1) == 0){
			if (*next_part == '\0')
				return cur;
			return DirTree_find(cur, next_part);
		}
		cur = cur->next_sibling;
	}
	return NULL;
}

DirTree *DirTree_findParent(DirTree *root, const char *full_path)
{
    DirTree *res = NULL;
    char *copy = strdup(full_path);
    char *last_slash = strrchr(copy, '/');

    if (last_slash > copy){
        *last_slash = '\0';
        res = DirTree_find(root, copy);
    }

    free(copy);
    return res;
}

DirTree *DirTree_insert(DirTree *root, const char *full_path)
{
	assert(full_path[0] == '/');
	const char *next_part = strchr(full_path+1, '/');

	if (next_part == NULL)
		next_part = full_path+strlen(full_path);

	if (full_path[1] == '\0')
		return root;

	DirTree **anchor = &(root->first_child);
	DirTree *cur = root->first_child;
	while (cur){
		if (strncmp(cur->name, full_path+1, next_part-full_path-1) == 0){
			if (*next_part == '\0')
				return cur; /* Element already exists */
			else
				return DirTree_insert(cur, next_part);
		}
		anchor = &(cur->next_sibling);
		cur = cur->next_sibling;
	}

	*anchor = DirTree_createFromSubstring(full_path+1, next_part-full_path-1);
	if (*next_part != '\0')
		return DirTree_insert(*anchor, next_part);
	return *anchor;
}

#if defined TEST_DIRTREE
void dump_tree(DirTree *root, int indent)
{
	for (int i=0; i<indent; i++)
		printf("  ");
	printf("%s\n", root->name);
	if (root->first_child)
		dump_tree(root->first_child, indent+1);
	if (root->next_sibling)
		dump_tree(root->next_sibling, indent);
}

DirTree *find_wrap(DirTree *root, const char *full_path)
{
	printf("FIND %s ... ", full_path);
	DirTree *res = DirTree_find(root, full_path);
	if (res)
		printf("FOUND %s\n", res->name);
	else
		printf("NOT FOUND\n");
	return res;
}

void test1()
{
	DirTree *root = DirTree_create("ROOT");
	DirTree_addChild(root, DirTree_create("a"));

	DirTree *b = DirTree_create("b");
	DirTree_addChild(root, b);
	DirTree_addChild(b, DirTree_create("c"));

	DirTree *c = b->first_child;
	assert(c->next_sibling == NULL);
	assert(c->first_child == NULL);

	DirTree_addChild(root, DirTree_create("z"));

	dump_tree(root, 0);

	assert(find_wrap(root, "/") == root);

	assert(find_wrap(root, "/b/c") == c);
	assert(find_wrap(root, "/a")   != NULL);
	assert(find_wrap(root, "/c")   == NULL);
	assert(find_wrap(root, "/a/b") == NULL);
	assert(find_wrap(root, "/b/d") == NULL);
	assert(find_wrap(root, "/z")   != NULL);

	DirTree_destroy(root);
}

void test2()
{
	DirTree *root = DirTree_create("ROOT");

	DirTree *abc = DirTree_insert(root, "/a/b/c");

	DirTree *de  = DirTree_insert(root, "/d/e");
	DirTree_insert(root, "/a/f");

	DirTree *deg = DirTree_insert(root, "/d/e/g");
	assert(de->first_child == deg);
	
	assert(abc == DirTree_insert(root, "/a/b/c"));

	dump_tree(root, 0);
	DirTree_destroy(root);
}

int main(int argc, const char **argv)
{
	test1();
	test2();
}

#endif
