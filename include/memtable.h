#ifndef AVL_TREE_H
#define AVL_TREE_H
#endif

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

void memtable_traverse_in_order(AVLNode *node, void (*callback)(AVLNode *, void *), void *context);

void free_memtable(Memtable *tree);