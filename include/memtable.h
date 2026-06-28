#ifndef AVL_TREE_H
#define AVL_TREE_H
#endif

typedef struct AVLNode {
    char *value;
    char *key;
    int height;
    struct AVLNode *left;
    struct AVLNode *right;
} AVLNode;

typedef struct Memtable {
    AVLNode *root;
    int bytes_allocated;
} Memtable;

Memtable *create_memtable();

Memtable *insert_memtable(Memtable *tree, char *key, char *value);

void free_memtable(Memtable *tree);