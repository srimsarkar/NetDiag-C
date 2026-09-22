#include "logger.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

static FILE *s_log_file = NULL;
static LogLevel s_min_level = LOG_LEVEL_INFO;

static const char *level_to_string(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_DEBUG:    return "DEBUG";
        case LOG_LEVEL_INFO:     return "INFO ";
        case LOG_LEVEL_WARN:     return "WARN ";
        case LOG_LEVEL_ERROR:    return "ERROR";
        case LOG_LEVEL_CRITICAL: return "CRIT ";
        default:                 return "UNKWN";
    }
}

static const char *level_to_color(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_DEBUG:    return COLOR_CYAN;
        case LOG_LEVEL_INFO:     return COLOR_GREEN;
        case LOG_LEVEL_WARN:     return COLOR_YELLOW;
        case LOG_LEVEL_ERROR:    return COLOR_RED;
        case LOG_LEVEL_CRITICAL: return COLOR_MAGENTA COLOR_BOLD;
        default:                 return COLOR_RESET;
    }
}

int logger_init(const char *log_file_path, LogLevel min_level) {
    s_min_level = min_level;
    
    if (log_file_path && strlen(log_file_path) > 0) {
        utils_ensure_parent_dir(log_file_path);
        s_log_file = fopen(log_file_path, "a");
        if (!s_log_file) {
            fprintf(stderr, "[WARN] Could not open log file '%s' for writing.\n", log_file_path);
            return -1;
        }
    }
    return 0;
}

void logger_shutdown(void) {
    if (s_log_file) {
        fflush(s_log_file);
        fclose(s_log_file);
        s_log_file = NULL;
    }
}

void logger_set_level(LogLevel level) {
    s_min_level = level;
}

void logger_log(LogLevel level, const char *file, int line, const char *format, ...) {
    if (level < s_min_level) {
        return;
    }
    
    char timestamp[32];
    utils_get_timestamp(timestamp, sizeof(timestamp));
    
    /* Format file basename */
    const char *basename = file;
    const char *p = file;
    while (*p) {
        if (*p == '/' || *p == '\\') {
            basename = p + 1;
        }
        p++;
    }
    
    va_list args;
    
    /* Console output with colors */
    FILE *stream = (level >= LOG_LEVEL_ERROR) ? stderr : stdout;
    fprintf(stream, "%s[%s]%s %s[%s]%s %s:%d - ",
            COLOR_WHITE, timestamp, COLOR_RESET,
            level_to_color(level), level_to_string(level), COLOR_RESET,
            basename, line);
    va_start(args, format);
    vfprintf(stream, format, args);
    va_end(args);
    fprintf(stream, "\n");
    fflush(stream);
    
    /* File output without ANSI escape codes */
    if (s_log_file) {
        fprintf(s_log_file, "[%s] [%s] %s:%d - ",
                timestamp, level_to_string(level), basename, line);
        va_start(args, format);
        vfprintf(s_log_file, format, args);
        va_end(args);
        fprintf(s_log_file, "\n");
        fflush(s_log_file);
    }
}
