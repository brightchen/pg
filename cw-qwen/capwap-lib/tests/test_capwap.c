#include "../include/capwap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Helper function to print test results
void print_test_result(const char* test_name, int result) {
    if (result == CAPWAP_SUCCESS) {
        printf("PASS: %s\n", test_name);
    } else {
        printf("FAIL: %s (result: %d)\n", test_name, result);
    }
}

// Test basic header parsing and building
void test_header_functions() {
    printf("\n=== Testing Header Functions ===\n");
    
    // Test buffer
    uint8_t buffer[256];
    size_t len;
    
    // Create a header
    capwap_header_t header = {0};
    header.version = 0;  // CAPWAP version 1
    header.type = 1;     // Control message
    header.wireless_binding = 0;  // IEEE 802.11
    header.header_length = 2;  // 8 bytes
    header.flags = 0;
    header.fragment_id = 123;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 1;
    header.wtp_ac_id = 456;
    header.session_id = 0x12345678;
    
    // Test building header
    len = sizeof(buffer);
    int result = capwap_build_header(buffer, &len, &header);
    print_test_result("capwap_build_header", result);
    
    // Test parsing header
    if (result == CAPWAP_SUCCESS) {
        capwap_header_t parsed_header;
        result = capwap_parse_header(buffer, len, &parsed_header);
        print_test_result("capwap_parse_header", result);
        
        // Verify parsed values match original
        if (result == CAPWAP_SUCCESS) {
            int match = (parsed_header.version == header.version &&
                         parsed_header.type == header.type &&
                         parsed_header.wireless_binding == header.wireless_binding &&
                         parsed_header.fragment_id == header.fragment_id &&
                         parsed_header.radio_id == header.radio_id &&
                         parsed_header.wtp_ac_id == header.wtp_ac_id &&
                         parsed_header.session_id == header.session_id);
            
            if (match) {
                printf("PASS: Header values match after parse/build cycle\n");
            } else {
                printf("FAIL: Header values don't match after parse/build cycle\n");
            }
        }
    }
}

// Test message element functions
void test_message_element_functions() {
    printf("\n=== Testing Message Element Functions ===\n");
    
    uint8_t buffer[256];
    size_t len;
    
    // Create a message element
    capwap_msg_element_t elem = {0};
    elem.type = 1;  // AC Descriptor
    elem.length = 8;
    uint8_t data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    elem.data = data;
    
    // Test building message element
    len = sizeof(buffer);
    int result = capwap_build_message_element(buffer, &len, &elem);
    print_test_result("capwap_build_message_element", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing message element
        capwap_msg_element_t parsed_elem;
        size_t bytes_consumed;
        result = capwap_parse_message_element(buffer, len, &parsed_elem, &bytes_consumed);
        print_test_result("capwap_parse_message_element", result);
        
        if (result == CAPWAP_SUCCESS) {
            int match = (parsed_elem.type == elem.type && parsed_elem.length == elem.length);
            if (match) {
                printf("PASS: Message element values match after parse/build cycle\n");
            } else {
                printf("FAIL: Message element values don't match after parse/build cycle\n");
            }
        }
    }
}

// Test Discovery Request message
void test_discovery_request() {
    printf("\n=== Testing Discovery Request Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create discovery request message
    capwap_discovery_req_t msg = {0};
    msg.discovery_type = CAPWAP_DISCOVERY_TYPE_DHCP;
    
    // Test building discovery request
    len = sizeof(buffer);
    int result = capwap_build_discovery_request(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_discovery_request", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing discovery request
        capwap_discovery_req_t parsed_msg = {0};
        result = capwap_parse_discovery_request(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_discovery_request", result);
        
        if (result == CAPWAP_SUCCESS) {
            if (parsed_msg.discovery_type == msg.discovery_type) {
                printf("PASS: Discovery request values match after parse/build cycle\n");
            } else {
                printf("FAIL: Discovery request values don't match after parse/build cycle\n");
            }
        }
    }
}

// Test Discovery Response message
void test_discovery_response() {
    printf("\n=== Testing Discovery Response Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create discovery response message
    capwap_discovery_resp_t msg = {0};
    msg.result_code = CAPWAP_RESULT_SUCCESS;
    uint8_t ac_desc[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    msg.ac_descriptor = ac_desc;
    msg.ac_descriptor_len = 10;
    
    // Test building discovery response
    len = sizeof(buffer);
    int result = capwap_build_discovery_response(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_discovery_response", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing discovery response
        capwap_discovery_resp_t parsed_msg = {0};
        result = capwap_parse_discovery_response(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_discovery_response", result);
        
        if (result == CAPWAP_SUCCESS) {
            if (parsed_msg.result_code == msg.result_code) {
                printf("PASS: Discovery response values match after parse/build cycle\n");
            } else {
                printf("FAIL: Discovery response values don't match after parse/build cycle\n");
            }
        }
    }
}

// Test Join Request message
void test_join_request() {
    printf("\n=== Testing Join Request Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create join request message
    capwap_join_req_t msg = {0};
    uint8_t wtp_desc[20] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
                            0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14};
    msg.wtp_descriptor = wtp_desc;
    msg.wtp_descriptor_len = 20;
    
    // Test building join request
    len = sizeof(buffer);
    int result = capwap_build_join_request(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_join_request", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing join request
        capwap_join_req_t parsed_msg = {0};
        result = capwap_parse_join_request(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_join_request", result);
        
        if (result == CAPWAP_SUCCESS) {
            if (parsed_msg.wtp_descriptor_len == msg.wtp_descriptor_len) {
                printf("PASS: Join request values match after parse/build cycle\n");
            } else {
                printf("FAIL: Join request values don't match after parse/build cycle\n");
            }
        }
    }
}

// Test Join Response message
void test_join_response() {
    printf("\n=== Testing Join Response Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create join response message
    capwap_join_resp_t msg = {0};
    msg.result_code = CAPWAP_RESULT_SUCCESS;
    uint8_t session_id[16] = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88,
                              0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00};
    msg.session_id = session_id;
    
    // Test building join response
    len = sizeof(buffer);
    int result = capwap_build_join_response(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_join_response", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing join response
        capwap_join_resp_t parsed_msg = {0};
        result = capwap_parse_join_response(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_join_response", result);
        
        if (result == CAPWAP_SUCCESS) {
            if (parsed_msg.result_code == msg.result_code) {
                printf("PASS: Join response values match after parse/build cycle\n");
            } else {
                printf("FAIL: Join response values don't match after parse/build cycle\n");
            }
        }
    }
}

// Test Echo Request message
void test_echo_request() {
    printf("\n=== Testing Echo Request Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create echo request message
    capwap_echo_req_t msg = {0};
    
    // Test building echo request
    len = sizeof(buffer);
    int result = capwap_build_echo_request(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_echo_request", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing echo request
        capwap_echo_req_t parsed_msg = {0};
        result = capwap_parse_echo_request(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_echo_request", result);
        
        if (result == CAPWAP_SUCCESS) {
            printf("PASS: Echo request parsed successfully\n");
        }
    }
}

// Test Echo Response message
void test_echo_response() {
    printf("\n=== Testing Echo Response Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create echo response message
    capwap_echo_resp_t msg = {0};
    msg.result_code = CAPWAP_RESULT_SUCCESS;
    
    // Test building echo response
    len = sizeof(buffer);
    int result = capwap_build_echo_response(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_echo_response", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing echo response
        capwap_echo_resp_t parsed_msg = {0};
        result = capwap_parse_echo_response(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_echo_response", result);
        
        if (result == CAPWAP_SUCCESS) {
            if (parsed_msg.result_code == msg.result_code) {
                printf("PASS: Echo response values match after parse/build cycle\n");
            } else {
                printf("FAIL: Echo response values don't match after parse/build cycle\n");
            }
        }
    }
}

// Test Configuration Status Request
void test_config_status_request() {
    printf("\n=== Testing Configuration Status Request Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create config status request message
    capwap_config_status_req_t msg = {0};
    uint8_t config_updates[15] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
                                  0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    msg.configuration_updates = config_updates;
    msg.config_updates_len = 15;
    
    // Test building config status request
    len = sizeof(buffer);
    int result = capwap_build_config_status_request(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_config_status_request", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing config status request
        capwap_config_status_req_t parsed_msg = {0};
        result = capwap_parse_config_status_request(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_config_status_request", result);
        
        if (result == CAPWAP_SUCCESS) {
            if (parsed_msg.config_updates_len == msg.config_updates_len) {
                printf("PASS: Config status request values match after parse/build cycle\n");
            } else {
                printf("FAIL: Config status request values don't match after parse/build cycle\n");
            }
        }
    }
}

// Test Configuration Status Response
void test_config_status_response() {
    printf("\n=== Testing Configuration Status Response Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create config status response message
    capwap_config_status_resp_t msg = {0};
    msg.result_code = CAPWAP_RESULT_SUCCESS;
    
    // Test building config status response
    len = sizeof(buffer);
    int result = capwap_build_config_status_response(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_config_status_response", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing config status response
        capwap_config_status_resp_t parsed_msg = {0};
        result = capwap_parse_config_status_response(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_config_status_response", result);
        
        if (result == CAPWAP_SUCCESS) {
            if (parsed_msg.result_code == msg.result_code) {
                printf("PASS: Config status response values match after parse/build cycle\n");
            } else {
                printf("FAIL: Config status response values don't match after parse/build cycle\n");
            }
        }
    }
}

// Test Configuration Update Request
void test_config_update_request() {
    printf("\n=== Testing Configuration Update Request Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create config update request message
    capwap_config_update_req_t msg = {0};
    
    // Test building config update request
    len = sizeof(buffer);
    int result = capwap_build_config_update_request(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_config_update_request", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing config update request
        capwap_config_update_req_t parsed_msg = {0};
        result = capwap_parse_config_update_request(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_config_update_request", result);
        
        if (result == CAPWAP_SUCCESS) {
            printf("PASS: Config update request parsed successfully\n");
        }
    }
}

// Test Configuration Update Response
void test_config_update_response() {
    printf("\n=== Testing Configuration Update Response Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create config update response message
    capwap_config_update_resp_t msg = {0};
    msg.result_code = CAPWAP_RESULT_SUCCESS;
    
    // Test building config update response
    len = sizeof(buffer);
    int result = capwap_build_config_update_response(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_config_update_response", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing config update response
        capwap_config_update_resp_t parsed_msg = {0};
        result = capwap_parse_config_update_response(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_config_update_response", result);
        
        if (result == CAPWAP_SUCCESS) {
            if (parsed_msg.result_code == msg.result_code) {
                printf("PASS: Config update response values match after parse/build cycle\n");
            } else {
                printf("FAIL: Config update response values don't match after parse/build cycle\n");
            }
        }
    }
}

// Test Change State Event Request
void test_change_state_event_request() {
    printf("\n=== Testing Change State Event Request Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create change state event request message
    capwap_change_state_event_req_t msg = {0};
    
    // Test building change state event request
    len = sizeof(buffer);
    int result = capwap_build_change_state_event_request(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_change_state_event_request", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing change state event request
        capwap_change_state_event_req_t parsed_msg = {0};
        result = capwap_parse_change_state_event_request(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_change_state_event_request", result);
        
        if (result == CAPWAP_SUCCESS) {
            printf("PASS: Change state event request parsed successfully\n");
        }
    }
}

// Test Change State Event Response
void test_change_state_event_response() {
    printf("\n=== Testing Change State Event Response Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create change state event response message
    capwap_change_state_event_resp_t msg = {0};
    msg.result_code = CAPWAP_RESULT_SUCCESS;
    
    // Test building change state event response
    len = sizeof(buffer);
    int result = capwap_build_change_state_event_response(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_change_state_event_response", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing change state event response
        capwap_change_state_event_resp_t parsed_msg = {0};
        result = capwap_parse_change_state_event_response(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_change_state_event_response", result);
        
        if (result == CAPWAP_SUCCESS) {
            if (parsed_msg.result_code == msg.result_code) {
                printf("PASS: Change state event response values match after parse/build cycle\n");
            } else {
                printf("FAIL: Change state event response values don't match after parse/build cycle\n");
            }
        }
    }
}

// Test Primary Discovery Request
void test_primary_discovery_request() {
    printf("\n=== Testing Primary Discovery Request Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create primary discovery request message
    capwap_primary_discovery_req_t msg = {0};
    msg.discovery_type = CAPWAP_DISCOVERY_TYPE_DNS;
    
    // Test building primary discovery request
    len = sizeof(buffer);
    int result = capwap_build_primary_discovery_request(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_primary_discovery_request", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing primary discovery request
        capwap_primary_discovery_req_t parsed_msg = {0};
        result = capwap_parse_primary_discovery_request(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_primary_discovery_request", result);
        
        if (result == CAPWAP_SUCCESS) {
            if (parsed_msg.discovery_type == msg.discovery_type) {
                printf("PASS: Primary discovery request values match after parse/build cycle\n");
            } else {
                printf("FAIL: Primary discovery request values don't match after parse/build cycle\n");
            }
        }
    }
}

// Test Primary Discovery Response
void test_primary_discovery_response() {
    printf("\n=== Testing Primary Discovery Response Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create primary discovery response message
    capwap_primary_discovery_resp_t msg = {0};
    msg.result_code = CAPWAP_RESULT_SUCCESS;
    
    // Test building primary discovery response
    len = sizeof(buffer);
    int result = capwap_build_primary_discovery_response(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_primary_discovery_response", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing primary discovery response
        capwap_primary_discovery_resp_t parsed_msg = {0};
        result = capwap_parse_primary_discovery_response(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_primary_discovery_response", result);
        
        if (result == CAPWAP_SUCCESS) {
            if (parsed_msg.result_code == msg.result_code) {
                printf("PASS: Primary discovery response values match after parse/build cycle\n");
            } else {
                printf("FAIL: Primary discovery response values don't match after parse/build cycle\n");
            }
        }
    }
}

// Test Data Transfer Request
void test_data_transfer_request() {
    printf("\n=== Testing Data Transfer Request Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create data transfer request message
    capwap_data_transfer_req_t msg = {0};
    
    // Test building data transfer request
    len = sizeof(buffer);
    int result = capwap_build_data_transfer_request(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_data_transfer_request", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing data transfer request
        capwap_data_transfer_req_t parsed_msg = {0};
        result = capwap_parse_data_transfer_request(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_data_transfer_request", result);
        
        if (result == CAPWAP_SUCCESS) {
            printf("PASS: Data transfer request parsed successfully\n");
        }
    }
}

// Test Data Transfer Response
void test_data_transfer_response() {
    printf("\n=== Testing Data Transfer Response Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create data transfer response message
    capwap_data_transfer_resp_t msg = {0};
    msg.result_code = CAPWAP_RESULT_SUCCESS;
    
    // Test building data transfer response
    len = sizeof(buffer);
    int result = capwap_build_data_transfer_response(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_data_transfer_response", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing data transfer response
        capwap_data_transfer_resp_t parsed_msg = {0};
        result = capwap_parse_data_transfer_response(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_data_transfer_response", result);
        
        if (result == CAPWAP_SUCCESS) {
            if (parsed_msg.result_code == msg.result_code) {
                printf("PASS: Data transfer response values match after parse/build cycle\n");
            } else {
                printf("FAIL: Data transfer response values don't match after parse/build cycle\n");
            }
        }
    }
}

// Test Clear Configuration Request
void test_clear_config_request() {
    printf("\n=== Testing Clear Configuration Request Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create clear config request message
    capwap_clear_config_req_t msg = {0};
    
    // Test building clear config request
    len = sizeof(buffer);
    int result = capwap_build_clear_config_request(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_clear_config_request", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing clear config request
        capwap_clear_config_req_t parsed_msg = {0};
        result = capwap_parse_clear_config_request(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_clear_config_request", result);
        
        if (result == CAPWAP_SUCCESS) {
            printf("PASS: Clear config request parsed successfully\n");
        }
    }
}

// Test Clear Configuration Response
void test_clear_config_response() {
    printf("\n=== Testing Clear Configuration Response Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create clear config response message
    capwap_clear_config_resp_t msg = {0};
    msg.result_code = CAPWAP_RESULT_SUCCESS;
    
    // Test building clear config response
    len = sizeof(buffer);
    int result = capwap_build_clear_config_response(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_clear_config_response", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing clear config response
        capwap_clear_config_resp_t parsed_msg = {0};
        result = capwap_parse_clear_config_response(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_clear_config_response", result);
        
        if (result == CAPWAP_SUCCESS) {
            if (parsed_msg.result_code == msg.result_code) {
                printf("PASS: Clear config response values match after parse/build cycle\n");
            } else {
                printf("FAIL: Clear config response values don't match after parse/build cycle\n");
            }
        }
    }
}

// Test Station Configuration Request
void test_station_config_request() {
    printf("\n=== Testing Station Configuration Request Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create station config request message
    capwap_station_config_req_t msg = {0};
    
    // Test building station config request
    len = sizeof(buffer);
    int result = capwap_build_station_config_request(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_station_config_request", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing station config request
        capwap_station_config_req_t parsed_msg = {0};
        result = capwap_parse_station_config_request(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_station_config_request", result);
        
        if (result == CAPWAP_SUCCESS) {
            printf("PASS: Station config request parsed successfully\n");
        }
    }
}

// Test Station Configuration Response
void test_station_config_response() {
    printf("\n=== Testing Station Configuration Response Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create station config response message
    capwap_station_config_resp_t msg = {0};
    msg.result_code = CAPWAP_RESULT_SUCCESS;
    
    // Test building station config response
    len = sizeof(buffer);
    int result = capwap_build_station_config_response(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_station_config_response", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing station config response
        capwap_station_config_resp_t parsed_msg = {0};
        result = capwap_parse_station_config_response(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_station_config_response", result);
        
        if (result == CAPWAP_SUCCESS) {
            if (parsed_msg.result_code == msg.result_code) {
                printf("PASS: Station config response values match after parse/build cycle\n");
            } else {
                printf("FAIL: Station config response values don't match after parse/build cycle\n");
            }
        }
    }
}

// Test WTP Event Request
void test_wtp_event_request() {
    printf("\n=== Testing WTP Event Request Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create WTP event request message
    capwap_wtp_event_req_t msg = {0};
    
    // Test building WTP event request
    len = sizeof(buffer);
    int result = capwap_build_wtp_event_request(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_wtp_event_request", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing WTP event request
        capwap_wtp_event_req_t parsed_msg = {0};
        result = capwap_parse_wtp_event_request(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_wtp_event_request", result);
        
        if (result == CAPWAP_SUCCESS) {
            printf("PASS: WTP event request parsed successfully\n");
        }
    }
}

// Test WTP Event Response
void test_wtp_event_response() {
    printf("\n=== Testing WTP Event Response Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create WTP event response message
    capwap_wtp_event_resp_t msg = {0};
    msg.result_code = CAPWAP_RESULT_SUCCESS;
    
    // Test building WTP event response
    len = sizeof(buffer);
    int result = capwap_build_wtp_event_response(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_wtp_event_response", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing WTP event response
        capwap_wtp_event_resp_t parsed_msg = {0};
        result = capwap_parse_wtp_event_response(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_wtp_event_response", result);
        
        if (result == CAPWAP_SUCCESS) {
            if (parsed_msg.result_code == msg.result_code) {
                printf("PASS: WTP event response values match after parse/build cycle\n");
            } else {
                printf("FAIL: WTP event response values don't match after parse/build cycle\n");
            }
        }
    }
}

// Test Reset Request
void test_reset_request() {
    printf("\n=== Testing Reset Request Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create reset request message
    capwap_reset_req_t msg = {0};
    msg.reset_type = 0;  // Cold reset
    
    // Test building reset request
    len = sizeof(buffer);
    int result = capwap_build_reset_request(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_reset_request", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing reset request
        capwap_reset_req_t parsed_msg = {0};
        result = capwap_parse_reset_request(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_reset_request", result);
        
        if (result == CAPWAP_SUCCESS) {
            printf("PASS: Reset request parsed successfully\n");
        }
    }
}

// Test Reset Response
void test_reset_response() {
    printf("\n=== Testing Reset Response Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create reset response message
    capwap_reset_resp_t msg = {0};
    msg.result_code = CAPWAP_RESULT_SUCCESS;
    
    // Test building reset response
    len = sizeof(buffer);
    int result = capwap_build_reset_response(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_reset_response", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing reset response
        capwap_reset_resp_t parsed_msg = {0};
        result = capwap_parse_reset_response(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_reset_response", result);
        
        if (result == CAPWAP_SUCCESS) {
            if (parsed_msg.result_code == msg.result_code) {
                printf("PASS: Reset response values match after parse/build cycle\n");
            } else {
                printf("FAIL: Reset response values don't match after parse/build cycle\n");
            }
        }
    }
}

// Test Image Data Request
void test_image_data_request() {
    printf("\n=== Testing Image Data Request Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create image data request message
    capwap_image_data_req_t msg = {0};
    
    // Test building image data request
    len = sizeof(buffer);
    int result = capwap_build_image_data_request(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_image_data_request", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing image data request
        capwap_image_data_req_t parsed_msg = {0};
        result = capwap_parse_image_data_request(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_image_data_request", result);
        
        if (result == CAPWAP_SUCCESS) {
            printf("PASS: Image data request parsed successfully\n");
        }
    }
}

// Test Image Data Response
void test_image_data_response() {
    printf("\n=== Testing Image Data Response Message ===\n");
    
    uint8_t buffer[1024];
    size_t len;
    
    // Create image data response message
    capwap_image_data_resp_t msg = {0};
    msg.result_code = CAPWAP_RESULT_SUCCESS;
    
    // Test building image data response
    len = sizeof(buffer);
    int result = capwap_build_image_data_response(buffer, &len, sizeof(buffer), &msg);
    print_test_result("capwap_build_image_data_response", result);
    
    if (result == CAPWAP_SUCCESS) {
        // Test parsing image data response
        capwap_image_data_resp_t parsed_msg = {0};
        result = capwap_parse_image_data_response(buffer, len, &parsed_msg);
        print_test_result("capwap_parse_image_data_response", result);
        
        if (result == CAPWAP_SUCCESS) {
            if (parsed_msg.result_code == msg.result_code) {
                printf("PASS: Image data response values match after parse/build cycle\n");
            } else {
                printf("FAIL: Image data response values don't match after parse/build cycle\n");
            }
        }
    }
}

// Test edge cases and error conditions
void test_error_conditions() {
    printf("\n=== Testing Error Conditions ===\n");
    
    uint8_t buffer[10];
    size_t len;
    capwap_header_t header = {0};
    
    // Test buffer overflow
    len = 5;  // Too small for header
    int result = capwap_build_header(buffer, &len, &header);
    if (result == CAPWAP_ERROR_BUFFER_OVERFLOW) {
        printf("PASS: Buffer overflow detected correctly\n");
    } else {
        printf("FAIL: Buffer overflow not detected (result: %d)\n", result);
    }
    
    // Test invalid input
    result = capwap_build_header(NULL, &len, &header);
    if (result == CAPWAP_ERROR_INVALID_INPUT) {
        printf("PASS: Invalid input detected correctly\n");
    } else {
        printf("FAIL: Invalid input not detected (result: %d)\n", result);
    }
    
    result = capwap_parse_header(NULL, 10, &header);
    if (result == CAPWAP_ERROR_INVALID_INPUT) {
        printf("PASS: Invalid input for parse detected correctly\n");
    } else {
        printf("FAIL: Invalid input for parse not detected (result: %d)\n", result);
    }
    
    // Test parsing with insufficient data
    uint8_t small_buffer[5] = {0};
    result = capwap_parse_header(small_buffer, 5, &header);
    if (result == CAPWAP_ERROR_INVALID_INPUT) {
        printf("PASS: Insufficient data for header parse detected correctly\n");
    } else {
        printf("FAIL: Insufficient data for header parse not detected (result: %d)\n", result);
    }
}

int main() {
    printf("CAPWAP Library Unit Tests\n");
    printf("========================\n");
    
    // Run all tests
    test_error_conditions();
    test_header_functions();
    test_message_element_functions();
    test_discovery_request();
    test_discovery_response();
    test_join_request();
    test_join_response();
    test_echo_request();
    test_echo_response();
    test_config_status_request();
    test_config_status_response();
    test_config_update_request();
    test_config_update_response();
    test_change_state_event_request();
    test_change_state_event_response();
    test_primary_discovery_request();
    test_primary_discovery_response();
    test_data_transfer_request();
    test_data_transfer_response();
    test_clear_config_request();
    test_clear_config_response();
    test_station_config_request();
    test_station_config_response();
    test_wtp_event_request();
    test_wtp_event_response();
    test_reset_request();
    test_reset_response();
    test_image_data_request();
    test_image_data_response();
    
    printf("\nAll tests completed.\n");
    return 0;
}