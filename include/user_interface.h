#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H
#include "memtable.h"
typedef struct key_value {
    char *key;
    char *value;
} KeyValue;

typedef struct database {
    Memtable *memtable;
} Database;

void db_insert(Database *db, char *key, char *value);
char *db_select(Database *db, char *key);
void db_update(Database *db, char *key, char *value);
void db_delete(Database *db, char *key);
void display_all_keys(Database *db);
int boot_sync(Database *db);
void clear();
void display_menu();
void display_separator();
#endif
