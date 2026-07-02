#include "k_way_iterator.h"
#include "min_heap.h"
#include "settings.h"
#include "logging.h"
#include "bloom_filter.h"
#include "sstables.h"
#include <string.h>
#include <stdlib.h>

static SSTableIterator *_create_iterator() {
    SSTableIterator *iterator = malloc(sizeof(SSTableIterator));
    if (!iterator) return NULL;
    iterator->sstable_id = 0;
    iterator->file = NULL;
    iterator->current_key = NULL;
    iterator->current_value = NULL;
    iterator->is_exhausted = 0;
    iterator->level = 0;
    return iterator;
}

static int _get_next_key_value_from_iterator(SSTableIterator *iterator) {
    if (iterator->is_exhausted) return -1;

    if (iterator->current_key) free(iterator->current_key);
    if (iterator->current_value) free(iterator->current_value);
    iterator->current_key = NULL;
    iterator->current_value = NULL;


    int key_len;
    if (fread(&key_len, sizeof(int), 1, iterator->file) != 1) {
        iterator->is_exhausted = 1;
        return -1;
    }

    if (key_len > 0) {
        iterator->current_key = malloc(key_len + 1);
        fread(iterator->current_key, sizeof(char), key_len, iterator->file);
        iterator->current_key[key_len] = '\0';
    }

    int value_len;
    fread(&value_len, sizeof(int), 1, iterator->file);
    if (value_len > 0) {
        iterator->current_value = malloc(value_len + 1);
        fread(iterator->current_value, sizeof(char), value_len, iterator->file);
        iterator->current_value[value_len] = '\0';
    }

    return 0;
}

int lsm_compare_key_pairs(const void *a, const void *b) {
    const HeapNode *nodeA = (const HeapNode *)a;
    const HeapNode *nodeB = (const HeapNode *)b;

    int cmp = strcmp(nodeA->key, nodeB->key);
    if (cmp != 0) {
        return cmp;
    }
    return nodeA->source_index - nodeB->source_index;
}

static SSTableIterator *_iterator_init(int sstable_id, int level) {
    char sstable_path[SSTABLE_MAX_PATH_LENGTH / 2];
    get_sstable_path(sstable_path, sizeof(sstable_path));
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/L%d_%d.dat", sstable_path, level, sstable_id);

    FILE *fp = fopen(file_path, "rb");
    if (!fp) return NULL;

    int len;
    if (fread(&len, sizeof(int), 1, fp) == 1 && len > 0) {
        fseek(fp, len, SEEK_CUR);
    }
    if (fread(&len, sizeof(int), 1, fp) == 1 && len > 0) {
        fseek(fp, len, SEEK_CUR);
    }

    SSTableIterator *iterator = _create_iterator();
    if (!iterator) {
        fclose(fp);
        return NULL;
    }

    iterator->sstable_id = sstable_id;
    iterator->file = fp;
    iterator->level = level;
    return iterator;
}

static void _advance_iterator_and_update_heap(MinHeap *heap, SSTableIterator **iters, int source_idx) {
    if (_get_next_key_value_from_iterator(iters[source_idx]) == 0) {
        HeapNode *new_node = create_node_from_key_value_pair(iters[source_idx]->current_key, iters[source_idx]->current_value);
        new_node->source_index = source_idx;
        heap_replace_root(heap, new_node);
    } else {
        heap_extract_min(heap);
    }
}

HeapNode *get_next_merged_kv(MinHeap *heap, SSTableIterator **iters) {
    if (heap->size == 0) return NULL;

    HeapNode *winner = heap->nodes[0];
    int winner_origin = winner->source_index;

    _advance_iterator_and_update_heap(heap, iters, winner_origin);

    while (heap->size > 0 && strcmp(winner->key, heap->nodes[0]->key) == 0) {
        HeapNode *obsolete = heap->nodes[0];
        int obsolete_origin = obsolete->source_index;

        _advance_iterator_and_update_heap(heap, iters, obsolete_origin);

        free(obsolete->key);
        if (obsolete->value) free(obsolete->value);
        free(obsolete);
    }

    return winner;
}

void compact_sstables(int num_iters, int level_to_merge) {
    MinHeap *heap = create_min_heap(num_iters);
    heap->compare_func = lsm_compare_key_pairs;

    FILE *output_pointer = NULL;
    long current_file_size = 0;
    int file_count = get_sstable_count(level_to_merge + 1);
    char sstable_path[SSTABLE_MAX_PATH_LENGTH / 2];
    char output_path[256];

    char *min_key = NULL;
    char *max_key = NULL;
    BloomFilter *bloom_filter = NULL;

    HeapNode *kv;

    SSTableIterator **iters = malloc(sizeof(SSTableIterator *) * num_iters);
    if (!iters) {
        free(heap->nodes);
        free(heap);
        return;
    }

    int *real_ids = malloc(sizeof(int) * num_iters);

    pthread_mutex_lock(&_sstable_mutex);
    LevelIndex *level_idx = &_fast_access_sstables[level_to_merge];
    for(int i = 0; i < num_iters && i < level_idx->count; i++) {
        real_ids[i] = level_idx->tables[i]->id;
    }
    pthread_mutex_unlock(&_sstable_mutex);

    for (int i = 0; i < num_iters; i++) {
        SSTableIterator *tmp = _iterator_init(real_ids[i], level_to_merge);
        if (!tmp) {
            for (int j = 0; j < i; j++) {
                fclose(iters[j]->file);
                free(iters[j]->current_key);
                free(iters[j]->current_value);
                free(iters[j]);
            }
            free(iters);
            free(heap->nodes);
            free(heap);
            error("Failed to initialize iterator for SSTable L%d_%d.dat", level_to_merge, i);
            return;

        }
        iters[i] = tmp;
    }

    for (int i = 0; i < num_iters; i++) {
        if (_get_next_key_value_from_iterator(iters[i]) == 0) {
            HeapNode *node = create_node_from_key_value_pair(iters[i]->current_key, iters[i]->current_value);
            node->source_index = i;
            heap->nodes[heap->size++] = node;
        }
    }
    build_min_heap(heap);

    while ((kv = get_next_merged_kv(heap, iters)) != NULL) {
        int key_len = strlen(kv->key);
        int value_len = kv->value ? (int)strlen(kv->value) : -1;
        unsigned long record_size = sizeof(int) * 2 + key_len + (value_len > 0 ? value_len : 0);
        unsigned long max_file_size = MAX_MEMTABLE_SIZE * 10 * (level_to_merge + 1);

        if (!output_pointer || (current_file_size + record_size) > max_file_size) {
            // Register and closes the last sstable before creating a new one
            if (output_pointer) {
                register_new_sstable(level_to_merge + 1, file_count - 1, min_key, max_key, bloom_filter);
                free(min_key);
                free(max_key);
                min_key = max_key = NULL;
                fclose(output_pointer);
            }

            get_sstable_path(sstable_path, sizeof(sstable_path));

            sprintf(output_path, "%s/L%d_%d.dat", sstable_path, level_to_merge + 1, file_count++);
            output_pointer = fopen(output_path, "wb");
            current_file_size = 0;
            bloom_filter = bloom_filter_create();

            int dummy_header = 0;
            fwrite(&dummy_header, sizeof(int), 1, output_pointer);
            fwrite(&dummy_header, sizeof(int), 1, output_pointer);
            current_file_size += sizeof(int) * 2;

            info("Creating new file: %s", output_path);
        }

        if (!min_key) min_key = strdup(kv->key);
        if (max_key) free(max_key);
        max_key = strdup(kv->key);

        fwrite(&key_len, sizeof(int), 1, output_pointer);
        fwrite(kv->key, sizeof(char), key_len, output_pointer);
        fwrite(&value_len, sizeof(int), 1, output_pointer);
        if (value_len > 0) {
            fwrite(kv->value, sizeof(char), value_len, output_pointer);
        };
        if (bloom_filter) {
            bloom_add(bloom_filter, kv->key);
        }

        current_file_size += record_size;

        free(kv->key);
        free(kv->value);
        free(kv);
    }

    if (output_pointer) {
        register_new_sstable(level_to_merge + 1, file_count - 1, min_key, max_key, bloom_filter);
        free(min_key); free(max_key);
        fclose(output_pointer);
    }
    for (int i = 0; i < num_iters; i++) {
        if (iters[i]) {
            if (iters[i]->file) fclose(iters[i]->file);
            if (iters[i]->current_key) free(iters[i]->current_key);
            if (iters[i]->current_value) free(iters[i]->current_value);
            delete_sstable(level_to_merge, iters[i]->sstable_id);
            free(iters[i]);
        }
    }

    free(iters);
    free(heap->nodes);
    free(heap);
    free(real_ids);

    info("Compaction of level %d completed successfully.", level_to_merge);
}

void display_all_keys_unified() {
    int total_files = 0;
    for (int level = 0; level < MAX_SSTABLE_LEVELS; level++) {
        total_files += get_sstable_count(level);
    }

    MinHeap *heap = create_min_heap(total_files);
    heap->compare_func = lsm_compare_key_pairs;


    SSTableIterator **iters = malloc(sizeof(SSTableIterator *) * total_files);
    if (!iters) {
        free(heap->nodes);
        free(heap);
        return;
    }

    int iters_idx = 0;
    for (int level = 0; level < MAX_SSTABLE_LEVELS; level++) {
        for (int i = 0; i < get_sstable_count(level); i++) {
            SSTableIterator *tmp = _iterator_init(i, level);
            if (!tmp) {
                for (int j = 0; j < i; j++) {
                    fclose(iters[j]->file);
                    free(iters[j]->current_key);
                free(iters[j]->current_value);
                free(iters[j]);
                }
                free(iters);
                free(heap->nodes);
                free(heap);
                error("Failed to initialize iterator for SSTable L%d_%d.dat", level, i);
                return;
            }
            iters[iters_idx++] = tmp;
        }
    }

    for (int i = 0; i < total_files; i++) {
        if (_get_next_key_value_from_iterator(iters[i]) == 0) {
            HeapNode *node = create_node_from_key_value_pair(iters[i]->current_key, iters[i]->current_value);
            node->source_index = i;
            heap->nodes[heap->size++] = node;
        }
    }
    build_min_heap(heap);

    HeapNode *kv;
    while ((kv = get_next_merged_kv(heap, iters)) != NULL) {
        if (kv->value == NULL || strlen(kv->value) == 0) {
            printf("Key: %s | Value: [DELETED]\n", kv->key);
        } else {
            printf("Key: %s\n", kv->key);
        }

        free(kv->key);
        free(kv->value);
        free(kv);
    }

    for (int i = 0; i < iters_idx; i++) {
        if (iters[i]) {
            if (iters[i]->file) fclose(iters[i]->file);
            free(iters[i]->current_key);
            free(iters[i]->current_value);
            free(iters[i]);
        }
    }
    free(iters);
    free(heap->nodes);
    free(heap);
}
