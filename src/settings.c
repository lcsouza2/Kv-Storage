#include <stdlib.h>

char *get_sstable_path() {
    char *path = getenv("SSTABLE_PATH");
    if (path == NULL) {
        return "./data/sstables";
    }
    return path;
}