#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H
#endif

typedef struct key_value {
    char *key;
    char *value;
} KeyValue;

typedef struct {
    Memtable *memtable;
} DB;