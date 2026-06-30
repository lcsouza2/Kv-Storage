typedef struct sstable {
    char *path;
    char *min_key;
    char *max_key;
    int level;
} SSTable;

typedef struct level_index {
    int count;             // Quantidade atual de SSTables neste nível
    int capacity;          // Capacidade alocada na memória (para o realloc)
    SSTable **tables;      // Array dinâmico de ponteiros para as SSTables
} LevelIndex;

SSTable *dump_memtable_to_disk(Memtable *memtable, int level);