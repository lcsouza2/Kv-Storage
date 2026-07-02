#include <stdio.h>
#include <stdarg.h>
#include "settings.h"

void _print_log(const char *level, const char *format, va_list args) {
    char buffer[MAX_LOG_LEN];
    vsnprintf(buffer, sizeof(buffer), format, args);
    printf("[%s]: ", level);
    vprintf(buffer, args);
    printf("\n");
}

void success(const char *format, ...) {
    va_list args;
    va_start(args, format);

    printf("\033[94m");
    _print_log("SUCCESS", format, args);
    printf("\033[0m");

    va_end(args);
}

void info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    _print_log("\033[32mINFO\033[0m", format, args);
    va_end(args);
}

void warning(const char *format, ...) {
    va_list args;
    va_start(args, format);
    _print_log("\033[33mWARNING\033[0m", format, args);
    va_end(args);
}

void error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    _print_log("\033[31mERROR\033[0m", format, args);
    va_end(args);
}

void debug(const char *format, ...) {
    if (!DEBUG_LOGS) return; // Skip debug logs if disabled
    va_list args;
    va_start(args, format);
    _print_log("\033[36mDEBUG\033[0m", format, args);
    va_end(args);
}