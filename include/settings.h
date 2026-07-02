#ifndef SETTINGS_H
#define SETTINGS_H
int get_memtable_size();
int get_max_log_len();
int get_debug_logs();
#include <stdlib.h>

#define MAX_SSTABLE_LEVEL_FILES 5
#define MAX_SSTABLE_LEVELS 7
#define MAX_MEMTABLE_SIZE get_memtable_size() // 5MB || defined by environment variable MEMTABLE_SIZE
#define SSTABLE_MAX_PATH_LENGTH 256
#define WAL_MAX_PATH_LENGTH 256
#define MAX_LOG_LEN get_max_log_len()
#define DEBUG_LOGS get_debug_logs()

#define MAX_KEY_LENGTH 256

#define BLOOM_FILTER_SIZE_BITS 2097152 // Size of the Bloom filter in bits
#define BLOOM_FILTER_SIZE_BYTES (BLOOM_FILTER_SIZE_BITS / 8) // Size of the Bloom filter in bytes

void get_sstable_path(char *buffer, size_t buffer_size);
void get_wal_path(char *buffer, size_t buffer_size);
int create_data_storage_directory();
#endif
