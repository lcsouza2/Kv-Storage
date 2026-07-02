#include <stdio.h>
#include <string.h>
#include "logging.h"
#include "sstables.h"
#include "settings.h"
#include "test_utils.h"

extern int test_flush_memtable_to_disk_success();
extern int test_tree_insertion();
extern int test_tree_left_rotation();
extern int test_tree_right_rotation();
extern int test_tree_double_left_rotation();
extern int test_tree_double_right_rotation();
extern int test_memtable_create();
extern int test_memtable_insert();
extern int test_memtable_min_max_keys();
extern int test_memtable_tracks_min_max_key_in_o1();
extern int test_memtable_search_existing_key();
extern int test_memtable_search_nonexistent_key();
extern int test_memtable_delete_existing_key();
extern int test_memtable_delete_nonexistent_key();
extern int test_memtable_update_existing_key();
extern int test_memtable_autoflush();
extern int test_sstable_search();
extern int test_bloom_filter_rejects_random_key();
extern int test_bloom_filter_accepts_existing_key();
extern int test_wal_syncing_success();
extern int test_memtable_level_select();
extern int test_sstable_level_select();
extern int test_delete_inserts_tombstone();
extern int test_merge_k_vias_success();
extern int test_display_all_keys_unified();
extern int test_auto_compaction();
extern int test_stress_async_insertions();

int main(int argc, char *argv[]) {
    int failed = 0;

    create_data_storage_directory();

    if (argc > 1 && strcmp(argv[1], "--stress") == 0) {
        if (test_stress_async_insertions() != 0) failed++;
        return 0;
    }

    info("Running tests...");

    if (test_flush_memtable_to_disk_success() != 0) failed++;
    clear_sstables_after_test();

    if (test_tree_insertion() != 0) failed++;
    if (test_tree_left_rotation() != 0) failed++;
    if (test_tree_right_rotation() != 0) failed++;
    if (test_tree_double_left_rotation() != 0) failed++;
    if (test_tree_double_right_rotation() != 0) failed++;
    clear_sstables_after_test();

    if (test_memtable_create() != 0) failed++;
    if (test_memtable_insert() != 0) failed++;
    if (test_memtable_min_max_keys() != 0) failed++;
    if (test_memtable_tracks_min_max_key_in_o1() != 0) failed++;
    if (test_memtable_update_existing_key() != 0) failed++;
    if (test_memtable_delete_existing_key() != 0) failed++;
    if (test_memtable_delete_nonexistent_key() != 0) failed++;
    if (test_memtable_search_existing_key() != 0) failed++;
    if (test_memtable_search_nonexistent_key() != 0) failed++;
    if (test_memtable_autoflush() != 0) failed++;
    clear_sstables_after_test();

    if (test_sstable_search() != 0) failed++;
    clear_sstables_after_test();

    if (test_bloom_filter_rejects_random_key() != 0) failed++;
    if (test_bloom_filter_accepts_existing_key() != 0) failed++;
    clear_sstables_after_test();

    if (test_wal_syncing_success() != 0) failed++;
    clear_sstables_after_test();

    if (test_memtable_level_select() != 0) failed++;
    if (test_sstable_level_select() != 0) failed++;
    clear_sstables_after_test();

    if (test_delete_inserts_tombstone() != 0) failed++;
    clear_sstables_after_test();

    if (test_merge_k_vias_success() != 0) failed++;
    clear_sstables_after_test();

    if (test_display_all_keys_unified() != 0) failed++;
    clear_sstables_after_test();

    if (test_auto_compaction() != 0) failed++;
    clear_sstables_after_test();

    if (failed > 0) {
        error("%d test(s) failed.", failed);
        return -1;
    }

    success("All tests passed!");
    return 0;
}