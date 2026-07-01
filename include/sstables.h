#ifndef SSTABLES_H
#define SSTABLES_H
#include "memtable.h"
#include "bloom_filter.h"
#include "utils.h"

typedef struct sstable {
    char *path;
    char *min_key;
    char *max_key;
    int level;
    int id;
    BloomFilter *bloom_filter;
} SSTable;

typedef struct level_index {
    int count;
    int capacity;
    SSTable **tables;
} LevelIndex;

int flush_memtable_to_disk(Memtable *memtable, int level);
SearchResult search_in_sstables(char *key);
int sync_sstables_from_manifest();
int register_new_sstable(int level, int id, char *min_key, char *max_key, BloomFilter *bloom_filter);
#endif
