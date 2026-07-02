#include "k_way_iterator.h"
#include "settings.h"
#include "test_utils.h"
#include "logging.h"
#include "user_interface.h"
#include <stdlib.h>
#include <stdio.h>
#include "string.h"
#include "compactor.h"

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

    char *value_1 = db_select(db, "key_1");
    char *value_2 = db_select(db, "key_2");
    char *value_3 = db_select(db, "key_3");

    ASSERT_TEST(value_1 != NULL, "Key 'key_1' should exist after compaction.");
    ASSERT_TEST(value_2 != NULL, "Key 'key_2' should exist after compaction.");
    ASSERT_TEST(value_3 != NULL, "Key 'key_3' should exist after compaction.");

    free(large_af_str);
    free(value_1);
    free(value_2);
    free(value_3);
    free(db->memtable);
    free(db);
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


    display_all_keys_unified();
    free(large_af_str);
    free(db->memtable);
    free(db);
    return 0;
}

int test_auto_compaction() {
    Database *db = malloc(sizeof(Database));
    db->memtable = NULL;
    init_background_compactor();

    char *large_af_str = malloc(MAX_MEMTABLE_SIZE + 1);
    memset(large_af_str, '.', MAX_MEMTABLE_SIZE);
    large_af_str[MAX_MEMTABLE_SIZE] = '\0';

    for (int i = 0; i < MAX_SSTABLE_LEVEL_FILES + 1; i++) {
        debug("Loop iteration %d: Inserting key_%d", i, i);
        char key[20];
        snprintf(key, sizeof(key), "key_%d", i);
        db_insert(db, key, large_af_str);
    }
    shutdown_background_compactor();

    FILE *l1 = fopen("./tests/data/sstables/L1_0.dat", "rb");
    ASSERT_TEST(l1 != NULL, "L1_0.dat should exist after auto compaction.");
    ASSERT_TEST(get_sstable_count(1) != 0, "Level 1 should have 1 sstable after auto compaction.");
    free(large_af_str);
    free(db->memtable);
    free(db);
    fclose(l1);

    return 0;
}