#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "settings.h"
#include "logging.h"
#include "memtable.h"

static FILE *_open_wal_file(char *mode) {
    char wal_path[WAL_MAX_PATH_LENGTH];
    get_wal_path(wal_path, sizeof(wal_path));
    FILE *file = fopen(wal_path, mode);
    if (!file) {
        error("Failed to open WAL file: %s", wal_path);
        return NULL;
    }
    return file;
}

static void _read_wal_logs(void (*callback)(Memtable *memtable, char *key, char *value), Memtable *memtable) {
    FILE *wal_file = _open_wal_file("rb");
    if (!wal_file) return;

    while (1) {
        int key_length;
        if (fread(&key_length, sizeof(int), 1, wal_file) != 1) break;

        char *key = malloc(key_length + 1);
        if (!key) {
            error("Memory allocation failed while reading WAL logs.");
            break;
        }
        fread(key, sizeof(char), key_length, wal_file);
        key[key_length] = '\0';

        int value_length;
        fread(&value_length, sizeof(int), 1, wal_file);

        char *value = NULL;
        if (value_length > 0) {
            value = malloc(value_length + 1);
            if (!value) {
                error("Memory allocation failed while reading WAL logs.");
                free(key);
                break;
            }
            fread(value, sizeof(char), value_length, wal_file);
            value[value_length] = '\0';
        }

        callback(memtable, key, value);

        free(key);
        if (value) free(value);
    }

    fclose(wal_file);
}

static void _wal_callback(Memtable *memtable, char *key, char *value) {
    if (!memtable) return;

    if (value) {
        insert_memtable(memtable, key, value);
        info("Replayed WAL log: %s -> %s", key, value);
    } else {
        delete_from_memtable(memtable, key);
        info("Replayed WAL log: %s -> TOMBSTONE", key);
    }
}

// INTERFACE FUNCTIONS =============================

/**
 * @brief Appends a key-value pair to the Write-Ahead Log (WAL).
 * If the value is NULL, it indicates a deletion (tombstone).
 * @param key (char*): The key to be logged.
 * @param value (char*): The value to be logged. If NULL, it indicates a deletion.
 * @return (int) 0 on success, -1 on failure.
 */
int append_wal_log(char *key, char *value) {
    FILE *wal_file = _open_wal_file("ab");
    if (!wal_file) return -1;
    int key_length = strlen(key);
    int value_length = (value != NULL) ? (int)strlen(value) : -1; // -1 indicates a tombstone (deletion)
    fwrite(&key_length, sizeof(int), 1, wal_file);
    fwrite(key, sizeof(char), key_length, wal_file);
    fwrite(&value_length, sizeof(int), 1, wal_file);
    if (value_length > 0) {
        fwrite(value, sizeof(char), value_length, wal_file);
    }

    fflush(wal_file);
    fsync(fileno(wal_file));

    fclose(wal_file);
    return 0;
}

/**
 * @brief Clears the Write-Ahead Log (WAL) by truncating the file.
 * @return (int) 0 on success, -1 on failure.
 */
int clear_wal_logs() {
    FILE *wal_file = _open_wal_file("wb");
    if (!wal_file) return -1;
    fclose(wal_file);
    return 0;
}

/**
 * @brief Synchronizes the Write-Ahead Log (WAL) with the provided Memtable.
 * @param memtable (Memtable *): The Memtable to synchronize with the WAL.
 * @return (int) 0 on success, -1 on failure.
 */
int sync_wal_to_memtable(Memtable *memtable) {
    if (!memtable) {
        debug("Memtable is NULL in sync_wal_to_memtable.");
        return -1;
    }

    _read_wal_logs(_wal_callback, memtable);
    return 0;
}

