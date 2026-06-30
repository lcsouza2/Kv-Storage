#include "test_utils.h"
#include "memtable.h"
#include <stdlib.h>
#include "logging.h"
#include <string.h>

int test_memtable_create() {
    Memtable *memtable = create_memtable();
    ASSERT_TEST(memtable != NULL, "Failed to create memtable.");
    ASSERT_TEST(memtable->root == NULL, "Newly created memtable should have a NULL root.");
    ASSERT_TEST(memtable->min_key == NULL, "Newly created memtable should have a NULL min_key.");
    ASSERT_TEST(memtable->max_key == NULL, "Newly created memtable should have a NULL max_key.");
    free_memtable(memtable);
    info("test_memtable_create passed.");
    return 0;
}

int test_memtable_insert() {
    Memtable *memtable = create_memtable();
    insert_memtable(memtable, "key1", "value1");
    insert_memtable(memtable, "key2", "value2");
    insert_memtable(memtable, "key3", "value3");

    ASSERT_TEST(memtable->root != NULL, "Root should not be NULL after insertions.");
    ASSERT_TEST(strcmp(memtable->root->key, "key2") == 0, "Root key should be 'key2'.");
    ASSERT_TEST(strcmp(memtable->root->left->key, "key1") == 0, "Left child of root should be 'key1'.");
    ASSERT_TEST(strcmp(memtable->root->right->key, "key3") == 0, "Right child of root should be 'key3'.");

    free_memtable(memtable);
    info("test_memtable_insert passed.");
    return 0;
}

int test_memtable_min_max_keys() {
    Memtable *memtable = create_memtable();
    insert_memtable(memtable, "key2", "value2");
    insert_memtable(memtable, "key1", "value1");
    insert_memtable(memtable, "key3", "value3");

    ASSERT_TEST(strcmp(memtable->min_key, "key1") == 0, "Min key should be 'key1'.");
    ASSERT_TEST(strcmp(memtable->max_key, "key3") == 0, "Max key should be 'key3'.");

    free_memtable(memtable);
    info("test_memtable_min_max_keys passed.");
    return 0;
}

int test_memtable_tacks_min_max_key_in_o1() {
    Memtable *memtable = create_memtable();

    insert_memtable(memtable, "dog", "cachorro");
    ASSERT_TEST(strcmp(memtable->min_key, "dog") == 0, "Min key should be 'dog' after first insertion.");
    ASSERT_TEST(strcmp(memtable->max_key, "dog") == 0, "Max key should be 'dog' after first insertion.");

    insert_memtable(memtable, "apple", "maca");
    ASSERT_TEST(strcmp(memtable->min_key, "apple") == 0, "Min key should be 'apple' after second insertion.");
    ASSERT_TEST(strcmp(memtable->max_key, "dog") == 0, "Max key should be 'dog' after second insertion.");

    insert_memtable(memtable, "zebra", "listrada");
    ASSERT_TEST(strcmp(memtable->min_key, "apple") == 0, "Min key should be 'apple' after third insertion.");
    ASSERT_TEST(strcmp(memtable->max_key, "zebra") == 0, "Max key should be 'zebra' after third insertion.");

    insert_memtable(memtable, "cat", "gato");
    ASSERT_TEST(strcmp(memtable->min_key, "apple") == 0, "Min key should be 'apple' after fourth insertion.");
    ASSERT_TEST(strcmp(memtable->max_key, "zebra") == 0, "Max key should be 'zebra' after fourth insertion.");
    return 0;
}