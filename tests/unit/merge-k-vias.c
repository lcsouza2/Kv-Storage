#include "k_way_iterator.h"
#include "settings.h"
#include "test_utils.h"
#include "logging.h"
#include "user_interface.h"
#include <stdlib.h>
#include <stdio.h>
#include "string.h"

int test_merge_k_vias_success() {
    char *large_af_str = malloc(MAX_MEMTABLE_SIZE + 1);
    memset(large_af_str, '.', MAX_MEMTABLE_SIZE);
    large_af_str[MAX_MEMTABLE_SIZE] = '\0';

    Database *db = malloc(sizeof(Database));
    db->memtable = NULL;
    db_insert(db, "key_1", large_af_str);
    db_insert(db, "key_2", large_af_str);
    db_insert(db, "key_3", large_af_str);

    compact_sstables(3, 0);

    db_select(db, "key_1");
    db_select(db, "key_2");
    db_select(db, "key_3");

    free(large_af_str);
    return 0;
}

int test_display_all_keys_unified() {
    Database *db = malloc(sizeof(Database));
    db->memtable = NULL;

    char *large_af_str = malloc(MAX_MEMTABLE_SIZE + 1);
    memset(large_af_str, '.', MAX_MEMTABLE_SIZE);
    large_af_str[MAX_MEMTABLE_SIZE] = '\0';

    db_insert(db, "key_1", large_af_str);
    db_insert(db, "key_2", large_af_str);
    db_insert(db, "key_3", large_af_str);

    compact_sstables(3, 0);

    free(large_af_str);
    display_all_keys_unified();

    return 0;
}