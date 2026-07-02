#include "sstables.h"
#include "logging.h"
#include "test_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "settings.h"
#include "user_interface.h"

int test_flush_memtable_to_disk_success() {
    Memtable *memtable = insert_memtable(NULL, "key1", "value1");
    insert_memtable(memtable, "key2", "value2");
    insert_memtable(memtable, "key3", "value3");

    int result = flush_memtable_to_disk(memtable, 0);

    ASSERT_TEST(result == 0, "Failed to flush memtable to disk.");

    // Clean up
    clear_memtable(memtable);

    success("test_flush_memtable_to_disk_success passed.");
    free(memtable);
    return 0;
}

int test_sstable_search() {
    char *large_af_str = malloc(MAX_MEMTABLE_SIZE + 1);
    memset(large_af_str, 'A', MAX_MEMTABLE_SIZE);
    large_af_str[MAX_MEMTABLE_SIZE] = '\0';

    Database *db = malloc(sizeof(Database));
    db->memtable = NULL;
    db_insert(db, "special_key", large_af_str);

    SearchResult result = search_in_sstables("special_key");

    ASSERT_TEST(result.found == 1, "Search result should be marked as found for existing key in SSTables.");
    ASSERT_TEST(result.value != NULL, "Search result value should not be NULL for existing key in SSTables.");
    ASSERT_TEST(strcmp(result.value, large_af_str) == 0, "Value for 'special_key' should match the inserted value in SSTables.");

    success("test_sstable_search passed.");

    free(large_af_str);
    free(result.value);
    clear_memtable(db->memtable);
    free(db->memtable);
    free(db);
    return 0;
}