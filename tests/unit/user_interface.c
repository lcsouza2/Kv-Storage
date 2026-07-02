#include "settings.h"
#include "test_utils.h"
#include "memtable.h"
#include "logging.h"
#include <stdlib.h>
#include <string.h>
#include "user_interface.h"

int test_memtable_autoflush() {
    Database *db = malloc(sizeof(Database));
    db->memtable = NULL;

    char *large_af_str = malloc(MAX_MEMTABLE_SIZE + 1);
    memset(large_af_str, 'A', MAX_MEMTABLE_SIZE);
    large_af_str[MAX_MEMTABLE_SIZE] = '\0';

    db_insert(db, "key1", large_af_str);
    db_insert(db, "key2", large_af_str);

    ASSERT_TEST(db->memtable->bytes_allocated <= MAX_MEMTABLE_SIZE, "Memtable should not exceed MAX_MEMTABLE_SIZE after multiple insertions.");
    success("test_memtable_autoflush passed.");
    clear_memtable(db->memtable);
    free(db->memtable);
    free(large_af_str);
    free(db);
    return 0;
}

int test_memtable_level_select() {
    Database *db = malloc(sizeof(Database));
    db->memtable = NULL;

    db_insert(db, "key1", "value1");
    db_insert(db, "key2", "value2");

    char *value1 = db_select(db, "key1");
    char *value2 = db_select(db, "key2");

    ASSERT_TEST(value1 && strcmp(value1, "value1") == 0, "Value for key1 should be 'value1'.");
    ASSERT_TEST(value2 && strcmp(value2, "value2") == 0, "Value for key2 should be 'value2'.");

    success("test_memtable_insert_and_select passed.");
    clear_memtable(db->memtable);
    free(db->memtable);
    free(db);
    return 0;
}

int test_sstable_level_select() {
    Database *db = malloc(sizeof(Database));
    db->memtable = NULL;

    char *large_af_str = malloc(MAX_MEMTABLE_SIZE + 1);
    memset(large_af_str, 'A', MAX_MEMTABLE_SIZE);
    large_af_str[MAX_MEMTABLE_SIZE] = '\0';

    db_insert(db, "key1", large_af_str);

    char *value1 = db_select(db, "key1");

    ASSERT_TEST(value1 && strcmp(value1, large_af_str) == 0, "Value for key1 should be 'AAAAAA...' from SSTable.");

    success("test_sstable_level_select passed.");
    free(value1);
    free(large_af_str);
    free(db->memtable);
    free(db);
    return 0;
}

int test_delete_inserts_tombstone() {
    Database *db = malloc(sizeof(Database));
    db->memtable = NULL;

    db_insert(db, "key1", "value1");
    db_delete(db, "key1");

    char *value = db_select(db, "key1");
    ASSERT_TEST(value == NULL, "Deleted key should return NULL on select.");

    success("test_delete_inserts_tombstone passed.");
    free(value);
    clear_memtable(db->memtable);
    free(db->memtable);
    free(db);
    return 0;
}