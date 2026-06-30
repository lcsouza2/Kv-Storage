#ifndef SETTINGS_H
#define SETTINGS_H
#endif

#define MAX_SSTABLE_LEVEL_FILES 5
#define MAX_SSTABLE_LEVELS 7
#define MAX_SSTABLE_L0_FILE_SIZE 10485760 // 10MB
#define SSTABLE_MAX_PATH_LENGTH 256

void get_sstable_path(char *buffer, size_t buffer_size);
void get_wal_path(char *buffer, size_t buffer_size);
int create_data_storage_directory();
