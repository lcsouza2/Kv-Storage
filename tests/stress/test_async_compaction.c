#include "logging.h"
#include "user_interface.h"
#include "compactor.h"
#include "test_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int test_stress_async_insertions() {
    Database *db = malloc(sizeof(Database));
    init_background_compactor();

    int TOTAL_KEYS = 20000;

    for (int i = 0; i < TOTAL_KEYS; i++) {
        char key[32];
        char value[64];
        sprintf(key, "key_%06d", i);
        sprintf(value, "value_to_key_%d", i);

        db_insert(db, key, value);
    }

    debug("Finished inserting %d keys. Triggering background compaction...", TOTAL_KEYS);

    shutdown_background_compactor();

    for (int i = 0; i < TOTAL_KEYS; i += 100) {
        char key[32];
        char expected_value[64];
        sprintf(key, "key_%06d", i);
        sprintf(expected_value, "value_to_key_%d", i);

        char *result = db_select(db, key);
        ASSERT_TEST(result != NULL, "Expected non-NULL result for key after insertions and compaction.");
        ASSERT_TEST(strcmp(result, expected_value) == 0, "Retrieved value does not match expected value.");
        free(result);
    }

    free(db->memtable);
    free(db);
    success("test_stress_async_insertions passed\n");
    return 0;
}