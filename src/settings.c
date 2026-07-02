#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include "logging.h"

static char *_get_data_storage_path() {
    char *path = getenv("DATA_PATH");
    if (path == NULL) {
        return "./data";
    }
    return path;
}

// INTERFACE FUNCTIONS =============================

/**
 * @brief Retrieves the path for storing SSTables.
 * @param buffer (char *): The buffer to store the path.
 * @param buffer_size (size_t): The size of the buffer.
 */
void get_sstable_path(char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return;
    const char *base_path = _get_data_storage_path();
    snprintf(buffer, buffer_size, "%s/sstables", base_path);
}

/**
 * @brief Retrieves the path for the Write-Ahead Log (WAL).
 * @param buffer (char *): The buffer to store the path.
 * @param buffer_size (size_t): The size of the buffer.
 */
void get_wal_path(char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return;
    const char *base_path = _get_data_storage_path();
    snprintf(buffer, buffer_size, "%s/database.wal", base_path);
}

/**
 * @brief Creates the data storage directory and its subdirectories for SSTables and WAL.
 * @return (int) 0 on success, -1 on failure.
 */
int create_data_storage_directory() {
    char sstables_path[256];
    char wal_path[256];
    char *data_path = _get_data_storage_path();
    get_sstable_path(sstables_path, sizeof(sstables_path));
    get_wal_path(wal_path, sizeof(wal_path));

    if (mkdir(data_path, 0755) == -1 && errno != EEXIST) {
        error("Failed to create data directory");
        return -1;
    }

    if (mkdir(sstables_path, 0755) == -1 && errno != EEXIST) {
        error("Failed to create sstables directory");
        return -1;
    }

    return 0;
}

/**
 * @brief Retrieves the configured size for the Memtable.
 * @return (int): The size of the Memtable in bytes. Defaults to 5MB if not set.
 */
int get_memtable_size() {
    if (getenv("MEMTABLE_SIZE")) {
        return atoi(getenv("MEMTABLE_SIZE"));
    }
    return 5242880; // Default to 5MB if not set
}

int get_debug_logs() {
    if (getenv("DEBUG_LOGS")) {
        return atoi(getenv("DEBUG_LOGS"));
    }
    return 0; // Default to 0 (disabled) if not set
}

int get_max_log_len() {
    if (getenv("MAX_LOG_LEN")) {
        return atoi(getenv("MAX_LOG_LEN"));
    }
    return 512; // Default value
}