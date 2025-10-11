#include "cw_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>

/* Internal state structure */
typedef struct {
    FILE *fp;
    cw_log_config_t config;
    uint64_t current_file_size;
    pthread_mutex_t mutex;
    int initialized;
} cw_log_state_t;

static cw_log_state_t g_log_state = {
    .fp = NULL,
    .initialized = 0,
    .current_file_size = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

/* Forward declarations */
static int cw_log_rotate(void);
static void cw_log_format_text(char *buffer, size_t size, int level, const char *file,
                                int line, const char *func, const char *msg);
static void cw_log_format_csv(char *buffer, size_t size, int level, const char *file,
                               int line, const char *func, const char *msg);
static void cw_log_format_json(char *buffer, size_t size, int level, const char *file,
                                int line, const char *func, const char *msg);
static const char* cw_log_basename(const char *path);

const char* cw_log_level_string(int level) {
    switch (level) {
        case CW_LOG_LEVEL_TRACE: return "TRACE";
        case CW_LOG_LEVEL_DEBUG: return "DEBUG";
        case CW_LOG_LEVEL_INFO:  return "INFO";
        case CW_LOG_LEVEL_WARN:  return "WARN";
        case CW_LOG_LEVEL_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

const char* cw_log_error_string(int error_code) {
    switch (error_code) {
        case CW_LOG_SUCCESS: return "Success";
        case CW_LOG_ERROR_INVALID_PARAM: return "Invalid parameter";
        case CW_LOG_ERROR_FILE_OPEN: return "Failed to open file";
        case CW_LOG_ERROR_FILE_WRITE: return "Failed to write to file";
        case CW_LOG_ERROR_INIT: return "Initialization error";
        case CW_LOG_ERROR_NOT_INIT: return "Not initialized";
        default: return "Unknown error";
    }
}

static const char* cw_log_basename(const char *path) {
    const char *base = strrchr(path, '/');
    return base ? base + 1 : path;
}

int cw_log_init(const cw_log_config_t *config) {
    if (config == NULL) {
        return CW_LOG_ERROR_INVALID_PARAM;
    }

    pthread_mutex_lock(&g_log_state.mutex);

    if (g_log_state.initialized) {
        cw_log_cleanup();
    }

    /* Copy configuration */
    memcpy(&g_log_state.config, config, sizeof(cw_log_config_t));

    /* Set defaults if not specified */
    if (g_log_state.config.max_file_size == 0) {
        g_log_state.config.max_file_size = CW_LOG_DEFAULT_MAX_FILE_SIZE;
    }
    if (g_log_state.config.max_backup_files == 0) {
        g_log_state.config.max_backup_files = CW_LOG_DEFAULT_MAX_FILES;
    }

    /* Open log file */
    g_log_state.fp = fopen(g_log_state.config.log_file_path, "a");
    if (g_log_state.fp == NULL) {
        pthread_mutex_unlock(&g_log_state.mutex);
        return CW_LOG_ERROR_FILE_OPEN;
    }

    /* Get current file size */
    struct stat st;
    if (stat(g_log_state.config.log_file_path, &st) == 0) {
        g_log_state.current_file_size = st.st_size;
    } else {
        g_log_state.current_file_size = 0;
    }

    g_log_state.initialized = 1;
    pthread_mutex_unlock(&g_log_state.mutex);

    return CW_LOG_SUCCESS;
}

int cw_log_init_from_file(const char *config_file) {
    if (config_file == NULL) {
        return CW_LOG_ERROR_INVALID_PARAM;
    }

    FILE *fp = fopen(config_file, "r");
    if (fp == NULL) {
        return CW_LOG_ERROR_FILE_OPEN;
    }

    cw_log_config_t config;
    memset(&config, 0, sizeof(config));

    /* Set defaults */
    config.log_level = CW_LOG_LEVEL_INFO;
    config.log_format = CW_LOG_FORMAT_TEXT;
    config.max_file_size = CW_LOG_DEFAULT_MAX_FILE_SIZE;
    config.max_backup_files = CW_LOG_DEFAULT_MAX_FILES;
    config.console_output = 0;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }

        char key[64], value[256];
        if (sscanf(line, "%63[^=]=%255s", key, value) == 2) {
            /* Trim whitespace */
            char *k = key;
            while (*k == ' ' || *k == '\t') k++;
            char *ke = k + strlen(k) - 1;
            while (ke > k && (*ke == ' ' || *ke == '\t' || *ke == '\n')) *ke-- = '\0';

            if (strcmp(k, "log_file_path") == 0) {
                strncpy(config.log_file_path, value, CW_LOG_MAX_PATH_LEN - 1);
                config.log_file_path[CW_LOG_MAX_PATH_LEN - 1] = '\0';
            } else if (strcmp(k, "log_level") == 0) {
                if (strcmp(value, "TRACE") == 0) config.log_level = CW_LOG_LEVEL_TRACE;
                else if (strcmp(value, "DEBUG") == 0) config.log_level = CW_LOG_LEVEL_DEBUG;
                else if (strcmp(value, "INFO") == 0) config.log_level = CW_LOG_LEVEL_INFO;
                else if (strcmp(value, "WARN") == 0) config.log_level = CW_LOG_LEVEL_WARN;
                else if (strcmp(value, "ERROR") == 0) config.log_level = CW_LOG_LEVEL_ERROR;
            } else if (strcmp(k, "log_format") == 0) {
                if (strcmp(value, "TEXT") == 0) config.log_format = CW_LOG_FORMAT_TEXT;
                else if (strcmp(value, "CSV") == 0) config.log_format = CW_LOG_FORMAT_CSV;
                else if (strcmp(value, "JSON") == 0) config.log_format = CW_LOG_FORMAT_JSON;
            } else if (strcmp(k, "max_file_size") == 0) {
                config.max_file_size = strtoull(value, NULL, 10);
            } else if (strcmp(k, "max_backup_files") == 0) {
                config.max_backup_files = atoi(value);
            } else if (strcmp(k, "console_output") == 0) {
                config.console_output = atoi(value);
            }
        }
    }

    fclose(fp);
    return cw_log_init(&config);
}

static int cw_log_rotate(void) {
    if (g_log_state.fp != NULL) {
        fclose(g_log_state.fp);
        g_log_state.fp = NULL;
    }

    /* Rotate existing backup files */
    for (int i = g_log_state.config.max_backup_files - 1; i > 0; i--) {
        char old_name[CW_LOG_MAX_PATH_LEN + 16];
        char new_name[CW_LOG_MAX_PATH_LEN + 16];

        if (i == 1) {
            snprintf(old_name, sizeof(old_name), "%s", g_log_state.config.log_file_path);
        } else {
            snprintf(old_name, sizeof(old_name), "%s.%d", g_log_state.config.log_file_path, i - 1);
        }
        snprintf(new_name, sizeof(new_name), "%s.%d", g_log_state.config.log_file_path, i);

        rename(old_name, new_name);
    }

    /* Open new log file */
    g_log_state.fp = fopen(g_log_state.config.log_file_path, "w");
    if (g_log_state.fp == NULL) {
        return CW_LOG_ERROR_FILE_OPEN;
    }

    g_log_state.current_file_size = 0;
    return CW_LOG_SUCCESS;
}

static void cw_log_format_text(char *buffer, size_t size, int level, const char *file,
                                int line, const char *func, const char *msg) {
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);

    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_info);

    snprintf(buffer, size, "[%s] [%s] [%s:%d:%s] %s\n",
             time_str, cw_log_level_string(level), cw_log_basename(file), line, func, msg);
}

static void cw_log_format_csv(char *buffer, size_t size, int level, const char *file,
                               int line, const char *func, const char *msg) {
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);

    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_info);

    /* Escape quotes in message */
    char escaped_msg[CW_LOG_MAX_MESSAGE_LEN];
    int j = 0;
    for (int i = 0; msg[i] != '\0' && j < CW_LOG_MAX_MESSAGE_LEN - 2; i++) {
        if (msg[i] == '"') {
            escaped_msg[j++] = '"';
        }
        escaped_msg[j++] = msg[i];
    }
    escaped_msg[j] = '\0';

    snprintf(buffer, size, "\"%s\",\"%s\",\"%s\",%d,\"%s\",\"%s\"\n",
             time_str, cw_log_level_string(level), cw_log_basename(file), line, func, escaped_msg);
}

static void cw_log_format_json(char *buffer, size_t size, int level, const char *file,
                                int line, const char *func, const char *msg) {
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);

    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_info);

    /* Escape quotes and backslashes in message */
    char escaped_msg[CW_LOG_MAX_MESSAGE_LEN];
    int j = 0;
    for (int i = 0; msg[i] != '\0' && j < CW_LOG_MAX_MESSAGE_LEN - 2; i++) {
        if (msg[i] == '"' || msg[i] == '\\') {
            escaped_msg[j++] = '\\';
        }
        escaped_msg[j++] = msg[i];
    }
    escaped_msg[j] = '\0';

    snprintf(buffer, size, "{\"timestamp\":\"%s\",\"level\":\"%s\",\"file\":\"%s\",\"line\":%d,\"function\":\"%s\",\"message\":\"%s\"}\n",
             time_str, cw_log_level_string(level), cw_log_basename(file), line, func, escaped_msg);
}

void cw_log_write(int level, const char *file, int line, const char *func, const char *fmt, ...) {
    if (!g_log_state.initialized) {
        return;
    }

    if (level < g_log_state.config.log_level) {
        return;
    }

    /* Format the message */
    char msg[CW_LOG_MAX_MESSAGE_LEN];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    /* Format the log entry */
    char log_entry[CW_LOG_MAX_MESSAGE_LEN * 2];
    switch (g_log_state.config.log_format) {
        case CW_LOG_FORMAT_CSV:
            cw_log_format_csv(log_entry, sizeof(log_entry), level, file, line, func, msg);
            break;
        case CW_LOG_FORMAT_JSON:
            cw_log_format_json(log_entry, sizeof(log_entry), level, file, line, func, msg);
            break;
        case CW_LOG_FORMAT_TEXT:
        default:
            cw_log_format_text(log_entry, sizeof(log_entry), level, file, line, func, msg);
            break;
    }

    pthread_mutex_lock(&g_log_state.mutex);

    /* Write to console if enabled */
    if (g_log_state.config.console_output) {
        fprintf(stderr, "%s", log_entry);
    }

    /* Write to file */
    if (g_log_state.fp != NULL) {
        size_t len = strlen(log_entry);
        if (fwrite(log_entry, 1, len, g_log_state.fp) == len) {
            g_log_state.current_file_size += len;

            /* Check if rotation is needed */
            if (g_log_state.current_file_size >= g_log_state.config.max_file_size) {
                cw_log_rotate();
            }
        }
    }

    pthread_mutex_unlock(&g_log_state.mutex);
}

int cw_log_set_level(int level) {
    if (level < CW_LOG_LEVEL_TRACE || level > CW_LOG_LEVEL_ERROR) {
        return CW_LOG_ERROR_INVALID_PARAM;
    }

    pthread_mutex_lock(&g_log_state.mutex);
    g_log_state.config.log_level = level;
    pthread_mutex_unlock(&g_log_state.mutex);

    return CW_LOG_SUCCESS;
}

int cw_log_get_level(void) {
    int level;
    pthread_mutex_lock(&g_log_state.mutex);
    level = g_log_state.config.log_level;
    pthread_mutex_unlock(&g_log_state.mutex);
    return level;
}

void cw_log_flush(void) {
    pthread_mutex_lock(&g_log_state.mutex);
    if (g_log_state.fp != NULL) {
        fflush(g_log_state.fp);
    }
    pthread_mutex_unlock(&g_log_state.mutex);
}

void cw_log_cleanup(void) {
    pthread_mutex_lock(&g_log_state.mutex);

    if (g_log_state.fp != NULL) {
        fclose(g_log_state.fp);
        g_log_state.fp = NULL;
    }

    g_log_state.initialized = 0;
    g_log_state.current_file_size = 0;

    pthread_mutex_unlock(&g_log_state.mutex);
}
