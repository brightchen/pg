#include "../include/cw_msg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void test_header_generation_parsing(void) {
    printf("\n=== Test: Header Generation and Parsing ===\n");

    cw_header_t header = {
        .preamble = CW_PREAMBLE_VERSION_0,
        .hlen = 2,
        .rid = 1,
        .wbid = 0,
        .flags = CW_FLAG_HDR_T,
        .fragment_id = 0,
        .fragment_offset = 0,
        .reserved = 0
    };

    uint8_t buffer[1024];
    int len = cw_generate_header(&header, buffer, sizeof(buffer));

    TEST_ASSERT(len > 0, "Header generation succeeded");
    TEST_ASSERT(len == 8, "Header length is 8 bytes");

    cw_header_t parsed_header;
    int parsed_len = cw_parse_header(buffer, len, &parsed_header);

    TEST_ASSERT(parsed_len > 0, "Header parsing succeeded");
    TEST_ASSERT(parsed_header.preamble == header.preamble, "Preamble matches");
    TEST_ASSERT(parsed_header.flags == header.flags, "Flags match");
}

void test_control_msg_init(void) {
    printf("\n=== Test: Control Message Initialization ===\n");

    cw_control_msg_t msg;
    cw_init_control_msg(&msg, CW_MSG_TYPE_DISCOVERY_REQUEST, 1);

    TEST_ASSERT(msg.ctrl_header.message_type == CW_MSG_TYPE_DISCOVERY_REQUEST,
                "Message type set correctly");
    TEST_ASSERT(msg.ctrl_header.seq_num == 1, "Sequence number set correctly");
    TEST_ASSERT(msg.element_count == 0, "Element count initialized to 0");
    TEST_ASSERT(msg.header.flags & CW_FLAG_HDR_T, "Control flag set");
}

void test_add_element(void) {
    printf("\n=== Test: Add Message Element ===\n");

    cw_control_msg_t msg;
    cw_init_control_msg(&msg, CW_MSG_TYPE_DISCOVERY_REQUEST, 1);

    uint8_t test_data[] = {1, 2, 3, 4};
    int ret = cw_add_element(&msg, CW_ELEM_DISCOVERY_TYPE, test_data, sizeof(test_data));

    TEST_ASSERT(ret == CW_MSG_SUCCESS, "Element added successfully");
    TEST_ASSERT(msg.element_count == 1, "Element count is 1");
    TEST_ASSERT(msg.elements[0].type == CW_ELEM_DISCOVERY_TYPE, "Element type is correct");
    TEST_ASSERT(msg.elements[0].length == 4, "Element length is correct");
}

void test_find_element(void) {
    printf("\n=== Test: Find Message Element ===\n");

    cw_control_msg_t msg;
    cw_init_control_msg(&msg, CW_MSG_TYPE_DISCOVERY_REQUEST, 1);

    uint8_t test_data1[] = {1};
    uint8_t test_data2[] = {2, 3};

    cw_add_element(&msg, CW_ELEM_DISCOVERY_TYPE, test_data1, sizeof(test_data1));
    cw_add_element(&msg, CW_ELEM_WTP_NAME, test_data2, sizeof(test_data2));

    const cw_msg_element_t *elem = cw_find_element(&msg, CW_ELEM_WTP_NAME);

    TEST_ASSERT(elem != NULL, "Element found");
    TEST_ASSERT(elem->type == CW_ELEM_WTP_NAME, "Found correct element type");
    TEST_ASSERT(elem->length == 2, "Element has correct length");

    const cw_msg_element_t *not_found = cw_find_element(&msg, CW_ELEM_AC_DESCRIPTOR);
    TEST_ASSERT(not_found == NULL, "Non-existent element returns NULL");
}

void test_discovery_request_generation(void) {
    printf("\n=== Test: Discovery Request Generation ===\n");

    cw_control_msg_t msg;
    cw_init_control_msg(&msg, CW_MSG_TYPE_DISCOVERY_REQUEST, 1);

    /* Add Discovery Type element */
    uint8_t discovery_type = CW_DISCOVERY_TYPE_DHCP;
    cw_add_element(&msg, CW_ELEM_DISCOVERY_TYPE, &discovery_type, 1);

    /* Add WTP Descriptor (simplified) */
    uint8_t wtp_desc[] = {2, 1}; /* max_radios=2, radios_in_use=1 */
    cw_add_element(&msg, CW_ELEM_WTP_DESCRIPTOR, wtp_desc, sizeof(wtp_desc));

    uint8_t buffer[1024];
    int len = cw_generate_control_msg(&msg, buffer, sizeof(buffer));

    TEST_ASSERT(len > 0, "Discovery request generated");
    TEST_ASSERT(len > 16, "Message has reasonable size");

    /* Try to parse it back */
    cw_control_msg_t parsed_msg;
    int parsed_len = cw_parse_control_msg(buffer, len, &parsed_msg);

    TEST_ASSERT(parsed_len > 0, "Discovery request parsed");
    TEST_ASSERT(parsed_msg.ctrl_header.message_type == CW_MSG_TYPE_DISCOVERY_REQUEST,
                "Message type preserved");
    TEST_ASSERT(parsed_msg.element_count == 2, "Element count preserved");
}

void test_discovery_response_generation(void) {
    printf("\n=== Test: Discovery Response Generation ===\n");

    cw_control_msg_t msg;
    cw_init_control_msg(&msg, CW_MSG_TYPE_DISCOVERY_RESPONSE, 1);

    /* Add AC Descriptor */
    cw_ac_descriptor_t ac_desc = {
        .stations = 10,
        .limit = 1000,
        .active_wtps = 5,
        .max_wtps = 100,
        .security = 0x01,
        .rmac_field = 0x01,
        .reserved = 0,
        .dtls_policy = 0x01
    };

    uint8_t ac_desc_buffer[128];
    int ac_desc_len = cw_generate_ac_descriptor(&ac_desc, ac_desc_buffer, sizeof(ac_desc_buffer));

    TEST_ASSERT(ac_desc_len > 0, "AC Descriptor generated");

    /* Skip the Type and Length fields when adding element */
    cw_add_element(&msg, CW_ELEM_AC_DESCRIPTOR, &ac_desc_buffer[4], ac_desc_len - 4);

    /* Add AC Name */
    const char *ac_name = "TestAC";
    cw_add_element(&msg, CW_ELEM_AC_NAME, (const uint8_t *)ac_name, strlen(ac_name));

    uint8_t buffer[1024];
    int len = cw_generate_control_msg(&msg, buffer, sizeof(buffer));

    TEST_ASSERT(len > 0, "Discovery response generated");

    /* Parse it back */
    cw_control_msg_t parsed_msg;
    int parsed_len = cw_parse_control_msg(buffer, len, &parsed_msg);

    TEST_ASSERT(parsed_len > 0, "Discovery response parsed");
    TEST_ASSERT(parsed_msg.ctrl_header.message_type == CW_MSG_TYPE_DISCOVERY_RESPONSE,
                "Message type is Discovery Response");
}

void test_result_code_generation_parsing(void) {
    printf("\n=== Test: Result Code Element ===\n");

    uint8_t buffer[128];
    int len = cw_generate_result_code(CW_RESULT_SUCCESS, buffer, sizeof(buffer));

    TEST_ASSERT(len > 0, "Result code generated");
    TEST_ASSERT(len == 8, "Result code element is 8 bytes");

    /* Parse the element */
    cw_msg_element_t elem;
    elem.type = ntohs(*((uint16_t *)&buffer[0]));
    elem.length = ntohs(*((uint16_t *)&buffer[2]));
    elem.value = &buffer[4];

    TEST_ASSERT(elem.type == CW_ELEM_RESULT_CODE, "Element type is Result Code");
    TEST_ASSERT(elem.length == 4, "Element length is 4");

    cw_result_code_t result;
    int ret = cw_parse_result_code(&elem, &result);

    TEST_ASSERT(ret == CW_MSG_SUCCESS, "Result code parsed");
    TEST_ASSERT(result.result_code == CW_RESULT_SUCCESS, "Result code value is SUCCESS");
}

void test_ac_name_generation_parsing(void) {
    printf("\n=== Test: AC Name Element ===\n");

    const char *test_name = "MyAccessController";
    uint8_t buffer[128];
    int len = cw_generate_ac_name(test_name, buffer, sizeof(buffer));

    TEST_ASSERT(len > 0, "AC name generated");
    TEST_ASSERT(len == 4 + strlen(test_name), "AC name element has correct size");

    /* Parse the element */
    cw_msg_element_t elem;
    elem.type = ntohs(*((uint16_t *)&buffer[0]));
    elem.length = ntohs(*((uint16_t *)&buffer[2]));
    elem.value = &buffer[4];

    TEST_ASSERT(elem.type == CW_ELEM_AC_NAME, "Element type is AC Name");

    cw_ac_name_t ac_name;
    int ret = cw_parse_ac_name(&elem, &ac_name);

    TEST_ASSERT(ret == CW_MSG_SUCCESS, "AC name parsed");
    TEST_ASSERT(ac_name.length == strlen(test_name), "AC name length matches");
    TEST_ASSERT(strncmp(ac_name.name, test_name, ac_name.length) == 0, "AC name content matches");
}

void test_session_id_generation_parsing(void) {
    printf("\n=== Test: Session ID Element ===\n");

    cw_session_id_t session_id;
    session_id.length = 16;
    for (int i = 0; i < 16; i++) {
        session_id.session_id[i] = i;
    }

    uint8_t buffer[128];
    int len = cw_generate_session_id(&session_id, buffer, sizeof(buffer));

    TEST_ASSERT(len > 0, "Session ID generated");
    TEST_ASSERT(len == 20, "Session ID element is 20 bytes");

    /* Parse the element */
    cw_msg_element_t elem;
    elem.type = ntohs(*((uint16_t *)&buffer[0]));
    elem.length = ntohs(*((uint16_t *)&buffer[2]));
    elem.value = &buffer[4];

    cw_session_id_t parsed_session_id;
    int ret = cw_parse_session_id(&elem, &parsed_session_id);

    TEST_ASSERT(ret == CW_MSG_SUCCESS, "Session ID parsed");
    TEST_ASSERT(parsed_session_id.length == 16, "Session ID length matches");
    TEST_ASSERT(memcmp(parsed_session_id.session_id, session_id.session_id, 16) == 0,
                "Session ID content matches");
}

void test_echo_request_response(void) {
    printf("\n=== Test: Echo Request/Response ===\n");

    /* Echo Request */
    cw_control_msg_t req_msg;
    cw_init_control_msg(&req_msg, CW_MSG_TYPE_ECHO_REQUEST, 10);

    uint8_t req_buffer[1024];
    int req_len = cw_generate_control_msg(&req_msg, req_buffer, sizeof(req_buffer));

    TEST_ASSERT(req_len > 0, "Echo request generated");

    /* Echo Response */
    cw_control_msg_t resp_msg;
    cw_init_control_msg(&resp_msg, CW_MSG_TYPE_ECHO_RESPONSE, 10);

    uint8_t resp_buffer[1024];
    int resp_len = cw_generate_control_msg(&resp_msg, resp_buffer, sizeof(resp_buffer));

    TEST_ASSERT(resp_len > 0, "Echo response generated");

    /* Parse and verify */
    cw_control_msg_t parsed_req;
    cw_parse_control_msg(req_buffer, req_len, &parsed_req);
    TEST_ASSERT(parsed_req.ctrl_header.message_type == CW_MSG_TYPE_ECHO_REQUEST,
                "Echo request type correct");
    TEST_ASSERT(parsed_req.ctrl_header.seq_num == 10, "Echo request sequence correct");

    cw_control_msg_t parsed_resp;
    cw_parse_control_msg(resp_buffer, resp_len, &parsed_resp);
    TEST_ASSERT(parsed_resp.ctrl_header.message_type == CW_MSG_TYPE_ECHO_RESPONSE,
                "Echo response type correct");
}

void test_msg_type_names(void) {
    printf("\n=== Test: Message Type Names ===\n");

    TEST_ASSERT(strcmp(cw_msg_type_name(CW_MSG_TYPE_DISCOVERY_REQUEST), "Discovery Request") == 0,
                "Discovery Request name");
    TEST_ASSERT(strcmp(cw_msg_type_name(CW_MSG_TYPE_JOIN_RESPONSE), "Join Response") == 0,
                "Join Response name");
    TEST_ASSERT(strcmp(cw_msg_type_name(CW_MSG_TYPE_ECHO_REQUEST), "Echo Request") == 0,
                "Echo Request name");
    TEST_ASSERT(strcmp(cw_msg_type_name(9999), "Unknown") == 0,
                "Unknown message type");
}

void test_elem_type_names(void) {
    printf("\n=== Test: Element Type Names ===\n");

    TEST_ASSERT(strcmp(cw_elem_type_name(CW_ELEM_AC_DESCRIPTOR), "AC Descriptor") == 0,
                "AC Descriptor name");
    TEST_ASSERT(strcmp(cw_elem_type_name(CW_ELEM_SESSION_ID), "Session ID") == 0,
                "Session ID name");
    TEST_ASSERT(strcmp(cw_elem_type_name(CW_ELEM_RESULT_CODE), "Result Code") == 0,
                "Result Code name");
    TEST_ASSERT(strcmp(cw_elem_type_name(9999), "Unknown") == 0,
                "Unknown element type");
}

void test_msg_error_strings(void) {
    printf("\n=== Test: Message Error Strings ===\n");

    TEST_ASSERT(strcmp(cw_msg_error_string(CW_MSG_SUCCESS), "Success") == 0,
                "SUCCESS error string");
    TEST_ASSERT(strcmp(cw_msg_error_string(CW_MSG_ERROR_INVALID_PARAM), "Invalid parameter") == 0,
                "INVALID_PARAM error string");
    TEST_ASSERT(strcmp(cw_msg_error_string(CW_MSG_ERROR_BUFFER_TOO_SMALL), "Buffer too small") == 0,
                "BUFFER_TOO_SMALL error string");
    TEST_ASSERT(strcmp(cw_msg_error_string(CW_MSG_ERROR_PARSE_FAILED), "Parse failed") == 0,
                "PARSE_FAILED error string");
    TEST_ASSERT(strcmp(cw_msg_error_string(CW_MSG_ERROR_UNSUPPORTED_TYPE), "Unsupported type") == 0,
                "UNSUPPORTED_TYPE error string");
    TEST_ASSERT(strcmp(cw_msg_error_string(CW_MSG_ERROR_INVALID_FORMAT), "Invalid format") == 0,
                "INVALID_FORMAT error string");
    TEST_ASSERT(strcmp(cw_msg_error_string(-999), "Unknown error") == 0,
                "Unknown error string");
}

int main(void) {
    printf("======================================\n");
    printf("  CW-MSG-LIB Unit Tests\n");
    printf("======================================\n");

    test_header_generation_parsing();
    test_control_msg_init();
    test_add_element();
    test_find_element();
    test_discovery_request_generation();
    test_discovery_response_generation();
    test_result_code_generation_parsing();
    test_ac_name_generation_parsing();
    test_session_id_generation_parsing();
    test_echo_request_response();
    test_msg_type_names();
    test_elem_type_names();
    test_msg_error_strings();

    printf("\n======================================\n");
    printf("Test Summary:\n");
    printf("  Total:  %d\n", test_count);
    printf("  Passed: %d\n", test_passed);
    printf("  Failed: %d\n", test_failed);
    printf("======================================\n");

    return (test_failed == 0) ? 0 : 1;
}
