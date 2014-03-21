#ifndef DEFINE_DIRTREE_HEADER
#define DEFINE_DIRTREE_HEADER

#include <stdbool.h>
typedef struct DirTree_t DirTree;

struct DirTree_t {
	const char *name;
	DirTree *first_child, *next_sibling;
	void *payload;
};

DirTree *DirTree_create(const char *name);
void DirTree_destroy(DirTree *self);

void DirTree_addChild(DirTree *self, DirTree *child);

DirTree *DirTree_find(DirTree *root, const char *full_path);
DirTree *DirTree_findParent(DirTree *root, const char *full_path);
DirTree *DirTree_insert(DirTree *root, const char *full_path);

#endif
