#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "settings.h"
#include "logging.h"
#include "sstables.h"
#include "memtable.h"
#include "bloom_filter.h"

LevelIndex _fast_access_sstables[MAX_SSTABLE_LEVELS];

typedef struct _serialization_context {
    FILE *file;
    int error;
    BloomFilter *bloom_filter;
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
        return -1;
    }

    int level = sstable->level;
    LevelIndex *level_index = &_fast_access_sstables[level];

    if (level_index->count >= level_index->capacity) {
        int new_capacity = (level_index->capacity == 0) ? 1 : level_index->capacity * 2;
        SSTable **new_tables = realloc(level_index->tables, new_capacity * sizeof(SSTable *));
        if (!new_tables) {
            error("Failed to reallocate memory for fast access sstables.");
            return -1;
        }
        level_index->tables = new_tables;
        level_index->capacity = new_capacity;
    }
    level_index->tables[level_index->count++] = sstable;
    return 0;
}


static void _write_node_to_sstable_callback(AVLNode *node, void *ctx) {
    _SerializationContext *context = (_SerializationContext *)ctx;
    if (!context || !context->file || context->error || !node) return;

    int key_length = strlen(node->key);
    int value_length = strlen(node->value);

    size_t w1 = fwrite(&key_length, sizeof(int), 1, context->file);
    size_t w2 = fwrite(node->key, sizeof(char), key_length, context->file);
    size_t w3 = fwrite(&value_length, sizeof(int), 1, context->file);
    size_t w4 = fwrite(node->value, sizeof(char), value_length, context->file);

    bloom_add(context->bloom_filter, node->key);

    if (w1 != 1 || w2 != (size_t)key_length || w3 != 1 || w4 != (size_t)value_length) {
        context->error = -1;
        error("IO error while writing node to SSTable.");
    }
}


static int _write_memtable_to_disk(Memtable *memtable, SSTable *sstable) {
    if (!memtable || !memtable->root) {
        debug("Memtable is empty or NULL.");
        return -1;
    }

    FILE *file = _open_file_guarded(sstable->path, "wb");
    if (!file) {
        error("Failed to open SSTable file for writing: %s", sstable->path);
        free(sstable->path);
        free(sstable);
        return -1;
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

    _SerializationContext context = { .file = file, .error = 0, .bloom_filter = sstable->bloom_filter };
    memtable_traverse_in_order(memtable->root, _write_node_to_sstable_callback, &context);
    fclose(file);
    return context.error;
}

static char *_read_and_search_in_sstable(SSTable *sstable, char *key) {
    if (!sstable || !sstable->path || !key) {
        debug("Invalid parameters for search_in_sstable.");
        return NULL;
    }

    FILE *file = _open_file_guarded(sstable->path, "rb");
    if (!file) {
        error("Failed to open SSTable file for reading: %s", sstable->path);
        return NULL;
    }

    int min_key_length, max_key_length;
    fread(&min_key_length, sizeof(int), 1, file);
    char *min_key = malloc(min_key_length + 1);
    fread(min_key, sizeof(char), min_key_length, file);
    min_key[min_key_length] = '\0';

    fread(&max_key_length, sizeof(int), 1, file);
    char *max_key = malloc(max_key_length + 1);
    fread(max_key, sizeof(char), max_key_length, file);
    max_key[max_key_length] = '\0';

    if (strcmp(key, min_key) < 0 || strcmp(key, max_key) > 0) {
        free(min_key);
        free(max_key);
        fclose(file);
        return NULL;
    }

    free(min_key);
    free(max_key);

    char *key_buffer = malloc(MAX_KEY_LENGTH + 1);
    int key_len_on_disk;

    while (fread(&key_len_on_disk, sizeof(int), 1, file) == 1) {

        if (key_len_on_disk <= 0 || key_len_on_disk > MAX_KEY_LENGTH) {
            error("Invalid key length read from SSTable: %d", key_len_on_disk);
            free(key_buffer);
            fclose(file);
            return NULL;
        }

        fread(key_buffer, sizeof(char), key_len_on_disk, file);
        key_buffer[key_len_on_disk] = '\0';

        if (strcmp(key_buffer, key) == 0) {
            int value_length;
            fread(&value_length, sizeof(int), 1, file);

            if (value_length == -1) { // Tombstone
                free(key_buffer);
                fclose(file);
                return NULL;
            }

            char *value_buffer = malloc(value_length + 1);
            fread(value_buffer, sizeof(char), value_length, file);
            value_buffer[value_length] = '\0';

            char *result = strdup(value_buffer);

            free(key_buffer);
            free(value_buffer);
            fclose(file);
            return result;
        } else {
            int unused_value_length;
            fread(&unused_value_length, sizeof(int), 1, file);

            if (unused_value_length > 0) {
                fseek(file, unused_value_length, SEEK_CUR);
            }
        }
    }
    fclose(file);
    return NULL;
}

// INTERFACE FUNCTIONS =============================

/**
 * Flushes the given memtable to disk as an SSTable at the specified level.
 * Returns 0 on success, or -1 on failure.
 * Params:
 *   memtable (Memtable *) - pointer to memtable to flush
 *   level (int) - the level at which to create the SSTable
 */
int flush_memtable_to_disk(Memtable *memtable, int level) {
    if (!memtable || !memtable->root) {
        error("Memtable is empty or NULL.");
        return -1;
    }

    if (level < 0 || level >= MAX_SSTABLE_LEVELS) {
        error("Invalid SSTable level: %d", level);
        return -1;
    }

    SSTable *sstable = malloc(sizeof(SSTable));
    if (!sstable) {
        error("Failed to allocate memory for SSTable.");
        return -1;
    };

    sstable->level = level;
    sstable->min_key = strdup(memtable->min_key);
    sstable->max_key = strdup(memtable->max_key);
    sstable->path = _generate_sstable_filepath(level);
    sstable->bloom_filter = bloom_filter_create();

    if (!sstable->path) {
        error("Failed to generate SSTable file path.");
        free(sstable);
        return -1;
    }

    if (_write_memtable_to_disk(memtable, sstable)) {
        error("Failed to write memtable to disk.");
        free(sstable->path);
        free(sstable);
        return -1;
    }

    if (_add_sstable_to_level_index(sstable)) {
        error("Failed to add SSTable to fast access index.");
        remove(sstable->path);
        free(sstable->path);
        free(sstable);
        return -1;
    }

    info("Memtable flushed to disk as SSTable: %s", sstable->path);
    return 0;
}

char *search_in_sstables(char *key) {
    for (int level = 0; level < MAX_SSTABLE_LEVELS; level++) {
        LevelIndex *level_index = &_fast_access_sstables[level];
        for (int i = level_index->count - 1; i >= 0; i--) {
            SSTable *sstable = level_index->tables[i];

            if (!bloom_check(sstable->bloom_filter, key)) {
                debug("Bloom filter said 'GET OUT!' to key: %s in SSTable: %s", key, sstable->path);
                continue;
            }
            debug("Bloom filter said 'Maybe...' to key: %s in SSTable: %s", key, sstable->path);

            if (sstable && strcmp(key, sstable->min_key) >= 0 && strcmp(key, sstable->max_key) <= 0) {
                char *result = _read_and_search_in_sstable(sstable, key);
                if (result) return result;
            }
        }
    }
    return NULL;
}