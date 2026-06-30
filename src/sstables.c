#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "settings.h"
#include "logging.h"
#include "sstables.h"
#include "memtable.h"

LevelIndex _fast_access_sstables[MAX_SSTABLE_LEVELS];

typedef struct _serialization_context {
    FILE *file;
    int error;
} _SerializationContext;

static FILE *_open_file_guarded(const char *path, const char *mode) {
    if (!path) {
        debug("Invalid file path.");
        return NULL;
    }
    FILE *file = fopen(path, mode);
    if (!file) {
        error("Failed to open SSTable file: %s", path);
        return NULL;
    }
    return file;
}

static char *_generate_sstable_filepath(int level) {
    char filepath[SSTABLE_MAX_PATH_LENGTH];
    char sstable_path[SSTABLE_MAX_PATH_LENGTH / 2];
    get_sstable_path(sstable_path, sizeof(sstable_path));
    int count = _fast_access_sstables[level].count;

    snprintf(
        filepath,
        sizeof(filepath),
        "%s/L%d_%d.dat",
        sstable_path,
        level,
        count
    );

    return strdup(filepath);
}

static int _add_sstable_to_level_index(SSTable *sstable) {
    if (!sstable) {
        debug("SSTable is NULL on _add_sstable_to_level_index.");
        return 1;
    }

    int level = sstable->level;
    LevelIndex *level_index = &_fast_access_sstables[level];

    if (level_index->count >= level_index->capacity) {
        int new_capacity = (level_index->capacity == 0) ? 1 : level_index->capacity * 2;
        SSTable **new_tables = realloc(level_index->tables, new_capacity * sizeof(SSTable *));
        if (!new_tables) {
            error("Failed to reallocate memory for fast access sstables.");
            return 1;
        }
        level_index->tables = new_tables;
        level_index->capacity = new_capacity;
    }
    level_index->tables[level_index->count++] = sstable;
    return 0;
}


static void _write_node_to_sstable_callback(AVLNode *node, void *ctx) {
    _SerializationContext *context = (_SerializationContext *)ctx;
    if (!context || !context->file || !context->error || !node) return;

    int key_length = strlen(node->key);
    int value_length = strlen(node->value);

    size_t w1 = fwrite(&key_length, sizeof(int), 1, context->file);
    size_t w2 = fwrite(node->key, sizeof(char), key_length, context->file);
    size_t w3 = fwrite(&value_length, sizeof(int), 1, context->file);
    size_t w4 = fwrite(node->value, sizeof(char), value_length, context->file);

    if (w1 != 1 || w2 != (size_t)key_length || w3 != 1 || w4 != (size_t)value_length) {
        context->error = 1;
        error("IO error while writing node to SSTable.");
    }
}


static int _write_memtable_to_disk(Memtable *memtable, SSTable *sstable) {
    if (!memtable || !memtable->root) {
        debug("Memtable is empty or NULL.");
        return 1;
    }

    FILE *file = _open_file_guarded(sstable->path, "wb");
    if (!file) {
        free(sstable->path);
        free(sstable);
        return 1;
    }

    char *min_key = memtable->min_key;
    char *max_key = memtable->max_key;

    int min_key_length = strlen(min_key);
    int max_key_length = strlen(max_key);

    //Writes a header to the SSTable file with the min and max keys for optimal search
    fwrite(&min_key_length, sizeof(int), 1, file);
    fwrite(min_key, sizeof(char), min_key_length, file);
    fwrite(&max_key_length, sizeof(int), 1, file);
    fwrite(max_key, sizeof(char), max_key_length, file);

    _SerializationContext context = { .file = file, .error = 0 };
    memtable_traverse_in_order(memtable->root, _write_node_to_sstable_callback, &context);
    fclose(file);
    return context.error;
}

// INTERFACE FUNCTIONS =============================

/**
 * Flushes the given memtable to disk as an SSTable at the specified level.
 * Returns a pointer to the created SSTable on success, or NULL on failure.
 * Params:
 *   memtable (Memtable *) - pointer to memtable to flush
 *   level (int) - the level at which to create the SSTable
 */
SSTable *flush_memtable_to_disk(Memtable *memtable, int level) {
    if (!memtable || !memtable->root) {
        error("Memtable is empty or NULL.");
        return NULL;
    }

    if (level < 0 || level >= MAX_SSTABLE_LEVELS) {
        error("Invalid SSTable level: %d", level);
        return NULL;
    }

    SSTable *sstable = malloc(sizeof(SSTable));
    if (!sstable) {
        error("Failed to allocate memory for SSTable.");
        return NULL;
    };

    sstable->level = level;
    sstable->min_key = memtable->min_key;
    sstable->max_key = memtable->max_key;
    sstable->path = _generate_sstable_filepath(level);

    if (!sstable->path) {
        error("Failed to generate SSTable file path.");
        free(sstable);
        return NULL;
    }

    if (_write_memtable_to_disk(memtable, sstable)) {
        error("Failed to write memtable to disk.");
        free(sstable->path);
        free(sstable);
        return NULL;
    }

    if (_add_sstable_to_level_index(sstable)) {
        error("Failed to add SSTable to fast access index.");
        remove(sstable->path);
        free(sstable->path);
        free(sstable);
        return NULL;
    }

    info("Memtable flushed to disk as SSTable: %s", sstable->path);
    return sstable;
}