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

#endif
