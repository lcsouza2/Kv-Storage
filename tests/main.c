#include <stdio.h>
#include "logging.h"
#include "sstables.h"

extern int test_create_sstable_success();
extern int test_tree_insertion();
extern int test_tree_left_rotation();
extern int test_tree_right_rotation();
extern int test_tree_double_left_rotation();
extern int test_tree_double_right_rotation();

int main() {
    int failed = 0;

    info("Running tests...");

    if (test_create_sstable_success() != 0) failed++;
    if (test_tree_insertion() != 0) failed++;
    if (test_tree_left_rotation() != 0) failed++;
    if (test_tree_right_rotation() != 0) failed++;
    if (test_tree_double_left_rotation() != 0) failed++;
    if (test_tree_double_right_rotation() != 0) failed++;

    if (failed > 0) {
        error("%d test(s) failed.", failed);
        return -1;
    }

    info("All tests passed!");
    return 0;
}