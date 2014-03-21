#ifndef DEFINE_DIRTREE_HEADER
#define DEFINE_DIRTREE_HEADER

#include <stdbool.h>
typedef struct Node_t Node;

struct Node_t {
	const char *name;
	Node *first_child, *next_sibling;
	void *payload;
};

Node *Node_create(const char *name);
void Node_destroy(Node *self);

void Node_addChild(Node *self, Node *child);

Node *Node_find(Node *root, const char *full_path);
Node *Node_insert(Node *root, const char *full_path);

#endif
