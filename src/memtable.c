#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "logging.h"
#include "memtable.h"
#include "sstables.h"
#include "settings.h"
#include "utils.h"

// LOW LEVEL FUNCTIONS =============================
int _get_height(AVLNode *node) {
    if (node == NULL) return 0;
    return node->height;
}

AVLNode *_create_node(char* key, char *value) {
    if (key == NULL) {
        debug("Key or value is NULL in _create_node.");
        return NULL;
    }
    AVLNode *node = malloc(sizeof(AVLNode));
    if (!node) {
        debug("Failed to allocate memory for AVLNode.");
        return NULL;
    }

    node->key = strdup(key);
    if (value != NULL) {
        node->value = strdup(value);
    } else {
        node->value = NULL;
    }
    if (!node->key) {
        debug("Failed to allocate memory for key in AVLNode.");
        if (node->value) free(node->value);
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
        if (node->value != NULL) {
            free(node->value);
        };
        if (value != NULL) {
            node->value = strdup(value);
            if (node->value == NULL) {
                debug("Failed to allocate memory for updated value in AVLNode.");
            }
        } else {
            node->value = NULL; //Tombstone
        }
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

AVLNode *_search(AVLNode *node, char *key) {
    if (node == NULL) return NULL;

    if (strcmp(key, node->key) == 0) return node;
    else if (strcmp(key, node->key) < 0) return _search(node->left, key);
    else return _search(node->right, key);
}

int _update_memtable_min_max_keys(Memtable *memtable, char *key) {
    if (!memtable || !key) {
        debug("Memtable or key is NULL in _update_memtable_min_max_keys.");
        return -1;
    }

    if (memtable->min_key == NULL || strcmp(key, memtable->min_key) < 0) {
        if (memtable->min_key) free(memtable->min_key);
        memtable->min_key = strdup(key);
        if (!memtable->min_key) {
            debug("Failed to allocate memory for min_key in Memtable.");
            return -1;
        }
    }

    if (memtable->max_key == NULL || strcmp(key, memtable->max_key) > 0) {
        if (memtable->max_key) free(memtable->max_key);
        memtable->max_key = strdup(key);
        if (!memtable->max_key) {
            debug("Failed to allocate memory for max_key in Memtable.");
            return -1;
        }
    }

    return 0;
}

void _free_tree(AVLNode *node) {
    if (node == NULL) return;
    _free_tree(node->left);
    _free_tree(node->right);
    free(node->key);
    free(node->value);
    free(node);
}


// INTERFACE FUNCTIONS =============================

/**
 * @brief Creates a new Memtable.
 * @return (Memtable *): Pointer to the newly created Memtable, or NULL on failure.
 */
Memtable *create_memtable() {
    Memtable *tree = malloc(sizeof(Memtable));
    if (!tree) {
        debug("Failed to allocate memory for Memtable.");
        return NULL;
    }
    tree->root = NULL;
    tree->min_key = NULL;
    tree->max_key = NULL;
    tree->bytes_allocated = 0;
    return tree;
}

/**
 * @brief Clears the given Memtable, freeing all associated memory.
 * @param tree (Memtable *): Pointer to the Memtable to clear.
 */
void clear_memtable(Memtable *tree) {
    if (tree == NULL) return;
    _free_tree(tree->root);
    if (tree->min_key) free(tree->min_key);
    if (tree->max_key) free(tree->max_key);
    tree->root = NULL;
    tree->min_key = NULL;
    tree->max_key = NULL;
    tree->bytes_allocated = 0;
}

/**
 * @brief Inserts a key-value pair into the Memtable.
 * @param tree (Memtable *): Pointer to the Memtable.
 * @param key (char *): The key to insert.
 * @param value (char *): The value to insert. If NULL, it indicates a deletion (tombstone).
 * @return (Memtable *): Pointer to the updated Memtable.
 */
Memtable *insert_memtable(Memtable *tree, char *key, char *value) {
    if (tree == NULL) tree = create_memtable();
    tree->root = _insert(tree->root, key, value);

    size_t val_len = (value != NULL) ? strlen(value) : 0;
    tree->bytes_allocated += sizeof(AVLNode) + strlen(key) + val_len + 2;
    _update_memtable_min_max_keys(tree, key);

    return tree;
}

/**
 * @brief Searches for a key in the Memtable.
 * @param tree (Memtable *): Pointer to the Memtable.
 * @param key (char *): The key to search for.
 * @return (SearchResult): Struct containing the value and found status.
 */
SearchResult search_memtable(Memtable *tree, char *key) {
    SearchResult result = { .value = NULL, .found = 0 };
    if (tree == NULL) return result;
    AVLNode *node = _search(tree->root, key);
    if (node == NULL) return result;

    result.value = node->value;
    result.found = 1;
    return result;
}

/**
 * @brief Deletes a key from the Memtable by inserting a tombstone (NULL value).
 * @param tree (Memtable *): Pointer to the Memtable.
 * @param key (char *): The key to delete.
 * @return (void *): Always returns NULL.
 */
void *delete_from_memtable(Memtable *tree, char *key) {
    if (tree == NULL) return NULL;
    tree = insert_memtable(tree, key, NULL);
    return NULL;
}

/**
 * @brief Traverses the Memtable in-order and applies a callback function to each node.
 * @param node (AVLNode *): Pointer to the current node.
 * @param callback (void (*)(AVLNode *, void *)): Callback function to apply to each node.
 * @param context (void *): Context to pass to the callback function.
 */
void memtable_traverse_in_order(AVLNode *node, void (*callback)(AVLNode *, void *), void *context) {
    if (node == NULL) return;
    memtable_traverse_in_order(node->left, callback, context);
    callback(node, context);
    memtable_traverse_in_order(node->right, callback, context);
}

