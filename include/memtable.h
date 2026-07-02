#ifndef MEMTABLE_H
#define MEMTABLE_H
#include "utils.h"

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
SearchResult search_memtable(Memtable *tree, char *key);
void *delete_from_memtable(Memtable *tree, char *key);
void display_all_keys_in_memtable(Memtable *tree);
void memtable_traverse_in_order(AVLNode *node, void (*callback)(AVLNode *, void *), void *context);
void clear_memtable(Memtable *tree);
#endif
