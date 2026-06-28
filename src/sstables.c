#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "settings.h"
#include "logging.h"
#include "sstables.h"

int _current_level_files[MAX_SSTABLE_LEVELS] = {0};

sstable_t *create_sstable(int min_key, int max_key, int level) {
    sstable_t *sstable = malloc(sizeof(sstable_t));
    if (!sstable) {
        debug("Failed to allocate memory for SSTable.");
        return NULL;
    }

    char filepath[SSTABLE_MAX_PAHT_LENGTH];

    snprintf(
        filepath,
        sizeof(filepath),
        "%s/L%d_%d.dat",
        SSTABLE_DEFAULT_PATH,
        level,
        _current_level_files[level]
    );

    _current_level_files[level]++;

    sstable->path = strdup(filepath);
    if (!sstable->path) {
        free(sstable);
        debug("Failed to generate filepath for SSTable.");
        return NULL;
    }
    sstable->min_key = min_key;
    sstable->max_key = max_key;
    sstable->level = level;

    FILE *file_try = fopen(sstable->path, "rb");
    if (file_try) {
        fclose(file_try);
        error("Error creating SSTable: file already exists: %s", sstable->path);
        free(sstable->path);
        free(sstable);
        return NULL;
    }

    fopen(sstable->path, "wb");
    info("sstable created: %s", sstable->path);
    return sstable;
}
