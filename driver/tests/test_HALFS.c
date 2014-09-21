#include "lighttest2.h"
#include "../HALFS.h"

TEST(create_node, {
    HALFS *node = HALFS_create("node");
    ASSERT(node->first_child == NULL);
    ASSERT(node->next_sibling == NULL);
    ASSERT(node->backend == NULL);
    ASSERT(streq(node->name, "node"));
    ASSERT(HALFS_find(node, "/") == node);
    HALFS_destroy(node);
})

TEST(add_child, {
    HALFS *root  = HALFS_create("ROOT");
    HALFS *child = HALFS_create("CHILD");
    HALFS_addChild(root, child);
    ASSERT(HALFS_find(root, "/CHILD") == child);
    ASSERT(HALFS_find(root, "/") == root);
    HALFS_destroy(root);
})

TEST(insert, {
    HALFS *root = HALFS_create("ROOT");
    HALFS *child2 = HALFS_insert(root, "/CHILD1/CHILD2");
    HALFS *child1 = HALFS_find(root, "/CHILD1");

    ASSERT(child1 != NULL);
    ASSERT(child1->first_child == child2);

    HALFS *child3 = HALFS_insert(root, "/CHILD1/CHILD3");
    ASSERT(child2->next_sibling == child3);

    HALFS_destroy(root);
})

SUITE(
    ADDTEST(create_node), 
    ADDTEST(add_child),
    ADDTEST(insert)
)