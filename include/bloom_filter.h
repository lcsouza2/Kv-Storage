#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H
#endif

#include <stdint.h>

typedef struct  bloom_filter {
    uint8_t *bit_array;
} BloomFilter;


BloomFilter *bloom_filter_create();

void bloom_add(BloomFilter *filter, const char *key);
int bloom_check(BloomFilter *filter, const char *key);