#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "settings.h"
#include "logging.h"
#include "sstables.h"
#include "memtable.h"

int _current_level_files[MAX_SSTABLE_LEVELS] = {0};


SSTable fast_access_sstables[MAX_SSTABLE_LEVELS];
int loaded_sstables_count = 0;


FILE *open_sstable(SSTable *sstable, const char *mode) {
    if (!sstable || !sstable->path) {
        debug("Invalid SSTable or path.");
        return NULL;
    }
    FILE *file = fopen(sstable->path, mode);
    if (!file) {
        error("Failed to open SSTable file: %s", sstable->path);
        return NULL;
    }
    return file;
}

char *get_memtable_min_key(Memtable *memtable) {
    if (!memtable || !memtable->root) return NULL;
    AVLNode *current = memtable->root;
    while (current->left != NULL) {
        current = current->left;
    }
    return current->key;
}


char *get_memtable_max_key(Memtable *memtable) {
    if (!memtable || !memtable->root) return NULL;
    AVLNode *current = memtable->root;
    while (current->right != NULL) {
        current = current->right;
    }
    return current->key;
}


char *_generate_sstable_filepath(int level) {
    char filepath[SSTABLE_MAX_PATH_LENGTH];

    snprintf(
        filepath,
        sizeof(filepath),
        "%s/L%d_%d.dat",
        get_sstable_path(),
        level,
        _current_level_files[level]
    );

    // Return a dynamically allocated string to avoid returning a Dangling Pointer since filepath is allocated in the stack
    // and will be deallocated when the function returns.
    return strdup(filepath);
}


SSTable *create_sstable_in_memory(char *min_key, char *max_key, int level) {
    SSTable *sstable = malloc(sizeof(SSTable));
    if (!sstable) {
        debug("Failed to allocate memory for SSTable.");
        return NULL;
    }

    char *filepath = _generate_sstable_filepath(level);
    if (!filepath) {
        free(sstable);
        debug("Failed to generate filepath for SSTable.");
        return NULL;
    }

    sstable->path = filepath;
    sstable->min_key = min_key;
    sstable->max_key = max_key;
    sstable->level = level;

    _current_level_files[level]++;

    info("sstable created on memory: %s", sstable->path);
    return sstable;
}


void write_node_to_sstable(FILE *file, AVLNode *node) {
    if (!file || !node) return;

    int key_length = strlen(node->key);
    int value_length = strlen(node->value);

    // Write the length of the key, the key itself, the length of the value, and the value
    fwrite(&key_length, sizeof(int), 1, file);
    fwrite(node->key, sizeof(char), key_length, file);

    fwrite(&value_length, sizeof(int), 1, file);
    fwrite(node->value, sizeof(char), value_length, file);
}


SSTable *create_memtable_on_disk(Memtable *memtable, SSTable *sstable) {
    if (!memtable || !memtable->root) {
        debug("Memtable is empty or NULL.");
        return NULL;
    }
    char *min_key = get_memtable_min_key(memtable);
    char *max_key = get_memtable_max_key(memtable);
    FILE *file = open_sstable(sstable, "wb");
    if (!file) {
        free(sstable->path);
        free(sstable);
        return NULL;
    }

    int min_key_length = strlen(min_key);
    int max_key_length = strlen(max_key);

    //Writes a header to the SSTable file with the min and max keys for optimal search
    fwrite(&min_key_length, sizeof(int), 1, file);
    fwrite(min_key, sizeof(char), min_key_length, file);
    fwrite(&max_key_length, sizeof(int), 1, file);
    fwrite(max_key, sizeof(char), max_key_length, file);

    memtable_traverse_in_order(memtable->root, write_node_to_sstable);
}
