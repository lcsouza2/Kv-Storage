#include "logging.h"
#include "test_utils.h"
#include "memtable.h"
#include <string.h>
#include <stdlib.h>

int test_tree_insertion() {
    Memtable *memtable = create_memtable();

    insert_memtable(memtable, "30", "thirty");
    insert_memtable(memtable, "20", "twenty");
    insert_memtable(memtable, "40", "forty");
    insert_memtable(memtable, "10", "ten");
    insert_memtable(memtable, "25", "twenty-five");

    ASSERT_TEST(memtable->root != NULL, "Root should not be NULL after insertions.");
    ASSERT_TEST(strcmp(memtable->root->value, "thirty") == 0, "Root value should be 'thirty'.");
    ASSERT_TEST(strcmp(memtable->root->left->value, "twenty") == 0, "Left child of root should be 'twenty'.");
    ASSERT_TEST(strcmp(memtable->root->right->value, "forty") == 0, "Right child of root should be 'forty'.");
    ASSERT_TEST(strcmp(memtable->root->left->left->value, "ten") == 0, "Left child of left child of root should be 'ten'.");
    ASSERT_TEST(strcmp(memtable->root->left->right->value, "twenty-five") == 0, "Right child of left child of root should be 'twenty-five'.");

    clear_memtable(memtable);
    free(memtable);
    success("test_tree_insertion passed.");
    return 0;
}

int test_tree_left_rotation() {
    Memtable *memtable = create_memtable();

    insert_memtable(memtable, "10", "ten");
    insert_memtable(memtable, "20", "twenty");
    insert_memtable(memtable, "30", "thirty"); // This should trigger a left rotation

    ASSERT_TEST(strcmp(memtable->root->value, "twenty") == 0, "Root value should be 'twenty' after left rotation.");
    ASSERT_TEST(strcmp(memtable->root->left->value, "ten") == 0, "Left child of root should be 'ten' after left rotation.");
    ASSERT_TEST(strcmp(memtable->root->right->value, "thirty") == 0, "Right child of root should be 'thirty' after left rotation.");

    clear_memtable(memtable);
    free(memtable);
    success("test_tree_left_rotation passed.");
    return 0;
}

int test_tree_right_rotation() {
    Memtable *memtable = create_memtable();

    insert_memtable(memtable, "30", "thirty");
    insert_memtable(memtable, "20", "twenty");
    insert_memtable(memtable, "10", "ten"); // This should trigger a right rotation

    ASSERT_TEST(strcmp(memtable->root->value, "twenty") == 0, "Root value should be 'twenty' after right rotation.");
    ASSERT_TEST(strcmp(memtable->root->left->value, "ten") == 0, "Left child of root should be 'ten' after right rotation.");
    ASSERT_TEST(strcmp(memtable->root->right->value, "thirty") == 0, "Right child of root should be 'thirty' after right rotation.");

    clear_memtable(memtable);
    free(memtable);
    success("test_tree_right_rotation passed.");
    return 0;
}

int test_tree_double_left_rotation() {
    Memtable *memtable = create_memtable();

    insert_memtable(memtable, "10", "ten");
    insert_memtable(memtable, "30", "thirty");
    insert_memtable(memtable, "20", "twenty"); // This should trigger a double left rotation

    ASSERT_TEST(strcmp(memtable->root->value, "twenty") == 0, "Root value should be 'twenty' after double left rotation.");
    ASSERT_TEST(strcmp(memtable->root->left->value, "ten") == 0, "Left child of root should be 'ten' after double left rotation.");
    ASSERT_TEST(strcmp(memtable->root->right->value, "thirty") == 0, "Right child of root should be 'thirty' after double left rotation.");

    clear_memtable(memtable);
    free(memtable);
    success("test_tree_double_left_rotation passed.");
    return 0;
}

int test_tree_double_right_rotation() {
    Memtable *memtable = create_memtable();

    insert_memtable(memtable, "30", "thirty");
    insert_memtable(memtable, "10", "ten");
    insert_memtable(memtable, "20", "twenty"); // This should trigger a double right rotation

    ASSERT_TEST(strcmp(memtable->root->value, "twenty") == 0, "Root value should be 'twenty' after double right rotation.");
    ASSERT_TEST(strcmp(memtable->root->left->value, "ten") == 0, "Left child of root should be 'ten' after double right rotation.");
    ASSERT_TEST(strcmp(memtable->root->right->value, "thirty") == 0, "Right child of root should be 'thirty' after double right rotation.");

    clear_memtable(memtable);
    free(memtable);
    success("test_tree_double_right_rotation passed.");
    return 0;
}