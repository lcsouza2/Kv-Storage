#ifndef UTILS_H
#define UTILS_H
#include <time.h>

typedef struct {
    struct timespec start;
} PerfTimer;

char *read_dynamic_input();
void start_perf_timer(PerfTimer *timer);
#endif