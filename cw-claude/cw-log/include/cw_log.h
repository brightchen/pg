#ifndef CW_LOG_H
#define CW_LOG_H

#include <stdio.h>
#include <stdint.h>

/* Log Level Constants */
#define CW_LOG_LEVEL_TRACE 0
#define CW_LOG_LEVEL_DEBUG 1
#define CW_LOG_LEVEL_INFO  2
#define CW_LOG_LEVEL_WARN  3
#define CW_LOG_LEVEL_ERROR 4

/* Log Format Constants */
#define CW_LOG_FORMAT_TEXT 0
#define CW_LOG_FORMAT_CSV  1
#define CW_LOG_FORMAT_JSON 2

/* Default Configuration Constants */
#define CW_LOG_DEFAULT_MAX_FILE_SIZE (10 * 1024 * 1024) /* 10MB */
#define CW_LOG_DEFAULT_MAX_FILES 5
#define CW_LOG_MAX_PATH_LEN 256
#define CW_LOG_MAX_MESSAGE_LEN 2048

/* Error Codes */
#define CW_LOG_SUCCESS 0
#define CW_LOG_ERROR_INVALID_PARAM -1
#define CW_LOG_ERROR_FILE_OPEN -2
#define CW_LOG_ERROR_FILE_WRITE -3
#define CW_LOG_ERROR_INIT -4
#define CW_LOG_ERROR_NOT_INIT -5

/* Log Configuration Structure */
typedef struct {
    char log_file_path[CW_LOG_MAX_PATH_LEN];
    int log_level;
    int log_format;
    uint64_t max_file_size;
    int max_backup_files;
    int console_output;  /* 1 to enable console output, 0 to disable */
} cw_log_config_t;

/**
 * Initialize the logging system with configuration
 * @param config Pointer to configuration structure
 * @return CW_LOG_SUCCESS on success, error code otherwise
 */
int cw_log_init(const cw_log_config_t *config);

/**
 * Initialize the logging system from configuration file
 * @param config_file Path to configuration file
 * @return CW_LOG_SUCCESS on success, error code otherwise
 */
int cw_log_init_from_file(const char *config_file);

/**
 * Log a message with specified level
 * @param level Log level
 * @param file Source file name
 * @param line Line number in source file
 * @param func Function name
 * @param fmt Format string (printf style)
 * @param ... Variable arguments
 */
void cw_log_write(int level, const char *file, int line, const char *func, const char *fmt, ...);

/**
 * Set the current log level
 * @param level New log level
 * @return CW_LOG_SUCCESS on success, error code otherwise
 */
int cw_log_set_level(int level);

/**
 * Get the current log level
 * @return Current log level
 */
int cw_log_get_level(void);

/**
 * Flush log buffer to file
 */
void cw_log_flush(void);

/**
 * Cleanup and close the logging system
 */
void cw_log_cleanup(void);

/**
 * Get string representation of log level
 * @param level Log level
 * @return String representation
 */
const char* cw_log_level_string(int level);

/**
 * Get string representation of error code
 * @param error_code Error code
 * @return String representation
 */
const char* cw_log_error_string(int error_code);

/* Convenience Macros */
#define CW_LOG_TRACE(fmt, ...) \
    cw_log_write(CW_LOG_LEVEL_TRACE, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define CW_LOG_DEBUG(fmt, ...) \
    cw_log_write(CW_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define CW_LOG_INFO(fmt, ...) \
    cw_log_write(CW_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define CW_LOG_WARN(fmt, ...) \
    cw_log_write(CW_LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define CW_LOG_ERROR(fmt, ...) \
    cw_log_write(CW_LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#endif /* CW_LOG_H */
