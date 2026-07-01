#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utils.h"

char *read_dynamic_input() {
    size_t capacity = 64;
    size_t length = 0;

    char *buffer = malloc(capacity);
    if (!buffer) return NULL;

    int c;
    while ((c = fgetc(stdin)) != '\n' && c != EOF) {
        buffer[length++] = (char)c;

        if (length == capacity) {
            capacity *= 2;

            char *new_buffer = realloc(buffer, capacity);
            if (!new_buffer) {
                free(buffer);
                return NULL;
            }
            buffer = new_buffer;
        }
    }

    buffer[length] = '\0';

    if (length == 0 && c == EOF) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

