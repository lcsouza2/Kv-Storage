#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "settings.h"

typedef struct  bloom_filter {
    uint8_t *bit_array;
} BloomFilter;

static uint32_t hash_djb2(const char *str) {
    /*
        This is the djb2 hash function by Dan Bernstein.
        Notes:
        - 5381 << 5 = 5381 * 32, so the line hash = ((hash << 5) + hash) + c; is equivalent to hash = hash * 33 + c;
    */
    uint32_t hash = 5381;
    int c;
    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

static uint32_t hash_sdbm(const char *str) {
    /*
        This is the sdbm hash function.
        It was created for sdbm (a public-domain reimplementation of ndbm) database library.
        It has been found to do well in scrambling bits, causing better distribution of the keys and fewer splits.
        Notes:
        - The line hash = c + (hash << 6) + (hash << 16) - hash; is equivalent to hash = c + hash * 65599 since 65599 = 2^16 + 2^6 - 1;

    */
    uint32_t hash = 0;
    int c;
    while ((c = (unsigned char)*str++)) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

static uint32_t hash_jenkins(const char *str) {
    /*
        This is the jenkins hash function.
        It is a non-cryptographic hash function that is known for its good performance and distribution.
        Notes:
        - The algorithm uses a series of bitwise operations to mix the input bits and produce a hash value.
        - It first adds the character to the hash, then performs a series of shifts and XORs to mix the bits.
    */
    uint32_t hash = 0;
    while (*str) {
        hash += (unsigned char)*str++;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

/**
 * @brief Creates a new Bloom filter.
 * @return (BloomFilter *): Pointer to the newly created Bloom filter, or NULL on failure.
 */
BloomFilter *bloom_filter_create() {
    BloomFilter *filter = malloc(sizeof(BloomFilter));
    if (!filter) return NULL;

    filter->bit_array = calloc(BLOOM_FILTER_SIZE_BYTES, sizeof(uint8_t));
    if (!filter->bit_array) {
        free(filter);
        return NULL;
    }

    return filter;
}

/**
 * @brief Adds a key to the Bloom filter.
 * @param filter (BloomFilter *): Pointer to the Bloom filter.
 * @param key (const char *): The key to add.
 */
void bloom_add(BloomFilter *filter, const char *key) {
    if (!filter || !key) return;

    uint32_t hash1 = hash_djb2(key) % BLOOM_FILTER_SIZE_BITS;
    uint32_t hash2 = hash_sdbm(key) % BLOOM_FILTER_SIZE_BITS;
    uint32_t hash3 = hash_jenkins(key) % BLOOM_FILTER_SIZE_BITS;

    filter->bit_array[hash1 / 8] |= (1 << (hash1 % 8));
    filter->bit_array[hash2 / 8] |= (1 << (hash2 % 8));
    filter->bit_array[hash3 / 8] |= (1 << (hash3 % 8));
}

/**
 * @brief Checks if a key is possibly in the Bloom filter.
 * @param filter (BloomFilter *): Pointer to the Bloom filter.
 * @param key (const char *): The key to check.
 * @return (int): Returns 1 if the key is possibly in the filter, 0 if definitely not.
 */
int bloom_check(BloomFilter *filter, const char *key) {
    if (!filter || !key) return 0;

    uint32_t hash1 = hash_djb2(key) % BLOOM_FILTER_SIZE_BITS;
    uint32_t hash2 = hash_sdbm(key) % BLOOM_FILTER_SIZE_BITS;
    uint32_t hash3 = hash_jenkins(key) % BLOOM_FILTER_SIZE_BITS;

    return (filter->bit_array[hash1 / 8] & (1 << (hash1 % 8))) &&
           (filter->bit_array[hash2 / 8] & (1 << (hash2 % 8))) &&
           (filter->bit_array[hash3 / 8] & (1 << (hash3 % 8)));
}

/**
 * @brief Frees the memory allocated for the Bloom filter.
 * @param filter (BloomFilter *): Pointer to the Bloom filter to free.
 */
void free_bloom_filter(BloomFilter *filter) {
    if (filter) {
        free(filter->bit_array);
        free(filter);
    }
}