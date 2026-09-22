#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

char *utils_trim(char *str) {
    if (!str) return NULL;
    
    /* Trim leading whitespace */
    while (isspace((unsigned char)*str)) {
        str++;
    }
    
    if (*str == '\0') {
        return str;
    }
    
    /* Trim trailing whitespace */
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }
    *(end + 1) = '\0';
    
    return str;
}

double utils_random_double(double min, double max) {
    if (min >= max) return min;
    double scale = (double)rand() / (double)RAND_MAX;
    return min + scale * (max - min);
}

int utils_random_int(int min, int max) {
    if (min >= max) return min;
    return min + rand() % (max - min + 1);
}

void utils_get_timestamp(char *buffer, size_t size) {
    if (!buffer || size == 0) return;
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    if (tm_info) {
        strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        snprintf(buffer, size, "UNKNOWN_TIME");
    }
}

double utils_parse_double(const char *str, double default_val) {
    if (!str) return default_val;
    char *endptr = NULL;
    double val = strtod(str, &endptr);
    if (endptr == str) {
        return default_val;
    }
    return val;
}

int utils_parse_int(const char *str, int default_val) {
    if (!str) return default_val;
    char *endptr = NULL;
    long val = strtol(str, &endptr, 10);
    if (endptr == str) {
        return default_val;
    }
    return (int)val;
}

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

int utils_split_csv_line(char *line, char **tokens, int max_tokens) {
    if (!line || !tokens || max_tokens <= 0) return 0;
    
    int count = 0;
    char *p = line;
    bool in_quotes = false;
    char *token_start = p;
    
    while (*p && count < max_tokens) {
        if (*p == '"') {
            in_quotes = !in_quotes;
        } else if (*p == ',' && !in_quotes) {
            *p = '\0';
            tokens[count++] = utils_trim(token_start);
            token_start = p + 1;
        }
        p++;
    }
    
    if (count < max_tokens) {
        tokens[count++] = utils_trim(token_start);
    }
    
    return count;
}

int utils_create_dir_if_missing(const char *dir_path) {
    if (!dir_path || strlen(dir_path) == 0) return 0;
    
    struct stat st;
    if (stat(dir_path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) return 0;
        return -1;
    }
    
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s", dir_path);
    size_t len = strlen(tmp);
    if (len > 0 && tmp[len - 1] == '/') tmp[len - 1] = '\0';
    
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (stat(tmp, &st) != 0) {
                if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                    return -1;
                }
            }
            *p = '/';
        }
    }
    
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        return -1;
    }
    
    return 0;
}

int utils_ensure_parent_dir(const char *file_path) {
    if (!file_path) return -1;
    
    char dir_path[512];
    snprintf(dir_path, sizeof(dir_path), "%s", file_path);
    
    char *last_slash = strrchr(dir_path, '/');
    if (!last_slash) {
        /* No parent directory component (e.g. "file.log" in current dir) */
        return 0;
    }
    
    *last_slash = '\0';
    return utils_create_dir_if_missing(dir_path);
}
