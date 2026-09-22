#ifndef NETDIAG_UTILS_H
#define NETDIAG_UTILS_H

#include <stddef.h>
#include <stdbool.h>

/* ANSI Terminal Color Codes */
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"

/**
 * Trims leading and trailing whitespace characters in place.
 * Returns pointer to the trimmed null-terminated string.
 */
char *utils_trim(char *str);

/**
 * Generates a random floating-point value in [min, max].
 */
double utils_random_double(double min, double max);

/**
 * Generates a random integer value in [min, max].
 */
int utils_random_int(int min, int max);

/**
 * Formats the current UTC timestamp into a human-readable buffer.
 * Buffer must have at least size bytes.
 */
void utils_get_timestamp(char *buffer, size_t size);

/**
 * Safely parses a double from a string, returning default_val if invalid.
 */
double utils_parse_double(const char *str, double default_val);

/**
 * Safely parses an int from a string, returning default_val if invalid.
 */
int utils_parse_int(const char *str, int default_val);

/**
 * Splits a CSV line into tokens, respecting quotes if present.
 * Returns the number of tokens parsed (up to max_tokens).
 */
int utils_split_csv_line(char *line, char **tokens, int max_tokens);

/**
 * Creates a directory if it does not already exist.
 * Returns 0 on success, negative error code on failure.
 */
int utils_create_dir_if_missing(const char *dir_path);

/**
 * Ensures the parent directory of a given file path exists.
 * Returns 0 on success, negative error code on failure.
 */
int utils_ensure_parent_dir(const char *file_path);

#endif /* NETDIAG_UTILS_H */
