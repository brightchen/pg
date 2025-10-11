#include "../include/cw_msg.h"
#include "../../cw-log/include/cw_log.h"
#include <stdio.h>

int main() {
    // Initialize logging
    cw_log_config_t log_config = {
        .log_file_path = "/tmp/test_comprehensive_logging.log",
        .log_level = CW_LOG_LEVEL_ERROR,
        .log_format = CW_LOG_FORMAT_TEXT,
        .max_file_size = 1024 * 1024,
        .max_backup_files = 1,
        .console_output = 1
    };
    cw_log_init(&log_config);

    printf("=== Comprehensive Error Logging Test ===\n\n");

    // Test 1: Parse functions with NULL parameters
    printf("Test 1: Parse functions with NULL parameters\n");

    printf("  cw_parse_result_code(NULL, NULL): ");
    int ret = cw_parse_result_code(NULL, NULL);
    printf("%s (%d)\n", cw_msg_error_string(ret), ret);

    printf("  cw_parse_session_id(NULL, NULL): ");
    ret = cw_parse_session_id(NULL, NULL);
    printf("%s (%d)\n", cw_msg_error_string(ret), ret);

    printf("  cw_parse_ac_name(NULL, NULL): ");
    ret = cw_parse_ac_name(NULL, NULL);
    printf("%s (%d)\n", cw_msg_error_string(ret), ret);

    printf("  cw_parse_wtp_name(NULL, NULL): ");
    ret = cw_parse_wtp_name(NULL, NULL);
    printf("%s (%d)\n\n", cw_msg_error_string(ret), ret);

    // Test 2: Parse functions with invalid data
    printf("Test 2: Parse functions with invalid data\n");

    // Result code element with insufficient length
    printf("  cw_parse_result_code() with short data: ");
    cw_msg_element_t elem;
    uint8_t short_data[2] = {0, 0};
    elem.value = short_data;
    elem.length = 2;
    cw_result_code_t result;
    ret = cw_parse_result_code(&elem, &result);
    printf("%s (%d)\n", cw_msg_error_string(ret), ret);

    // Session ID element with excessive length
    printf("  cw_parse_session_id() with excessive length: ");
    uint8_t long_data[256];
    elem.value = long_data;
    elem.length = 256; // Exceeds CW_MAX_SESSION_ID_LEN (128)
    cw_session_id_t session_id;
    ret = cw_parse_session_id(&elem, &session_id);
    printf("%s (%d)\n\n", cw_msg_error_string(ret), ret);

    // Test 3: Generate functions with NULL parameters
    printf("Test 3: Generate functions with NULL parameters\n");

    printf("  cw_generate_ac_descriptor(NULL, NULL): ");
    ret = cw_generate_ac_descriptor(NULL, NULL, 0);
    printf("%s (%d)\n", cw_msg_error_string(ret), ret);

    printf("  cw_generate_result_code() with NULL buffer: ");
    ret = cw_generate_result_code(0, NULL, 0);
    printf("%s (%d)\n", cw_msg_error_string(ret), ret);

    printf("  cw_generate_session_id(NULL, NULL): ");
    ret = cw_generate_session_id(NULL, NULL, 0);
    printf("%s (%d)\n", cw_msg_error_string(ret), ret);

    printf("  cw_generate_ac_name(NULL, NULL): ");
    ret = cw_generate_ac_name(NULL, NULL, 0);
    printf("%s (%d)\n\n", cw_msg_error_string(ret), ret);

    // Test 4: Generate functions with buffer too small
    printf("Test 4: Generate functions with buffer too small\n");

    printf("  cw_generate_ac_descriptor() with small buffer: ");
    cw_ac_descriptor_t desc = {
        .stations = 100,
        .limit = 500,
        .active_wtps = 10,
        .max_wtps = 50,
        .security = 1,
        .rmac_field = 0,
        .reserved = 0,
        .dtls_policy = 1
    };
    uint8_t small_buffer[4];
    ret = cw_generate_ac_descriptor(&desc, small_buffer, 4);
    printf("%s (%d)\n", cw_msg_error_string(ret), ret);

    printf("  cw_generate_result_code() with small buffer: ");
    ret = cw_generate_result_code(0, small_buffer, 4);
    printf("%s (%d)\n", cw_msg_error_string(ret), ret);

    printf("\nAll comprehensive error logging tests completed.\n");
    printf("Check /tmp/test_comprehensive_logging.log for detailed error messages.\n");

    cw_log_cleanup();
    return 0;
}
