#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "settings.h"
#include "logging.h"
#include "sstables.h"
#include "memtable.h"
#include "bloom_filter.h"
#include "utils.h"

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

    int key_length = (node->key) ? (int)strlen(node->key) : 0;
    int value_length = (node->value) ? (int)strlen(node->value) : -1; // -1 indicates a tombstone

    size_t w1 = fwrite(&key_length, sizeof(int), 1, context->file);
    size_t w2 = 0;
    if (key_length > 0) {
        w2 = fwrite(node->key, sizeof(char), key_length, context->file);
    }

    size_t w3 = fwrite(&value_length, sizeof(int), 1, context->file);
    size_t w4 = 0;
    if (value_length > 0) {
        w4 = fwrite(node->value, sizeof(char), value_length, context->file);
    }

    if (context->bloom_filter && node->key) {
        bloom_add(context->bloom_filter, node->key);
    }

    size_t expected_w2 = (key_length > 0) ? (size_t)key_length : 0;
    size_t expected_w4 = (value_length > 0) ? (size_t)value_length : 0;

    if (w1 != 1 || w2 != expected_w2 || w3 != 1 || w4 != expected_w4) {
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

    int min_key_length = (min_key) ? strlen(min_key) : 0;
    fwrite(&min_key_length, sizeof(int), 1, file);
    if (min_key_length > 0) {
        fwrite(min_key, sizeof(char), min_key_length, file);
    }

    int max_key_length = (max_key) ? strlen(max_key) : 0;
    fwrite(&max_key_length, sizeof(int), 1, file);
    if (max_key_length > 0) {
        fwrite(max_key, sizeof(char), max_key_length, file);
    }

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

static int _write_to_manifest(uint8_t opcode, SSTable *sstable) {
    char sstable_path[SSTABLE_MAX_PATH_LENGTH / 2];
    get_sstable_path(sstable_path, sizeof(sstable_path));
    char manifest_path[SSTABLE_MAX_PATH_LENGTH];
    snprintf(manifest_path, sizeof(manifest_path), "%s/manifest.dat", sstable_path);

    FILE *manifest_file = _open_file_guarded(manifest_path, "ab");
    if (!manifest_file) {
        error("Failed to open manifest file for writing: %s", manifest_path);
        return -1;
    }

    fwrite(&opcode, sizeof(uint8_t), 1, manifest_file);

    int level = sstable->level;
    fwrite(&level, sizeof(int), 1, manifest_file);

    fwrite(&sstable->id, sizeof(int), 1, manifest_file);

    int min_key_length = (sstable->min_key) ? strlen(sstable->min_key) : 0;
    fwrite(&min_key_length, sizeof(int), 1, manifest_file);
    if (min_key_length > 0) {
        fwrite(sstable->min_key, sizeof(char), min_key_length, manifest_file);
    }

    int max_key_length = (sstable->max_key) ? strlen(sstable->max_key) : 0;
    fwrite(&max_key_length, sizeof(int), 1, manifest_file);
    if (max_key_length > 0) {
        fwrite(sstable->max_key, sizeof(char), max_key_length, manifest_file);
    }

    uint8_t *bloom_array = (sstable->bloom_filter) ? sstable->bloom_filter->bit_array : NULL;
    int bloom_filter_size = BLOOM_FILTER_SIZE_BYTES;
    fwrite(&bloom_filter_size, sizeof(int), 1, manifest_file);
    if (bloom_array != NULL) {
        fwrite(bloom_array, sizeof(uint8_t), BLOOM_FILTER_SIZE_BYTES, manifest_file);
    } else {
        uint8_t empty_bf[BLOOM_FILTER_SIZE_BYTES] = {0};
        fwrite(empty_bf, sizeof(uint8_t), BLOOM_FILTER_SIZE_BYTES, manifest_file);
    }

    fflush(manifest_file);
    fsync(fileno(manifest_file));

    fclose(manifest_file);
    return 0;
}

// INTERFACE FUNCTIONS =============================

/**
 * @brief Flushes the given memtable to disk as an SSTable at the specified level.
 * @param memtable (Memtable *): Pointer to memtable to flush.
 * @param level (int): The level at which to create the SSTable.
 * @return (int): 0 on success, -1 on failure.
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

    int id = _fast_access_sstables[level].count;

    sstable->level = level;
    sstable->min_key = strdup(memtable->min_key);
    sstable->max_key = strdup(memtable->max_key);
    sstable->id = id;
    sstable->path = generate_sstable_filepath(level, id);
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

    if (_write_to_manifest(0x01, sstable)) {
        error("Failed to write SSTable metadata to manifest.");
        remove(sstable->path);
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

/**
 * @brief Frees the memory allocated for an SSTable.
 * @param sstable (SSTable *): Pointer to the SSTable to free.
 */
static void _free_sstable(SSTable *sstable) {
    if (sstable) {
        if (sstable->path) free(sstable->path);
        if (sstable->min_key) free(sstable->min_key);
        if (sstable->max_key) free(sstable->max_key);
        free_bloom_filter(sstable->bloom_filter);
        free(sstable);
    }
}

/**
 * @brief Synchronizes SSTables from the manifest file.
 * @return (int) 0 on success, -1 on failure.
 */
int sync_sstables_from_manifest() {
    char sstable_path[SSTABLE_MAX_PATH_LENGTH / 2];
    get_sstable_path(sstable_path, sizeof(sstable_path));
    char manifest_path[SSTABLE_MAX_PATH_LENGTH];
    snprintf(manifest_path, sizeof(manifest_path), "%s/manifest.dat", sstable_path);

    FILE *manifest_file = _open_file_guarded(manifest_path, "rb");
    if (!manifest_file) {
        error("Failed to open manifest file for reading: %s", manifest_path);
        return -1;
    }

    uint8_t opcode;
    while (fread(&opcode, sizeof(uint8_t), 1, manifest_file) == 1) {
        int level, id, bloom_filter_size, min_key_length, max_key_length;
        char *min_key = NULL, *max_key = NULL;

        fread(&level, sizeof(int), 1, manifest_file);
        fread(&id, sizeof(int), 1, manifest_file);

        fread(&min_key_length, sizeof(int), 1, manifest_file);
        if (min_key_length > 0) {
            min_key = malloc(min_key_length + 1);
            fread(min_key, sizeof(char), min_key_length, manifest_file);
            min_key[min_key_length] = '\0';
        }
        fread(&max_key_length, sizeof(int), 1, manifest_file);
        if (max_key_length > 0) {
            max_key = malloc(max_key_length + 1);
            fread(max_key, sizeof(char), max_key_length, manifest_file);
            max_key[max_key_length] = '\0';
        }

        fread(&bloom_filter_size, sizeof(int), 1, manifest_file);
        uint8_t *bloom_array = malloc(bloom_filter_size);
        fread(bloom_array, sizeof(uint8_t), bloom_filter_size, manifest_file); // Read bloom filter data


        if (opcode == 0x01) { // Add SSTable
            SSTable *sstable = malloc(sizeof(SSTable));
            sstable->level = level;
            sstable->id = id;
            sstable->min_key = min_key;
            sstable->max_key = max_key;

            sstable->path = generate_sstable_filepath(level, id);

            sstable->bloom_filter = bloom_filter_create();
            if (sstable->bloom_filter && sstable->bloom_filter->bit_array) {
                free(sstable->bloom_filter->bit_array);
            }
            sstable->bloom_filter->bit_array = bloom_array;

            if (_add_sstable_to_level_index(sstable)) {
                error("Failed to add SSTable from manifest to fast access index.");
                _free_sstable(sstable);
                continue;
            }
        } else if (opcode == 0x02) { // Remove SSTable

            if (min_key) free(min_key);
            if (max_key) free(max_key);
            if (bloom_array) free(bloom_array);

            LevelIndex *level_index = &_fast_access_sstables[level];
            for (int i = 0; i < level_index->count; i++) {
                SSTable *sstable = level_index->tables[i];
                if (sstable && sstable->id == id) {
                    _free_sstable(sstable);

                    for (int j = i; j < level_index->count - 1; j++) {
                        level_index->tables[j] = level_index->tables[j + 1];
                    }
                    level_index->count--;
                    break;
                }
            }
        }
    }
    fclose(manifest_file);
    return 0;
}

/**
 * @brief Searches for a key in the SSTables across all levels.
 * @param key (char *): The key to search for.
 * @return (SearchResult): Struct containing the value and found status.
 */
SearchResult search_in_sstables(char *key) {
    SearchResult result = { .value = NULL, .found = 0 };
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
                char *value = _read_and_search_in_sstable(sstable, key);
                if (value) {
                    result.value = value;
                    result.found = 1;
                    return result;
                }
            }
        }
    }
    return result;
}

int register_new_sstable(int level, int id, char *min_key, char *max_key, BloomFilter *bloom_filter) {
    if (level < 0 || level >= MAX_SSTABLE_LEVELS) {
        error("Invalid SSTable level: %d", level);
        return -1;
    }

    SSTable *sstable = malloc(sizeof(SSTable));
    if (!sstable) {
        error("Failed to allocate memory for SSTable.");
        return -1;
    }

    BloomFilter *bloom_filter_copy = bloom_filter_create();

    if (!bloom_filter_copy) {
        error("Failed to create Bloom filter for SSTable.");
        free(sstable);
        return -1;
    }

    sstable->level = level;
    sstable->id = id;
    sstable->min_key = strdup(min_key);
    sstable->max_key = strdup(max_key);
    sstable->path = generate_sstable_filepath(level, id);
    memcpy(bloom_filter_copy->bit_array, bloom_filter->bit_array, BLOOM_FILTER_SIZE_BYTES);
    sstable->bloom_filter = bloom_filter_copy;
    free_bloom_filter(bloom_filter);


    if (_add_sstable_to_level_index(sstable)) {
        error("Failed to add SSTable to fast access index.");
        _free_sstable(sstable);
        return -1;
    }

    if (_write_to_manifest(0x01, sstable)) {
        error("Failed to write SSTable metadata to manifest.");
        _free_sstable(sstable);
        return -1;
    }
    return 0;
}

int delete_sstable(int level, int id) {
    if (level < 0 || level >= MAX_SSTABLE_LEVELS) {
        error("Invalid SSTable level: %d", level);
        return -1;
    }

    LevelIndex *level_index = &_fast_access_sstables[level];
    for (int i = 0; i < level_index->count; i++) {
        SSTable *sstable = level_index->tables[i];
        if (sstable && sstable->id == id) {
            if (_write_to_manifest(0x02, sstable)) {
                error("Failed to write SSTable deletion to manifest.");
                return -1;
            }

            if (remove(sstable->path) != 0) {
                warning("Failed to delete physical file %s, but manifest was updated.", sstable->path);
            }

            _free_sstable(sstable);

            for (int j = i; j < level_index->count - 1; j++) {
                level_index->tables[j] = level_index->tables[j + 1];
            }
            level_index->tables[level_index->count - 1] = NULL;
            level_index->count--;


            return 0;
        }
    }
    // error("SSTable L%d_%d.dat not found for deletion.", level, id);
    return -1;
}

int get_sstable_count(int level) {
    if (level < 0 || level >= MAX_SSTABLE_LEVELS) {
        error("Invalid SSTable level: %d", level);
        return -1;
    }
    return _fast_access_sstables[level].count;
}