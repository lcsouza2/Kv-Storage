#include "logging.h"
#include "sstables.h"
#include "memtable.h"
#include "bloom_filter.h"
#include "test_utils.h"

int test_bloom_filter_rejects_random_key() {
    BloomFilter *filter = bloom_filter_create();
    bloom_add(filter, "existing_key");

    int result = bloom_check(filter, "random_key");
    ASSERT_TEST(result == 0, "Bloom filter should reject a random key that was not added.");

    success("test_bloom_filter_rejects_random_key passed.");
    return 0;
}

int test_bloom_filter_accepts_existing_key() {
    BloomFilter *filter = bloom_filter_create();
    bloom_add(filter, "existing_key");

    int result = bloom_check(filter, "existing_key");
    ASSERT_TEST(result == 1, "Bloom filter should accept an existing key that was added.");

    success("test_bloom_filter_accepts_existing_key passed.");
    return 0;
}