#ifndef NETDIAG_LOGGER_H
#define NETDIAG_LOGGER_H

#include <stdio.h>
#include <stdbool.h>

/* Logging Levels */
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_CRITICAL
} LogLevel;

/**
 * Initializes the logger with an optional log file path and minimum log level.
 * Pass NULL for log_file_path to only log to stdout/stderr.
 */
int logger_init(const char *log_file_path, LogLevel min_level);

/**
 * Closes the log file if opened and cleans up logger state.
 */
void logger_shutdown(void);

/**
 * Sets the minimum logging level.
 */
void logger_set_level(LogLevel level);

/**
 * Core logging function.
 */
void logger_log(LogLevel level, const char *file, int line, const char *format, ...);

/* Convenience logging macros */
#define LOG_DEBUG(...)    logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)     logger_log(LOG_LEVEL_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)     logger_log(LOG_LEVEL_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...)    logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_CRITICAL(...) logger_log(LOG_LEVEL_CRITICAL, __FILE__, __LINE__, __VA_ARGS__)

#endif /* NETDIAG_LOGGER_H */
