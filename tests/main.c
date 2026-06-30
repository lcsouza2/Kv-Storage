#include <stdio.h>
#include "logging.h"
#include "sstables.h"
#include "settings.h"

extern int test_flush_memtable_to_disk_success();
extern int test_tree_insertion();
extern int test_tree_left_rotation();
extern int test_tree_right_rotation();
extern int test_tree_double_left_rotation();
extern int test_tree_double_right_rotation();
extern int test_memtable_create();
extern int test_memtable_insert();
extern int test_memtable_min_max_keys();
extern int test_memtable_tacks_min_max_key_in_o1();

int main() {
    int failed = 0;

    create_data_storage_directory();

    info("Running tests...");

    if (test_flush_memtable_to_disk_success() != 0) failed++;
    if (test_tree_insertion() != 0) failed++;
    if (test_tree_left_rotation() != 0) failed++;
    if (test_tree_right_rotation() != 0) failed++;
    if (test_tree_double_left_rotation() != 0) failed++;
    if (test_tree_double_right_rotation() != 0) failed++;
    if (test_memtable_create() != 0) failed++;
    if (test_memtable_insert() != 0) failed++;
    if (test_memtable_min_max_keys() != 0) failed++;
    if (test_memtable_tacks_min_max_key_in_o1() != 0) failed++;

    if (failed > 0) {
        error("%d test(s) failed.", failed);
        return -1;
    }

    info("All tests passed!");
    return 0;
}