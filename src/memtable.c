#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "logging.h"
#include "memtable.h"

// LOW LEVEL FUNCTIONS =============================
int _get_height(AVLNode *node) {
    if (node == NULL) return 0;
    return node->height;
}

AVLNode *_create_node(char* key, char *value) {
    if (key == NULL || value == NULL) {
        debug("Key or value is NULL in _create_node.");
        return NULL;
    }
    AVLNode *node = malloc(sizeof(AVLNode));
    if (!node) {
        debug("Failed to allocate memory for AVLNode.");
        return NULL;
    }

    node->key = strdup(key);
    node->value = strdup(value);
    if (!node->key || !node->value) {
        debug("Failed to allocate memory for key or value in AVLNode.");
        free(node->key);
        free(node->value);
        free(node);
        return NULL;
    }
    node->height = 1;
    node->left = NULL;
    node->right = NULL;
    return node;
}

AVLNode *_simple_left_rotate(AVLNode *node) {
    AVLNode *new_root = node->right;
    node->right = new_root->left;
    new_root->left = node;

    int l_height = _get_height(node->left);
    int r_height = _get_height(node->right);
    int new_root_r_height = _get_height(new_root->right);
    node->height = 1 + (l_height > r_height ? l_height : r_height);
    new_root->height = 1 + (l_height > new_root_r_height ? l_height : new_root_r_height);

    return new_root;
}

AVLNode *_simple_right_rotate(AVLNode *node) {
    AVLNode *new_root = node->left;
    node->left = new_root->right;
    new_root->right = node;

    int l_height = _get_height(node->left);
    int r_height = _get_height(node->right);
    int new_root_l_height = _get_height(new_root->left);
    node->height = 1 + (l_height > r_height ? l_height : r_height);
    new_root->height = 1 + (new_root_l_height > r_height ? new_root_l_height : r_height);

    return new_root;
}

AVLNode *_double_left_rotate(AVLNode *node) {
    node->right = _simple_right_rotate(node->right);
    return _simple_left_rotate(node);
}

AVLNode *_double_right_rotate(AVLNode *node) {
    node->left = _simple_left_rotate(node->left);
    return _simple_right_rotate(node);
}

AVLNode *_insert(AVLNode *node, char *key, char *value) {
    if (node == NULL) return _create_node(key, value);

    if (strcmp(key, node->key) > 0) node->right = _insert(node->right, key, value);
    else if (strcmp(key, node->key) < 0) node->left = _insert(node->left, key, value);
    else {
        debug("Key already exists in the AVLNode.");
        return node;
    }

    int l_height = _get_height(node->left);
    int r_height = _get_height(node->right);

    node->height = 1 + (l_height > r_height ? l_height : r_height);
    int balance_factor = _get_height(node->left) - _get_height(node->right);
    if (balance_factor > 1 && strcmp(key, node->left->key) < 0) return _simple_right_rotate(node);
    if (balance_factor < -1 && strcmp(key, node->right->key) > 0) return _simple_left_rotate(node);
    if (balance_factor > 1 && strcmp(key, node->left->key) > 0) return _double_right_rotate(node);
    if (balance_factor < -1 && strcmp(key, node->right->key) < 0) return _double_left_rotate(node);

    return node;
}

void _free_tree(AVLNode *node) {
    if (node == NULL) return;
    _free_tree(node->left);
    _free_tree(node->right);
    free(node->key);
    free(node->value);
    free(node);
}
// LOW LEVEL FUNCTIONS =============================


// INTERFACE FUNCTIONS =============================
Memtable *create_memtable() {
    Memtable *tree = malloc(sizeof(Memtable));
    if (!tree) {
        debug("Failed to allocate memory for Memtable.");
        return NULL;
    }
    tree->root = NULL;
    tree->bytes_allocated = sizeof(Memtable);
    return tree;
}

Memtable *insert_memtable(Memtable *tree, char *key, char *value) {
    if (tree == NULL) return NULL;
    tree->root = _insert(tree->root, key, value);
    tree->bytes_allocated += sizeof(AVLNode) + strlen(key) + strlen(value) + 2; //\0
    return tree;
}

void free_memtable(Memtable *tree) {
    if (tree == NULL) return;
    _free_tree(tree->root);
    free(tree);
}
// INTERFACE FUNCTIONS =============================
