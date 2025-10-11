#include "../include/cw_msg.h"
#include "../../cw-log/include/cw_log.h"
#include <stdio.h>

int main() {
    // Initialize logging
    cw_log_config_t log_config = {
        .log_file_path = "/tmp/test_msg_logging.log",
        .log_level = CW_LOG_LEVEL_ERROR,
        .log_format = CW_LOG_FORMAT_TEXT,
        .max_file_size = 1024 * 1024,
        .max_backup_files = 1,
        .console_output = 1
    };
    cw_log_init(&log_config);

    printf("=== Testing cw-msg-lib Error Logging ===\n\n");

    // Test 1: NULL parameter
    printf("Test 1: NULL parameters should log error\n");
    int ret = cw_generate_header(NULL, NULL, 100);
    printf("  Result: %s (%d)\n\n", cw_msg_error_string(ret), ret);

    // Test 2: Buffer too small
    printf("Test 2: Buffer too small should log error\n");
    cw_header_t header = {.preamble = 0, .hlen = 2};
    uint8_t small_buffer[4];
    ret = cw_generate_header(&header, small_buffer, 4);
    printf("  Result: %s (%d)\n\n", cw_msg_error_string(ret), ret);

    // Test 3: Parse with insufficient data
    printf("Test 3: Insufficient data should log error\n");
    uint8_t short_data[4] = {0, 0, 0, 0};
    cw_header_t parsed_header;
    ret = cw_parse_header(short_data, 4, &parsed_header);
    printf("  Result: %s (%d)\n\n", cw_msg_error_string(ret), ret);

    printf("All error logging tests completed.\n");
    printf("Check /tmp/test_msg_logging.log for error messages.\n");
    
    cw_log_cleanup();
    return 0;
}
