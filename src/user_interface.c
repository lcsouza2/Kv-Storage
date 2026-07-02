#include <stdio.h>
#include "memtable.h"
#include "logging.h"
#include "settings.h"
#include "wal.h"
#include "user_interface.h"
#include "sstables.h"
#include "utils.h"
#include "k_way_iterator.h"
#include "compactor.h"


static void _check_and_flush_memtable(Database *db) {
    if (db->memtable->bytes_allocated > MAX_MEMTABLE_SIZE) {
        if (flush_memtable_to_disk(db->memtable, 0) == 0) {
            clear_memtable(db->memtable);
            clear_wal_logs();
        }
    }
}

// INTERFACE FUNCTIONS =============================


/**
 * @brief Inserts a key-value pair into the database.
 * @param db (Database *): The database instance.
 * @param key (char *): The key to be inserted.
 * @param value (char *): The value to be inserted.
 * @note If the key already exists, its value will be updated.
 */
void db_insert(Database *db, char *key, char *value) {
    if (!key || !value) {
        debug("Invalid key or value for insert operation.");
        return;
    }

    timer_type start, end;

    GET_TIME(start);
    append_wal_log(key, value);

    Memtable *tree = insert_memtable(db->memtable, key, value);
    db->memtable = tree;

    _check_and_flush_memtable(db);
    GET_TIME(end);
    info("Client INSERT: %s -> %s [Completed in: %f seconds]", key, value, DIFF_TIME(start, end));
}


/**
 * @brief Selects a value by key from the database.
 * @param db (Database *): The database instance.
 * @param key (char *): The key to search for.
 * @return (char *): a pointer to the value if found, NULL otherwise.
 */
char *db_select(Database *db, char *key) {
    timer_type start, end;

    GET_TIME(start);
    SearchResult result = search_memtable(db->memtable, key);
    char *value = result.value;
    if (!result.found) {
        debug("Key not found in memtable, searching in SSTables...");
        result = search_in_sstables(key);
        value = result.value;
    }
    GET_TIME(end);

    if (value || result.found) {
        info("Client SELECT key: %s [Completed in: %f seconds]", key, DIFF_TIME(start, end));
    } else {
        info("Client SELECT: Key not found: %s [Completed in: %f seconds]", key, DIFF_TIME(start, end));
    }
    return value;
}

/**
 * @brief Updates a value by key in the database.
 * @param db (Database *): The database instance.
 * @param key (char *): The key to be updated.
 * @param value (char *): The new value to be set.
 * @note If the key does not exist, it will be inserted.
 */
void db_update(Database *db, char *key, char *value) {
    timer_type start, end;

    GET_TIME(start);
    append_wal_log(key, value);

    Memtable *tree = insert_memtable(db->memtable, key, value);
    db->memtable = tree;

   _check_and_flush_memtable(db);
    GET_TIME(end);
    info("Client UPDATE: %s -> %s [Completed in: %f seconds]", key, value, DIFF_TIME(start, end));
}

/**
 * @brief Deletes a key-value pair from the database.
 * @param db (Database *): The database instance.
 * @param key (char *): The key to be deleted.
 */
void db_delete(Database *db, char *key) {
    timer_type start, end;

    GET_TIME(start);
    append_wal_log(key, NULL);

    Memtable *tree = insert_memtable(db->memtable, key, NULL);
    db->memtable = tree;

    _check_and_flush_memtable(db);
    GET_TIME(end);
    info("Client DELETE: %s [Completed in: %f seconds]", key, DIFF_TIME(start, end));
}

/**
 * @brief Synchronizes the database with the Write-Ahead Log (WAL) and SSTables during boot.
 * @param db (Database *): The database instance.
 * @return (int) 0 on success, -1 on failure.
 */
int boot_sync(Database *db) {
    if (create_data_storage_directory()) {
        error("Failed to create data storage directory.");
        return -1;
    }

    if (!db) {
        error("Invalid database instance for boot sync.");
        return -1;
    }

    db->memtable = create_memtable();
    if (!db->memtable) {
        error("Failed to initialize memtable.");
        free(db);
        return EXIT_FAILURE;
    }

    if (sync_wal_to_memtable(db->memtable)) {
        error("Failed to sync WAL to memtable during boot.");
        return -1;
    }

    if (sync_sstables_from_manifest()) {
        error("Failed to sync SSTables from manifest during boot.");
        return -1;
    }

    init_background_compactor();
    return 0;
}

void display_all_keys(Database *db) {
    if (!db || !db->memtable) {
        error("Invalid database or memtable for displaying keys.");
        return;
    }

    display_all_keys_in_memtable(db->memtable);
    display_all_keys_unified();
    return;
}

void clear() {
    printf("\033[H\033[J");
}

void display_separator() {
    printf("========================================\n");
}

void display_menu() {
    printf("=== Main Menu ===\n");
    printf("1. Add Key-Value Pair\n");
    printf("2. Get Value by Key\n");
    printf("3. Update Value by Key\n");
    printf("4. Delete Key-Value Pair\n");
    printf("5. Display All Keys\n");
    printf("6. Clear Screen\n");
    printf("7. Exit\n");
    printf("Choose an option: ");
}


