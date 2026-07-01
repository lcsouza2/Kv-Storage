#include "test_utils.h"
#include "memtable.h"
#include "logging.h"
#include "wal.h"
#include <stdlib.h>
#include <string.h>
#include "user_interface.h"

int test_wal_syncing_success() {
    Database *db = malloc(sizeof(Database));
    db->memtable = NULL;

    db_insert(db, "key1", "value1");
    db_insert(db, "key2", "value2");
    db_insert(db, "key3", "value3");
    clear_memtable(db->memtable);

    int sync_result = sync_wal_to_memtable(db->memtable);
    ASSERT_TEST(sync_result == 0, "Failed to sync WAL to memtable.");

    SearchResult result1 = search_memtable(db->memtable, "key1");
    SearchResult result2 = search_memtable(db->memtable, "key2");
    SearchResult result3 = search_memtable(db->memtable, "key3");

    ASSERT_TEST(result1.found == 1 && result1.value != NULL && strcmp(result1.value, "value1") == 0, "Key 'key1' not found or value mismatch.");
    ASSERT_TEST(result2.found == 1 && result2.value != NULL && strcmp(result2.value, "value2") == 0, "Key 'key2' not found or value mismatch.");
    ASSERT_TEST(result3.found == 1 && result3.value != NULL && strcmp(result3.value, "value3") == 0, "Key 'key3' not found or value mismatch.");

    clear_memtable(db->memtable);
    free(db->memtable);
    free(db);

    success("test_wal_syncing_success passed.");
    return 0;
}