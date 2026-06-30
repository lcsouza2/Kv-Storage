#ifndef AVL_TREE_H
#define AVL_TREE_H
#endif
#include "interface.h"

typedef struct avl_node {
    char *value;
    char *key;
    int height;
    struct avl_node *left;
    struct avl_node *right;
} AVLNode;

typedef struct memtable {
    AVLNode *root;
    char *min_key;
    char *max_key;
    int bytes_allocated;
} Memtable;

Memtable *create_memtable();

Memtable *insert_memtable(Memtable *tree, char *key, char *value);

KeyValue *search_memtable(Memtable *tree, char *key);

void *delete_from_memtable(Memtable *tree, char *key);

void memtable_traverse_in_order(AVLNode *node, void (*callback)(AVLNode *, void *), void *context);

void clear_memtable(Memtable *tree);