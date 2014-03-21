#include "dirtree.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

char *strdup(const char *str);
char *strndup(const char *str, size_t n);

Node *Node_create(const char *name)
{
	Node *res = malloc(sizeof(Node));
	if (! res)
		return NULL;
	res->name = (const char *) strdup(name);
	res->first_child = res->next_sibling = NULL;
	return res;
}

static Node *Node_createFromSubstring(const char *name, size_t len)
{
	Node *res = malloc(sizeof(Node));
	if (! res)
		return NULL;
	res->name = (const char *) strndup(name, len);
	res->first_child = res->next_sibling = NULL;
	return res;
}

void Node_destroy(Node *self)
{
	for (Node *it=self->first_child; it != NULL;){
		Node *next = it->next_sibling;
		Node_destroy(it);
		it = next;
	}
	free((void*)self->name);
	free(self);
}

void Node_addChild(Node *self, Node *child)
{
	Node **anchor = &(self->first_child);
	while (*anchor){
		anchor = &((*anchor)->next_sibling);
	}
	*anchor = child;
}

Node *Node_find(Node *root, const char *full_path)
{
	assert(full_path[0] == '/');
	const char *next_part = strchr(full_path+1, '/');

	if (next_part == NULL)
		next_part = full_path+strlen(full_path);

	if (full_path[1] == '\0')
		return root;

	Node *cur = root->first_child;
	while (cur){
		if (strncmp(cur->name, full_path+1, next_part-full_path-1) == 0){
			if (*next_part == '\0')
				return cur;
			return Node_find(cur, next_part);
		}
		cur = cur->next_sibling;
	}
	return NULL;
}

Node *Node_insert(Node *root, const char *full_path)
{
	assert(full_path[0] == '/');
	const char *next_part = strchr(full_path+1, '/');

	if (next_part == NULL)
		next_part = full_path+strlen(full_path);

	if (full_path[1] == '\0')
		return root;

	Node **anchor = &(root->first_child);
	Node *cur = root->first_child;
	while (cur){
		if (strncmp(cur->name, full_path+1, next_part-full_path-1) == 0){
			if (*next_part == '\0')
				return cur; /* Element already exists */
			else
				return Node_insert(cur, next_part);
		}
		anchor = &(cur->next_sibling);
		cur = cur->next_sibling;
	}

	*anchor = Node_createFromSubstring(full_path+1, next_part-full_path-1);
	if (*next_part != '\0')
		return Node_insert(*anchor, next_part);
	return *anchor;
}

#if defined TEST_DIRTREE
void dump_tree(Node *root, int indent)
{
	for (int i=0; i<indent; i++)
		printf("  ");
	printf("%s\n", root->name);
	if (root->first_child)
		dump_tree(root->first_child, indent+1);
	if (root->next_sibling)
		dump_tree(root->next_sibling, indent);
}

Node *find_wrap(Node *root, const char *full_path)
{
	printf("FIND %s ... ", full_path);
	Node *res = Node_find(root, full_path);
	if (res)
		printf("FOUND %s\n", res->name);
	else
		printf("NOT FOUND\n");
	return res;
}

void test1()
{
	Node *root = Node_create("ROOT");
	Node_addChild(root, Node_create("a"));

	Node *b = Node_create("b");
	Node_addChild(root, b);
	Node_addChild(b, Node_create("c"));

	Node *c = b->first_child;
	assert(c->next_sibling == NULL);
	assert(c->first_child == NULL);

	Node_addChild(root, Node_create("z"));

	dump_tree(root, 0);

	assert(find_wrap(root, "/") == root);

	assert(find_wrap(root, "/b/c") == c);
	assert(find_wrap(root, "/a")   != NULL);
	assert(find_wrap(root, "/c")   == NULL);
	assert(find_wrap(root, "/a/b") == NULL);
	assert(find_wrap(root, "/b/d") == NULL);
	assert(find_wrap(root, "/z")   != NULL);

	Node_destroy(root);
}

void test2()
{
	Node *root = Node_create("ROOT");

	Node *abc = Node_insert(root, "/a/b/c");

	Node *de  = Node_insert(root, "/d/e");
	Node_insert(root, "/a/f");

	Node *deg = Node_insert(root, "/d/e/g");
	assert(de->first_child == deg);
	
	assert(abc == Node_insert(root, "/a/b/c"));

	dump_tree(root, 0);
	Node_destroy(root);
}

int main(int argc, const char **argv)
{
	test1();
	test2();
}

#endif
