#include "include/capwap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("CAPWAP Library Example\n");
    printf("=====================\n");
    
    // Example 1: Building a Discovery Request message
    printf("\n1. Building a Discovery Request message:\n");
    
    uint8_t buffer[1024];
    size_t length;
    
    capwap_discovery_req_t discovery_req = {0};
    discovery_req.discovery_type = CAPWAP_DISCOVERY_TYPE_DHCP;
    
    int result = capwap_build_discovery_request(buffer, &length, sizeof(buffer), &discovery_req);
    if (result == CAPWAP_SUCCESS) {
        printf("   Discovery Request built successfully, length: %zu bytes\n", length);
    } else {
        printf("   Error building Discovery Request: %d\n", result);
        return 1;
    }
    
    // Example 2: Parsing the Discovery Request message
    printf("\n2. Parsing the Discovery Request message:\n");
    
    capwap_discovery_req_t parsed_discovery_req = {0};
    result = capwap_parse_discovery_request(buffer, length, &parsed_discovery_req);
    if (result == CAPWAP_SUCCESS) {
        printf("   Discovery Request parsed successfully\n");
        printf("   Discovery Type: %d\n", parsed_discovery_req.discovery_type);
    } else {
        printf("   Error parsing Discovery Request: %d\n", result);
        return 1;
    }
    
    // Example 3: Building a Join Request message
    printf("\n3. Building a Join Request message:\n");
    
    capwap_join_req_t join_req = {0};
    uint8_t wtp_descriptor[20] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
                                  0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14};
    join_req.wtp_descriptor = wtp_descriptor;
    join_req.wtp_descriptor_len = 20;
    
    result = capwap_build_join_request(buffer, &length, sizeof(buffer), &join_req);
    if (result == CAPWAP_SUCCESS) {
        printf("   Join Request built successfully, length: %zu bytes\n", length);
    } else {
        printf("   Error building Join Request: %d\n", result);
        return 1;
    }
    
    // Example 4: Parsing the Join Request message
    printf("\n4. Parsing the Join Request message:\n");
    
    capwap_join_req_t parsed_join_req = {0};
    result = capwap_parse_join_request(buffer, length, &parsed_join_req);
    if (result == CAPWAP_SUCCESS) {
        printf("   Join Request parsed successfully\n");
        printf("   WTP Descriptor Length: %zu\n", parsed_join_req.wtp_descriptor_len);
        // Note: parsed_join_req.wtp_descriptor points to data within the buffer
    } else {
        printf("   Error parsing Join Request: %d\n", result);
        return 1;
    }
    
    // Example 5: Building an Echo Response message
    printf("\n5. Building an Echo Response message:\n");
    
    capwap_echo_resp_t echo_resp = {0};
    echo_resp.result_code = CAPWAP_RESULT_SUCCESS;
    
    result = capwap_build_echo_response(buffer, &length, sizeof(buffer), &echo_resp);
    if (result == CAPWAP_SUCCESS) {
        printf("   Echo Response built successfully, length: %zu bytes\n", length);
    } else {
        printf("   Error building Echo Response: %d\n", result);
        return 1;
    }
    
    // Example 6: Parsing the Echo Response message
    printf("\n6. Parsing the Echo Response message:\n");
    
    capwap_echo_resp_t parsed_echo_resp = {0};
    result = capwap_parse_echo_response(buffer, length, &parsed_echo_resp);
    if (result == CAPWAP_SUCCESS) {
        printf("   Echo Response parsed successfully\n");
        printf("   Result Code: %d\n", parsed_echo_resp.result_code);
    } else {
        printf("   Error parsing Echo Response: %d\n", result);
        return 1;
    }
    
    // Example 7: Handling buffer overflow
    printf("\n7. Testing buffer overflow protection:\n");
    
    uint8_t small_buffer[5];  // Too small for any CAPWAP message
    size_t small_length = 5;
    
    result = capwap_build_discovery_request(small_buffer, &small_length, 5, &discovery_req);
    if (result == CAPWAP_ERROR_BUFFER_OVERFLOW) {
        printf("   Buffer overflow correctly detected: %d\n", result);
    } else {
        printf("   Unexpected result for buffer overflow test: %d\n", result);
    }
    
    // Example 8: Building a header directly
    printf("\n8. Building a CAPWAP header directly:\n");
    
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
    
    small_length = sizeof(small_buffer);
    result = capwap_build_header(small_buffer, &small_length, &header);
    if (result == CAPWAP_SUCCESS) {
        printf("   Header built successfully, length: %zu bytes\n", small_length);
        
        // Parse the header back
        capwap_header_t parsed_header = {0};
        result = capwap_parse_header(small_buffer, small_length, &parsed_header);
        if (result == CAPWAP_SUCCESS) {
            printf("   Header parsed successfully\n");
            printf("   Version: %d, Type: %d, WTP/AC ID: %d\n", 
                   parsed_header.version, parsed_header.type, parsed_header.wtp_ac_id);
        }
    } else {
        printf("   Error building header: %d\n", result);
    }
    
    printf("\nAll examples completed successfully!\n");
    return 0;
}