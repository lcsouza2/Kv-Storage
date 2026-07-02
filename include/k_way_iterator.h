#ifndef K_WAY_ITERATOR_H
#define K_WAY_ITERATOR_H
#include <stdio.h>

typedef struct {
    FILE *file;
    char *current_key;
    char *current_value;
    int level;
    int sstable_id;
    int is_exhausted;
} SSTableIterator;


void compact_sstables(int num_iters, int level_to_merge);
void display_all_keys_unified();
#endif