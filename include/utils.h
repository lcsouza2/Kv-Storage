#ifndef UTILS_H
#define UTILS_H
#include <time.h>

typedef struct {
    char *value;
    int found;
} SearchResult;

#ifdef _WIN32
    // No Windows, usamos o clock() que retorna em CLOCKS_PER_SEC
    typedef clock_t timer_type;
    #define GET_TIME(t) t = clock()
    #define DIFF_TIME(start, end) ((double)(end - start) / CLOCKS_PER_SEC)
#else
    // Em Linux/Unix, usamos o nanosegundo preciso
    #include <time.h>
    typedef struct timespec timer_type;
    #define GET_TIME(t) clock_gettime(CLOCK_MONOTONIC, &t)
    #define DIFF_TIME(start, end) ((end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9)
#endif

char *read_dynamic_input();
char *generate_sstable_filepath(int level, int id);
#endif