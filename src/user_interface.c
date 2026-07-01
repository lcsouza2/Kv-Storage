#include <stdio.h>
#include "memtable.h"
#include "logging.h"
#include "settings.h"
#include "wal.h"
#include "user_interface.h"
#include "sstables.h"
#include "utils.h"

static void check_and_flush_memtable(Database *db) {
    if (db->memtable->bytes_allocated > MAX_MEMTABLE_SIZE) {
        info("Memtable threshold exceeded. Flushing...");
        if (flush_memtable_to_disk(db->memtable, 0) == 0) {
            clear_memtable(db->memtable);
            clear_wal_logs();
        }
    }
}

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

    check_and_flush_memtable(db);
    GET_TIME(end);
    info("Client INSERT: %s -> %s [Completed in: %f seconds]", key, value, DIFF_TIME(start, end));

}

char *db_select(Database *db, char *key) {
    timer_type start, end;

    GET_TIME(start);
    char *value = search_memtable(db->memtable, key);
    if (!value) {
        debug("Key not found in memtable, searching in SSTables...");
        value = search_in_sstables(key);
    }
    GET_TIME(end);

    if (value) {
        info("Client SELECT key: %s [Completed in: %f seconds]", key, DIFF_TIME(start, end));
    } else {
        debug("Key not found in memtable, searching in SSTables...");
        info("Client SELECT: Key not found: %s [Completed in: %f seconds]", key, DIFF_TIME(start, end));
    }
    return value;
}

void db_update(Database *db, char *key, char *value) {
    timer_type start, end;

    GET_TIME(start);
    append_wal_log(key, value);

    Memtable *tree = insert_memtable(db->memtable, key, value);
    db->memtable = tree;

   check_and_flush_memtable(db);
    GET_TIME(end);
    info("Client UPDATE: %s -> %s [Completed in: %f seconds]", key, value, DIFF_TIME(start, end));
}

void db_delete(Database *db, char *key) {
    timer_type start, end;

    GET_TIME(start);
    append_wal_log(key, NULL);

    Memtable *tree = insert_memtable(db->memtable, key, NULL);
    db->memtable = tree;

    check_and_flush_memtable(db);
    GET_TIME(end);
    info("Client DELETE: %s [Completed in: %f seconds]", key, DIFF_TIME(start, end));
}

void boot_sync(Database *db) {
    sync_wal_to_memtable(db->memtable);
    // index_sstables(); // This function is not implemented yet, but it will be used to index the existing SSTables for faster search.
}

void clear() {
    printf("\033[H\033[J");
}

void print_main_menu() {
    printf("=== Main Menu ===\n");
    printf("1. Add Key-Value Pair\n");
    printf("2. Get Value by Key\n");
    printf("3. Update Value by Key\n");
    printf("4. Delete Key-Value Pair\n");
    printf("5. Display All Key-Value Pairs\n");
    printf("6. Exit\n");
    printf("Choose an option: ");
}


