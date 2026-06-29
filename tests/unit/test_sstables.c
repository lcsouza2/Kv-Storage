#include "sstables.h"
#include "logging.h"
#include "test_utils.h"
#include <stdio.h>
#include <stdlib.h>

int test_create_sstable_success() {
    SSTable *sstable = create_sstable("Apple", "Red", 0);

    ASSERT_TEST(sstable != NULL, "Struct returned NULL");
    ASSERT_TEST(sstable->min_key == 10, "Wrong min_key assigned");
    ASSERT_TEST(sstable->level == 0, "Wrong level assigned");

    FILE *fp = fopen(sstable->path, "rb");
    ASSERT_TEST(fp != NULL, "File was not created on disk");
    if (fp) fclose(fp);

    free(sstable->path);
    free(sstable);

    info("test_create_sstable_success passed.");
    return 0;
}