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

typedef struct AVLTree {
    AVLNode *root;
    int bytes_allocated;
} AVLTree;

AVLTree *create_avl_tree();

AVLTree *insert_avl_tree(AVLTree *tree, char *key, char *value);

void print_tree_hierarchical(AVLNode *node, int level);

void free_avl_tree(AVLTree *tree);