#include "../include/cw_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define TEST_LOG_FILE "/tmp/test_cw_log.log"
#define TEST_CONFIG_FILE "/tmp/test_cw_log.conf"

static int test_count = 0;
static int test_passed = 0;
static int test_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        test_count++; \
        if (condition) { \
            test_passed++; \
            printf("  [PASS] %s\n", message); \
        } else { \
            test_failed++; \
            printf("  [FAIL] %s\n", message); \
        } \
    } while(0)

static void cleanup_test_files(void) {
    unlink(TEST_LOG_FILE);
    unlink(TEST_CONFIG_FILE);
    for (int i = 1; i <= 5; i++) {
        char backup[256];
        snprintf(backup, sizeof(backup), "%s.%d", TEST_LOG_FILE, i);
        unlink(backup);
    }
}

static int file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

static long file_size(const char *path) __attribute__((unused));
static long file_size(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

static int file_contains(const char *path, const char *text) {
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        return 0;
    }

    char buffer[4096];
    int found = 0;
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (strstr(buffer, text) != NULL) {
            found = 1;
            break;
        }
    }

    fclose(fp);
    return found;
}

void test_log_init(void) {
    printf("\n=== Test: Log Initialization ===\n");
    cleanup_test_files();

    cw_log_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.log_file_path, TEST_LOG_FILE);
    config.log_level = CW_LOG_LEVEL_DEBUG;
    config.log_format = CW_LOG_FORMAT_TEXT;
    config.max_file_size = 1024;
    config.max_backup_files = 3;
    config.console_output = 0;

    int ret = cw_log_init(&config);
    TEST_ASSERT(ret == CW_LOG_SUCCESS, "Initialize log system");
    TEST_ASSERT(file_exists(TEST_LOG_FILE), "Log file created");

    cw_log_cleanup();
    cleanup_test_files();
}

void test_log_levels(void) {
    printf("\n=== Test: Log Levels ===\n");
    cleanup_test_files();

    cw_log_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.log_file_path, TEST_LOG_FILE);
    config.log_level = CW_LOG_LEVEL_INFO;
    config.log_format = CW_LOG_FORMAT_TEXT;
    config.console_output = 0;

    cw_log_init(&config);

    CW_LOG_TRACE("This is trace");
    CW_LOG_DEBUG("This is debug");
    CW_LOG_INFO("This is info");
    CW_LOG_WARN("This is warning");
    CW_LOG_ERROR("This is error");

    cw_log_flush();

    TEST_ASSERT(!file_contains(TEST_LOG_FILE, "This is trace"), "TRACE not logged when level is INFO");
    TEST_ASSERT(!file_contains(TEST_LOG_FILE, "This is debug"), "DEBUG not logged when level is INFO");
    TEST_ASSERT(file_contains(TEST_LOG_FILE, "This is info"), "INFO logged when level is INFO");
    TEST_ASSERT(file_contains(TEST_LOG_FILE, "This is warning"), "WARN logged when level is INFO");
    TEST_ASSERT(file_contains(TEST_LOG_FILE, "This is error"), "ERROR logged when level is INFO");

    cw_log_cleanup();
    cleanup_test_files();
}

void test_log_format_text(void) {
    printf("\n=== Test: Text Format ===\n");
    cleanup_test_files();

    cw_log_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.log_file_path, TEST_LOG_FILE);
    config.log_level = CW_LOG_LEVEL_INFO;
    config.log_format = CW_LOG_FORMAT_TEXT;
    config.console_output = 0;

    cw_log_init(&config);
    CW_LOG_INFO("Test message");
    cw_log_flush();

    TEST_ASSERT(file_contains(TEST_LOG_FILE, "[INFO]"), "Text format contains [INFO]");
    TEST_ASSERT(file_contains(TEST_LOG_FILE, "Test message"), "Text format contains message");
    TEST_ASSERT(file_contains(TEST_LOG_FILE, "test_cw_log.c"), "Text format contains file name");

    cw_log_cleanup();
    cleanup_test_files();
}

void test_log_format_csv(void) {
    printf("\n=== Test: CSV Format ===\n");
    cleanup_test_files();

    cw_log_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.log_file_path, TEST_LOG_FILE);
    config.log_level = CW_LOG_LEVEL_INFO;
    config.log_format = CW_LOG_FORMAT_CSV;
    config.console_output = 0;

    cw_log_init(&config);
    CW_LOG_INFO("CSV test message");
    cw_log_flush();

    TEST_ASSERT(file_contains(TEST_LOG_FILE, "\"INFO\""), "CSV format contains quoted INFO");
    TEST_ASSERT(file_contains(TEST_LOG_FILE, "\"CSV test message\""), "CSV format contains quoted message");

    cw_log_cleanup();
    cleanup_test_files();
}

void test_log_format_json(void) {
    printf("\n=== Test: JSON Format ===\n");
    cleanup_test_files();

    cw_log_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.log_file_path, TEST_LOG_FILE);
    config.log_level = CW_LOG_LEVEL_INFO;
    config.log_format = CW_LOG_FORMAT_JSON;
    config.console_output = 0;

    cw_log_init(&config);
    CW_LOG_INFO("JSON test message");
    cw_log_flush();

    TEST_ASSERT(file_contains(TEST_LOG_FILE, "\"level\":\"INFO\""), "JSON format contains level field");
    TEST_ASSERT(file_contains(TEST_LOG_FILE, "\"message\":\"JSON test message\""), "JSON format contains message field");
    TEST_ASSERT(file_contains(TEST_LOG_FILE, "\"timestamp\""), "JSON format contains timestamp field");

    cw_log_cleanup();
    cleanup_test_files();
}

void test_log_rotation(void) {
    printf("\n=== Test: Log Rotation ===\n");
    cleanup_test_files();

    cw_log_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.log_file_path, TEST_LOG_FILE);
    config.log_level = CW_LOG_LEVEL_INFO;
    config.log_format = CW_LOG_FORMAT_TEXT;
    config.max_file_size = 200; /* Small size to trigger rotation */
    config.max_backup_files = 3;
    config.console_output = 0;

    cw_log_init(&config);

    /* Write enough logs to trigger rotation */
    for (int i = 0; i < 20; i++) {
        CW_LOG_INFO("Log rotation test message number %d", i);
    }
    cw_log_flush();

    TEST_ASSERT(file_exists(TEST_LOG_FILE), "Main log file exists after rotation");

    /* Check if backup files were created */
    char backup1[256];
    snprintf(backup1, sizeof(backup1), "%s.1", TEST_LOG_FILE);
    int backup_exists = file_exists(backup1);
    TEST_ASSERT(backup_exists, "Backup log file created after rotation");

    cw_log_cleanup();
    cleanup_test_files();
}

void test_log_set_get_level(void) {
    printf("\n=== Test: Set/Get Log Level ===\n");
    cleanup_test_files();

    cw_log_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.log_file_path, TEST_LOG_FILE);
    config.log_level = CW_LOG_LEVEL_INFO;
    config.log_format = CW_LOG_FORMAT_TEXT;
    config.console_output = 0;

    cw_log_init(&config);

    TEST_ASSERT(cw_log_get_level() == CW_LOG_LEVEL_INFO, "Initial log level is INFO");

    cw_log_set_level(CW_LOG_LEVEL_ERROR);
    TEST_ASSERT(cw_log_get_level() == CW_LOG_LEVEL_ERROR, "Log level changed to ERROR");

    cw_log_cleanup();
    cleanup_test_files();
}

void test_log_init_from_file(void) {
    printf("\n=== Test: Init from Config File ===\n");
    cleanup_test_files();

    /* Create config file */
    FILE *fp = fopen(TEST_CONFIG_FILE, "w");
    fprintf(fp, "log_file_path=%s\n", TEST_LOG_FILE);
    fprintf(fp, "log_level=DEBUG\n");
    fprintf(fp, "log_format=JSON\n");
    fprintf(fp, "max_file_size=2048\n");
    fprintf(fp, "max_backup_files=5\n");
    fprintf(fp, "console_output=0\n");
    fclose(fp);

    int ret = cw_log_init_from_file(TEST_CONFIG_FILE);
    TEST_ASSERT(ret == CW_LOG_SUCCESS, "Initialize from config file");
    TEST_ASSERT(file_exists(TEST_LOG_FILE), "Log file created from config");

    CW_LOG_DEBUG("Test from config file");
    cw_log_flush();

    TEST_ASSERT(file_contains(TEST_LOG_FILE, "\"level\":\"DEBUG\""), "Config file settings applied");

    cw_log_cleanup();
    cleanup_test_files();
}

void test_log_level_string(void) {
    printf("\n=== Test: Log Level String ===\n");

    TEST_ASSERT(strcmp(cw_log_level_string(CW_LOG_LEVEL_TRACE), "TRACE") == 0, "TRACE level string");
    TEST_ASSERT(strcmp(cw_log_level_string(CW_LOG_LEVEL_DEBUG), "DEBUG") == 0, "DEBUG level string");
    TEST_ASSERT(strcmp(cw_log_level_string(CW_LOG_LEVEL_INFO), "INFO") == 0, "INFO level string");
    TEST_ASSERT(strcmp(cw_log_level_string(CW_LOG_LEVEL_WARN), "WARN") == 0, "WARN level string");
    TEST_ASSERT(strcmp(cw_log_level_string(CW_LOG_LEVEL_ERROR), "ERROR") == 0, "ERROR level string");
}

void test_log_error_string(void) {
    printf("\n=== Test: Log Error String ===\n");

    TEST_ASSERT(strcmp(cw_log_error_string(CW_LOG_SUCCESS), "Success") == 0, "SUCCESS error string");
    TEST_ASSERT(strcmp(cw_log_error_string(CW_LOG_ERROR_INVALID_PARAM), "Invalid parameter") == 0, "INVALID_PARAM error string");
    TEST_ASSERT(strcmp(cw_log_error_string(CW_LOG_ERROR_FILE_OPEN), "Failed to open file") == 0, "FILE_OPEN error string");
    TEST_ASSERT(strcmp(cw_log_error_string(CW_LOG_ERROR_FILE_WRITE), "Failed to write to file") == 0, "FILE_WRITE error string");
    TEST_ASSERT(strcmp(cw_log_error_string(CW_LOG_ERROR_INIT), "Initialization error") == 0, "INIT error string");
    TEST_ASSERT(strcmp(cw_log_error_string(CW_LOG_ERROR_NOT_INIT), "Not initialized") == 0, "NOT_INIT error string");
    TEST_ASSERT(strcmp(cw_log_error_string(-999), "Unknown error") == 0, "Unknown error string");
}

int main(void) {
    printf("======================================\n");
    printf("  CW-LOG Unit Tests\n");
    printf("======================================\n");

    test_log_init();
    test_log_levels();
    test_log_format_text();
    test_log_format_csv();
    test_log_format_json();
    test_log_rotation();
    test_log_set_get_level();
    test_log_init_from_file();
    test_log_level_string();
    test_log_error_string();

    printf("\n======================================\n");
    printf("Test Summary:\n");
    printf("  Total:  %d\n", test_count);
    printf("  Passed: %d\n", test_passed);
    printf("  Failed: %d\n", test_failed);
    printf("======================================\n");

    cleanup_test_files();

    return (test_failed == 0) ? 0 : 1;
}
