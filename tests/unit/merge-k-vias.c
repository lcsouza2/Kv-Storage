#include "k_way.h"
#include "settings.h"
#include "test_utils.h"
#include "logging.h"
#include "user_interface.h"
#include <stdlib.h>
#include <stdio.h>

int test_merge_k_vias_success() {
    SSTableIterator *iterators[3];
    iterators[0] = _create_iterator();
    iterators[1] = _create_iterator();
    iterators[2] = _create_iterator();

    char *large_af_str = malloc(MAX_MEMTABLE_SIZE + 1);
    memset(large_af_str, '.', MAX_MEMTABLE_SIZE);
    large_af_str[MAX_MEMTABLE_SIZE] = '\0';

    Database *db = malloc(sizeof(Database));
    db->memtable = NULL;
    db_insert(db, "key_1", large_af_str);
    db_insert(db, "key_2", "value_2");
    db_insert(db, "key_3", "value_3");

    compact_sstables(iterators, 3, "test_output");

    db_select(db, "key_1");
    db_select(db, "key_2");
    db_select(db, "key_3");

    return 0;
}