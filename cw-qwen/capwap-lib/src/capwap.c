#include "capwap.h"
#include <string.h>
#include <stdlib.h>

// Utility functions for network byte order conversion
uint16_t capwap_htons(uint16_t host_short) {
    return ((host_short & 0xFF) << 8) | ((host_short >> 8) & 0xFF);
}

uint32_t capwap_htonl(uint32_t host_long) {
    return ((host_long & 0xFF) << 24) | 
           (((host_long >> 8) & 0xFF) << 16) | 
           (((host_long >> 16) & 0xFF) << 8) | 
           ((host_long >> 24) & 0xFF);
}

uint16_t capwap_ntohs(uint16_t net_short) {
    return ((net_short & 0xFF) << 8) | ((net_short >> 8) & 0xFF);
}

uint32_t capwap_ntohl(uint32_t net_long) {
    return ((net_long & 0xFF) << 24) | 
           (((net_long >> 8) & 0xFF) << 16) | 
           (((net_long >> 16) & 0xFF) << 8) | 
           ((net_long >> 24) & 0xFF);
}

// Parse CAPWAP header
int capwap_parse_header(uint8_t *buffer, size_t len, capwap_header_t *header) {
    if (!buffer || !header || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse the header fields according to RFC 5415
    // Byte 0: [Version 4 bits][Type 2 bits][Wireless Binding 2 bits]
    header->version = (buffer[0] >> 4) & 0xF;
    header->type = (buffer[0] >> 2) & 0x3;
    header->wireless_binding = buffer[0] & 0x3;
    
    // Byte 1: [Header Length 5 bits][Flags 3 bits]
    header->header_length = (buffer[1] >> 3) & 0x1F;
    header->flags = buffer[1] & 0x7;
    
    header->fragment_id = capwap_ntohs(*(uint16_t*)(buffer + 2));
    
    uint16_t frag_offset_flags = capwap_ntohs(*(uint16_t*)(buffer + 4));
    header->fragment_offset = (frag_offset_flags >> 3) & 0x1FFF;
    header->final_fragment = (frag_offset_flags >> 2) & 0x1;
    header->reserved2 = frag_offset_flags & 0x3;
    
    header->radio_id = buffer[5];
    header->wtp_ac_id = capwap_ntohs(*(uint16_t*)(buffer + 6));
    header->session_id = capwap_ntohl(*(uint32_t*)(buffer + 8));

    // Validate version
    if (header->version != CAPWAP_VERSION_1) {
        return CAPWAP_ERROR_UNSUPPORTED_VERSION;
    }

    return CAPWAP_SUCCESS;
}

// Build CAPWAP header
int capwap_build_header(uint8_t *buffer, size_t *len, capwap_header_t *header) {
    if (!buffer || !len || !header) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    if (*len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_BUFFER_OVERFLOW;
    }

    // Set the header fields according to RFC 5415
    // Byte 0: [Version 4 bits][Type 2 bits][Wireless Binding 2 bits]
    buffer[0] = ((header->version & 0xF) << 4) | 
                ((header->type & 0x3) << 2) | 
                (header->wireless_binding & 0x3);
    
    // Byte 1: [Header Length 5 bits][Flags 3 bits]
    buffer[1] = ((header->header_length & 0x1F) << 3) | 
                (header->flags & 0x7);
    
    *(uint16_t*)(buffer + 2) = capwap_htons(header->fragment_id);
    
    uint16_t frag_offset_flags = ((header->fragment_offset & 0x1FFF) << 3) | 
                                ((header->final_fragment & 0x1) << 2) | 
                                (header->reserved2 & 0x3);
    *(uint16_t*)(buffer + 4) = capwap_htons(frag_offset_flags);
    
    buffer[5] = header->radio_id;
    *(uint16_t*)(buffer + 6) = capwap_htons(header->wtp_ac_id);
    *(uint32_t*)(buffer + 8) = capwap_htonl(header->session_id);

    *len = CAPWAP_HEADER_SIZE;
    return CAPWAP_SUCCESS;
}

// Parse a single message element
int capwap_parse_message_element(uint8_t *buffer, size_t len, capwap_msg_element_t *elem, size_t *bytes_consumed) {
    if (!buffer || !elem || !bytes_consumed || len < 4) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Read type and length (both in network byte order)
    elem->type = capwap_ntohs(*(uint16_t*)(buffer));
    elem->length = capwap_ntohs(*(uint16_t*)(buffer + 2));

    // Check if we have enough data for the element
    if (len < (4 + (size_t)elem->length)) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Set data pointer to point to the actual data within the buffer
    elem->data = buffer + 4;

    *bytes_consumed = 4 + elem->length;
    return CAPWAP_SUCCESS;
}

// Build a single message element
int capwap_build_message_element(uint8_t *buffer, size_t *len, capwap_msg_element_t *elem) {
    if (!buffer || !len || !elem) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Calculate required space
    size_t required_len = 4 + elem->length;

    if (*len < required_len) {
        return CAPWAP_ERROR_BUFFER_OVERFLOW;
    }

    // Write type and length in network byte order
    *(uint16_t*)(buffer) = capwap_htons(elem->type);
    *(uint16_t*)(buffer + 2) = capwap_htons(elem->length);

    // Copy the data
    if (elem->length > 0 && elem->data != NULL) {
        memcpy(buffer + 4, elem->data, elem->length);
    }

    *len = required_len;
    return CAPWAP_SUCCESS;
}

// Build Discovery Request message
int capwap_build_discovery_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_discovery_req_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = CAPWAP_DISCOVERY_REQUEST; // Set message type for Discovery Request
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int result = capwap_build_header(buffer, &header_len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    current_pos += header_len;

    // Add Discovery Type message element
    uint32_t network_discovery_type = capwap_htonl(msg->discovery_type);
    capwap_msg_element_t elem_discovery_type = {
        .type = capwap_htons(CAPWAP_ELEM_DISCOVERY_TYPE),
        .length = 4,
        .data = (uint8_t*)&network_discovery_type
    };
    
    uint8_t temp_buffer[8]; // Type(2) + Length(2) + Data(4)
    size_t elem_len = 8;
    result = capwap_build_message_element(temp_buffer, &elem_len, &elem_discovery_type);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    // Update the temp buffer to use the actual network byte order value
    *(uint32_t*)(temp_buffer + 4) = capwap_htonl(msg->discovery_type);
    elem_len = 8;
    
    if (current_pos + elem_len > capacity) {
        return CAPWAP_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(buffer + current_pos, temp_buffer, elem_len);
    current_pos += elem_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Discovery Request message
int capwap_parse_discovery_request(uint8_t *buffer, size_t len, capwap_discovery_req_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            case CAPWAP_ELEM_DISCOVERY_TYPE:
                if (elem.length == 4) {
                    msg->discovery_type = capwap_ntohl(*(uint32_t*)elem.data);
                }
                break;
                
            // Add other message elements as needed
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Discovery Response message
int capwap_build_discovery_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_discovery_resp_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = CAPWAP_DISCOVERY_RESPONSE; // Set message type for Discovery Response
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int result = capwap_build_header(buffer, &header_len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    current_pos += header_len;

    // Add Result Code message element
    uint32_t network_result_code = capwap_htonl(msg->result_code);
    capwap_msg_element_t elem_result_code = {
        .type = capwap_htons(CAPWAP_ELEM_RESULT_CODE),
        .length = 4,
        .data = (uint8_t*)&network_result_code
    };
    
    uint8_t temp_buffer[8]; // Type(2) + Length(2) + Data(4)
    size_t elem_len = 8;
    result = capwap_build_message_element(temp_buffer, &elem_len, &elem_result_code);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    // Update the temp buffer to use the actual network byte order value
    *(uint32_t*)(temp_buffer + 4) = capwap_htonl(msg->result_code);
    elem_len = 8;
    
    if (current_pos + elem_len > capacity) {
        return CAPWAP_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(buffer + current_pos, temp_buffer, elem_len);
    current_pos += elem_len;

    // Add AC Descriptor if present
    if (msg->ac_descriptor && msg->ac_descriptor_len > 0) {
        capwap_msg_element_t elem_ac_desc = {
            .type = capwap_htons(CAPWAP_ELEM_AC_DESCRIPTOR),
            .length = msg->ac_descriptor_len,
            .data = msg->ac_descriptor
        };
        
        uint8_t desc_buffer[65536]; // Max possible size for element
        size_t desc_elem_len = 4 + msg->ac_descriptor_len; // Type(2) + Length(2) + actual data
        if (desc_elem_len > sizeof(desc_buffer)) {
            return CAPWAP_ERROR_BUFFER_OVERFLOW;
        }
        
        result = capwap_build_message_element(desc_buffer, &desc_elem_len, &elem_ac_desc);
        if (result != CAPWAP_SUCCESS) {
            return result;
        }
        
        if (current_pos + desc_elem_len > capacity) {
            return CAPWAP_ERROR_BUFFER_OVERFLOW;
        }
        
        memcpy(buffer + current_pos, desc_buffer, desc_elem_len);
        current_pos += desc_elem_len;
    }

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Discovery Response message
int capwap_parse_discovery_response(uint8_t *buffer, size_t len, capwap_discovery_resp_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            case CAPWAP_ELEM_RESULT_CODE:
                if (elem.length == 4) {
                    msg->result_code = capwap_ntohl(*(uint32_t*)elem.data);
                }
                break;
                
            case CAPWAP_ELEM_AC_DESCRIPTOR:
                msg->ac_descriptor = elem.data;
                msg->ac_descriptor_len = elem.length;
                break;
                
            case CAPWAP_ELEM_AC_NAME:
                msg->ac_name = (char*)elem.data;
                break;
                
            // Add other message elements as needed
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Join Request message
int capwap_build_join_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_join_req_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = CAPWAP_JOIN_REQUEST; // Set message type for Join Request
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int result = capwap_build_header(buffer, &header_len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    current_pos += header_len;

    // Add WTP Descriptor message element if present
    if (msg->wtp_descriptor && msg->wtp_descriptor_len > 0) {
        capwap_msg_element_t elem_wtp_desc = {
            .type = capwap_htons(CAPWAP_ELEM_WTP_DESCRIPTOR),
            .length = msg->wtp_descriptor_len,
            .data = msg->wtp_descriptor
        };
        
        uint8_t desc_buffer[65536]; // Max possible size for element
        size_t desc_elem_len = 4 + msg->wtp_descriptor_len; // Type(2) + Length(2) + actual data
        if (desc_elem_len > sizeof(desc_buffer)) {
            return CAPWAP_ERROR_BUFFER_OVERFLOW;
        }
        
        result = capwap_build_message_element(desc_buffer, &desc_elem_len, &elem_wtp_desc);
        if (result != CAPWAP_SUCCESS) {
            return result;
        }
        
        if (current_pos + desc_elem_len > capacity) {
            return CAPWAP_ERROR_BUFFER_OVERFLOW;
        }
        
        memcpy(buffer + current_pos, desc_buffer, desc_elem_len);
        current_pos += desc_elem_len;
    }

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Join Request message
int capwap_parse_join_request(uint8_t *buffer, size_t len, capwap_join_req_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            case CAPWAP_ELEM_WTP_DESCRIPTOR:
                msg->wtp_descriptor = elem.data;
                msg->wtp_descriptor_len = elem.length;
                break;
                
            case CAPWAP_ELEM_WTP_BOARD_DATA:
                msg->wtp_board_data = elem.data;
                msg->wtp_board_data_len = elem.length;
                break;
                
            case CAPWAP_ELEM_CAPWAP_CONTROL_IPV4:
            case CAPWAP_ELEM_CAPWAP_CONTROL_IPV6:
                msg->ac_control_addr = elem.data;
                msg->ac_control_addr_len = elem.length;
                break;
                
            // Add other message elements as needed
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Join Response message
int capwap_build_join_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_join_resp_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = CAPWAP_JOIN_RESPONSE; // Set message type for Join Response
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int result = capwap_build_header(buffer, &header_len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    current_pos += header_len;

    // Add Result Code message element
    uint32_t network_result_code = capwap_htonl(msg->result_code);
    capwap_msg_element_t elem_result_code = {
        .type = capwap_htons(CAPWAP_ELEM_RESULT_CODE),
        .length = 4,
        .data = (uint8_t*)&network_result_code
    };
    
    uint8_t temp_buffer[8]; // Type(2) + Length(2) + Data(4)
    size_t elem_len = 8;
    result = capwap_build_message_element(temp_buffer, &elem_len, &elem_result_code);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    // Update the temp buffer to use the actual network byte order value
    *(uint32_t*)(temp_buffer + 4) = capwap_htonl(msg->result_code);
    elem_len = 8;
    
    if (current_pos + elem_len > capacity) {
        return CAPWAP_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(buffer + current_pos, temp_buffer, elem_len);
    current_pos += elem_len;

    // Add AC Descriptor if present
    if (msg->ac_descriptor && msg->ac_descriptor_len > 0) {
        capwap_msg_element_t elem_ac_desc = {
            .type = capwap_htons(CAPWAP_ELEM_AC_DESCRIPTOR),
            .length = msg->ac_descriptor_len,
            .data = msg->ac_descriptor
        };
        
        uint8_t desc_buffer[65536]; // Max possible size for element
        size_t desc_elem_len = 4 + msg->ac_descriptor_len; // Type(2) + Length(2) + actual data
        if (desc_elem_len > sizeof(desc_buffer)) {
            return CAPWAP_ERROR_BUFFER_OVERFLOW;
        }
        
        result = capwap_build_message_element(desc_buffer, &desc_elem_len, &elem_ac_desc);
        if (result != CAPWAP_SUCCESS) {
            return result;
        }
        
        if (current_pos + desc_elem_len > capacity) {
            return CAPWAP_ERROR_BUFFER_OVERFLOW;
        }
        
        memcpy(buffer + current_pos, desc_buffer, desc_elem_len);
        current_pos += desc_elem_len;
    }

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Join Response message
int capwap_parse_join_response(uint8_t *buffer, size_t len, capwap_join_resp_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            case CAPWAP_ELEM_RESULT_CODE:
                if (elem.length == 4) {
                    msg->result_code = capwap_ntohl(*(uint32_t*)elem.data);
                }
                break;
                
            case CAPWAP_ELEM_AC_DESCRIPTOR:
                msg->ac_descriptor = elem.data;
                msg->ac_descriptor_len = elem.length;
                break;
                
            case CAPWAP_ELEM_SESSION_ID:
                msg->session_id = elem.data;
                break;
                
            // Add other message elements as needed
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Echo Request message
int capwap_build_echo_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_echo_req_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = CAPWAP_ECHO_REQUEST; // Set message type for Echo Request
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int result = capwap_build_header(buffer, &header_len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    current_pos += header_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Echo Request message
int capwap_parse_echo_request(uint8_t *buffer, size_t len, capwap_echo_req_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            case CAPWAP_ELEM_AC_NAME:
                msg->ac_name = (char*)elem.data;
                break;
                
            // Add other message elements as needed
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Echo Response message
int capwap_build_echo_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_echo_resp_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = CAPWAP_ECHO_RESPONSE; // Set message type for Echo Response
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int result = capwap_build_header(buffer, &header_len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    current_pos += header_len;

    // Add Result Code message element
    uint32_t network_result_code = capwap_htonl(msg->result_code);
    capwap_msg_element_t elem_result_code = {
        .type = capwap_htons(CAPWAP_ELEM_RESULT_CODE),
        .length = 4,
        .data = (uint8_t*)&network_result_code
    };
    
    uint8_t temp_buffer[8]; // Type(2) + Length(2) + Data(4)
    size_t elem_len = 8;
    result = capwap_build_message_element(temp_buffer, &elem_len, &elem_result_code);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    // Update the temp buffer to use the actual network byte order value
    *(uint32_t*)(temp_buffer + 4) = capwap_htonl(msg->result_code);
    elem_len = 8;
    
    if (current_pos + elem_len > capacity) {
        return CAPWAP_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(buffer + current_pos, temp_buffer, elem_len);
    current_pos += elem_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Echo Response message
int capwap_parse_echo_response(uint8_t *buffer, size_t len, capwap_echo_resp_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            case CAPWAP_ELEM_RESULT_CODE:
                if (elem.length == 4) {
                    msg->result_code = capwap_ntohl(*(uint32_t*)elem.data);
                }
                break;
                
            case CAPWAP_ELEM_AC_NAME:
                msg->ac_name = (char*)elem.data;
                break;
                
            // Add other message elements as needed
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Configuration Status Request message
int capwap_build_config_status_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_config_status_req_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = CAPWAP_CONFIG_STATUS_REQUEST; // Set message type for Config Status Request
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    // Add configuration updates element if present
    if (msg->configuration_updates && msg->config_updates_len > 0) {
        capwap_msg_element_t elem_config = {
            .type = capwap_htons(CAPWAP_ELEM_AC_DESCRIPTOR), // Using AC Descriptor as placeholder
            .length = msg->config_updates_len,
            .data = msg->configuration_updates
        };
        
        uint8_t config_buffer[65536]; // Max possible size for element
        size_t config_elem_len = 4 + msg->config_updates_len; // Type(2) + Length(2) + actual data
        if (config_elem_len > sizeof(config_buffer)) {
            return CAPWAP_ERROR_BUFFER_OVERFLOW;
        }
        
        int result = capwap_build_message_element(config_buffer, &config_elem_len, &elem_config);
        if (result != CAPWAP_SUCCESS) {
            return result;
        }
        
        if (current_pos + config_elem_len > capacity) {
            return CAPWAP_ERROR_BUFFER_OVERFLOW;
        }
        
        memcpy(buffer + current_pos, config_buffer, config_elem_len);
        current_pos += config_elem_len;
    }

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Configuration Status Request message
int capwap_parse_config_status_request(uint8_t *buffer, size_t len, capwap_config_status_req_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            case CAPWAP_ELEM_AC_DESCRIPTOR: // Placeholder for configuration updates
                msg->configuration_updates = elem.data;
                msg->config_updates_len = elem.length;
                break;
                
            // Add other message elements as needed
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Configuration Status Response message
int capwap_build_config_status_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_config_status_resp_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = CAPWAP_CONFIG_STATUS_RESPONSE; // Set message type for Config Status Response
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    // Add Result Code message element
    uint32_t network_result_code = capwap_htonl(msg->result_code);
    capwap_msg_element_t elem_result_code = {
        .type = capwap_htons(CAPWAP_ELEM_RESULT_CODE),
        .length = 4,
        .data = (uint8_t*)&network_result_code
    };
    
    uint8_t temp_buffer[8]; // Type(2) + Length(2) + Data(4)
    size_t elem_len = 8;
    int result = capwap_build_message_element(temp_buffer, &elem_len, &elem_result_code);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    // Update the temp buffer to use the actual network byte order value
    *(uint32_t*)(temp_buffer + 4) = capwap_htonl(msg->result_code);
    elem_len = 8;
    
    if (current_pos + elem_len > capacity) {
        return CAPWAP_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(buffer + current_pos, temp_buffer, elem_len);
    current_pos += elem_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Configuration Status Response message
int capwap_parse_config_status_response(uint8_t *buffer, size_t len, capwap_config_status_resp_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            case CAPWAP_ELEM_RESULT_CODE:
                if (elem.length == 4) {
                    msg->result_code = capwap_ntohl(*(uint32_t*)elem.data);
                }
                break;
                
            // Add other message elements as needed
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Configuration Update Request message
int capwap_build_config_update_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_config_update_req_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = 0;
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Configuration Update Request message
int capwap_parse_config_update_request(uint8_t *buffer, size_t len, capwap_config_update_req_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            // Add processing for configuration update elements as needed
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Configuration Update Response message
int capwap_build_config_update_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_config_update_resp_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = 0;
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    // Add Result Code message element
    uint32_t network_result_code = capwap_htonl(msg->result_code);
    capwap_msg_element_t elem_result_code = {
        .type = capwap_htons(CAPWAP_ELEM_RESULT_CODE),
        .length = 4,
        .data = (uint8_t*)&network_result_code
    };
    
    uint8_t temp_buffer[8]; // Type(2) + Length(2) + Data(4)
    size_t elem_len = 8;
    int result = capwap_build_message_element(temp_buffer, &elem_len, &elem_result_code);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    // Update the temp buffer to use the actual network byte order value
    *(uint32_t*)(temp_buffer + 4) = capwap_htonl(msg->result_code);
    elem_len = 8;
    
    if (current_pos + elem_len > capacity) {
        return CAPWAP_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(buffer + current_pos, temp_buffer, elem_len);
    current_pos += elem_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Configuration Update Response message
int capwap_parse_config_update_response(uint8_t *buffer, size_t len, capwap_config_update_resp_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            case CAPWAP_ELEM_RESULT_CODE:
                if (elem.length == 4) {
                    msg->result_code = capwap_ntohl(*(uint32_t*)elem.data);
                }
                break;
                
            // Add other message elements as needed
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Change State Event Request message
int capwap_build_change_state_event_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_change_state_event_req_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = CAPWAP_CHANGE_STATE_EVENT_REQUEST; // Set message type for Change State Event Request
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Change State Event Request message
int capwap_parse_change_state_event_request(uint8_t *buffer, size_t len, capwap_change_state_event_req_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            // Process event-related elements
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Change State Event Response message
int capwap_build_change_state_event_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_change_state_event_resp_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = CAPWAP_CHANGE_STATE_EVENT_RESPONSE; // Set message type for Change State Event Response
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    // Add Result Code message element
    uint32_t network_result_code = capwap_htonl(msg->result_code);
    capwap_msg_element_t elem_result_code = {
        .type = capwap_htons(CAPWAP_ELEM_RESULT_CODE),
        .length = 4,
        .data = (uint8_t*)&network_result_code
    };
    
    uint8_t temp_buffer[8]; // Type(2) + Length(2) + Data(4)
    size_t elem_len = 8;
    int result = capwap_build_message_element(temp_buffer, &elem_len, &elem_result_code);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    // Update the temp buffer to use the actual network byte order value
    *(uint32_t*)(temp_buffer + 4) = capwap_htonl(msg->result_code);
    elem_len = 8;
    
    if (current_pos + elem_len > capacity) {
        return CAPWAP_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(buffer + current_pos, temp_buffer, elem_len);
    current_pos += elem_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Change State Event Response message
int capwap_parse_change_state_event_response(uint8_t *buffer, size_t len, capwap_change_state_event_resp_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            case CAPWAP_ELEM_RESULT_CODE:
                if (elem.length == 4) {
                    msg->result_code = capwap_ntohl(*(uint32_t*)elem.data);
                }
                break;
                
            // Add other message elements as needed
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Primary Discovery Request message
int capwap_build_primary_discovery_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_primary_discovery_req_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = 0;
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    // Add Discovery Type message element
    uint32_t network_discovery_type = capwap_htonl(msg->discovery_type);
    capwap_msg_element_t elem_discovery_type = {
        .type = capwap_htons(CAPWAP_ELEM_DISCOVERY_TYPE),
        .length = 4,
        .data = (uint8_t*)&network_discovery_type
    };
    
    uint8_t temp_buffer[8]; // Type(2) + Length(2) + Data(4)
    size_t elem_len = 8;
    int result = capwap_build_message_element(temp_buffer, &elem_len, &elem_discovery_type);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    // Update the temp buffer to use the actual network byte order value
    *(uint32_t*)(temp_buffer + 4) = capwap_htonl(msg->discovery_type);
    elem_len = 8;
    
    if (current_pos + elem_len > capacity) {
        return CAPWAP_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(buffer + current_pos, temp_buffer, elem_len);
    current_pos += elem_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Primary Discovery Request message
int capwap_parse_primary_discovery_request(uint8_t *buffer, size_t len, capwap_primary_discovery_req_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            case CAPWAP_ELEM_DISCOVERY_TYPE:
                if (elem.length == 4) {
                    msg->discovery_type = capwap_ntohl(*(uint32_t*)elem.data);
                }
                break;
                
            // Add other message elements as needed
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Primary Discovery Response message
int capwap_build_primary_discovery_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_primary_discovery_resp_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = 0;
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    // Add Result Code message element
    uint32_t network_result_code = capwap_htonl(msg->result_code);
    capwap_msg_element_t elem_result_code = {
        .type = capwap_htons(CAPWAP_ELEM_RESULT_CODE),
        .length = 4,
        .data = (uint8_t*)&network_result_code
    };
    
    uint8_t temp_buffer[8]; // Type(2) + Length(2) + Data(4)
    size_t elem_len = 8;
    int result = capwap_build_message_element(temp_buffer, &elem_len, &elem_result_code);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    // Update the temp buffer to use the actual network byte order value
    *(uint32_t*)(temp_buffer + 4) = capwap_htonl(msg->result_code);
    elem_len = 8;
    
    if (current_pos + elem_len > capacity) {
        return CAPWAP_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(buffer + current_pos, temp_buffer, elem_len);
    current_pos += elem_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Primary Discovery Response message
int capwap_parse_primary_discovery_response(uint8_t *buffer, size_t len, capwap_primary_discovery_resp_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            case CAPWAP_ELEM_RESULT_CODE:
                if (elem.length == 4) {
                    msg->result_code = capwap_ntohl(*(uint32_t*)elem.data);
                }
                break;
                
            // Add other message elements as needed
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Data Transfer Request message
int capwap_build_data_transfer_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_data_transfer_req_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = 0;
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Data Transfer Request message
int capwap_parse_data_transfer_request(uint8_t *buffer, size_t len, capwap_data_transfer_req_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            // Process data transfer elements
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Data Transfer Response message
int capwap_build_data_transfer_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_data_transfer_resp_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = 0;
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    // Add Result Code message element
    uint32_t network_result_code = capwap_htonl(msg->result_code);
    capwap_msg_element_t elem_result_code = {
        .type = capwap_htons(CAPWAP_ELEM_RESULT_CODE),
        .length = 4,
        .data = (uint8_t*)&network_result_code
    };
    
    uint8_t temp_buffer[8]; // Type(2) + Length(2) + Data(4)
    size_t elem_len = 8;
    int result = capwap_build_message_element(temp_buffer, &elem_len, &elem_result_code);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    // Update the temp buffer to use the actual network byte order value
    *(uint32_t*)(temp_buffer + 4) = capwap_htonl(msg->result_code);
    elem_len = 8;
    
    if (current_pos + elem_len > capacity) {
        return CAPWAP_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(buffer + current_pos, temp_buffer, elem_len);
    current_pos += elem_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Data Transfer Response message
int capwap_parse_data_transfer_response(uint8_t *buffer, size_t len, capwap_data_transfer_resp_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            case CAPWAP_ELEM_RESULT_CODE:
                if (elem.length == 4) {
                    msg->result_code = capwap_ntohl(*(uint32_t*)elem.data);
                }
                break;
                
            // Add other message elements as needed
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Clear Configuration Request message
int capwap_build_clear_config_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_clear_config_req_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = 0;
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Clear Configuration Request message
int capwap_parse_clear_config_request(uint8_t *buffer, size_t len, capwap_clear_config_req_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            // No specific required elements for this message
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Clear Configuration Response message
int capwap_build_clear_config_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_clear_config_resp_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = 0;
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    // Add Result Code message element
    uint32_t network_result_code = capwap_htonl(msg->result_code);
    capwap_msg_element_t elem_result_code = {
        .type = capwap_htons(CAPWAP_ELEM_RESULT_CODE),
        .length = 4,
        .data = (uint8_t*)&network_result_code
    };
    
    uint8_t temp_buffer[8]; // Type(2) + Length(2) + Data(4)
    size_t elem_len = 8;
    int result = capwap_build_message_element(temp_buffer, &elem_len, &elem_result_code);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    // Update the temp buffer to use the actual network byte order value
    *(uint32_t*)(temp_buffer + 4) = capwap_htonl(msg->result_code);
    elem_len = 8;
    
    if (current_pos + elem_len > capacity) {
        return CAPWAP_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(buffer + current_pos, temp_buffer, elem_len);
    current_pos += elem_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Clear Configuration Response message
int capwap_parse_clear_config_response(uint8_t *buffer, size_t len, capwap_clear_config_resp_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            case CAPWAP_ELEM_RESULT_CODE:
                if (elem.length == 4) {
                    msg->result_code = capwap_ntohl(*(uint32_t*)elem.data);
                }
                break;
                
            // Add other message elements as needed
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Station Configuration Request message
int capwap_build_station_config_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_station_config_req_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = 0;
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Station Configuration Request message
int capwap_parse_station_config_request(uint8_t *buffer, size_t len, capwap_station_config_req_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            // Process station configuration elements
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Station Configuration Response message
int capwap_build_station_config_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_station_config_resp_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = 0;
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    // Add Result Code message element
    uint32_t network_result_code = capwap_htonl(msg->result_code);
    capwap_msg_element_t elem_result_code = {
        .type = capwap_htons(CAPWAP_ELEM_RESULT_CODE),
        .length = 4,
        .data = (uint8_t*)&network_result_code
    };
    
    uint8_t temp_buffer[8]; // Type(2) + Length(2) + Data(4)
    size_t elem_len = 8;
    int result = capwap_build_message_element(temp_buffer, &elem_len, &elem_result_code);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    // Update the temp buffer to use the actual network byte order value
    *(uint32_t*)(temp_buffer + 4) = capwap_htonl(msg->result_code);
    elem_len = 8;
    
    if (current_pos + elem_len > capacity) {
        return CAPWAP_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(buffer + current_pos, temp_buffer, elem_len);
    current_pos += elem_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Station Configuration Response message
int capwap_parse_station_config_response(uint8_t *buffer, size_t len, capwap_station_config_resp_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            case CAPWAP_ELEM_RESULT_CODE:
                if (elem.length == 4) {
                    msg->result_code = capwap_ntohl(*(uint32_t*)elem.data);
                }
                break;
                
            // Add other message elements as needed
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build WTP Event Request message
int capwap_build_wtp_event_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_wtp_event_req_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = CAPWAP_WTP_EVENT_REQUEST; // Set message type for WTP Event Request
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse WTP Event Request message
int capwap_parse_wtp_event_request(uint8_t *buffer, size_t len, capwap_wtp_event_req_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            // Process WTP event elements
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build WTP Event Response message
int capwap_build_wtp_event_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_wtp_event_resp_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = CAPWAP_WTP_EVENT_RESPONSE; // Set message type for WTP Event Response
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    // Add Result Code message element
    uint32_t network_result_code = capwap_htonl(msg->result_code);
    capwap_msg_element_t elem_result_code = {
        .type = capwap_htons(CAPWAP_ELEM_RESULT_CODE),
        .length = 4,
        .data = (uint8_t*)&network_result_code
    };
    
    uint8_t temp_buffer[8]; // Type(2) + Length(2) + Data(4)
    size_t elem_len = 8;
    int result = capwap_build_message_element(temp_buffer, &elem_len, &elem_result_code);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    // Update the temp buffer to use the actual network byte order value
    *(uint32_t*)(temp_buffer + 4) = capwap_htonl(msg->result_code);
    elem_len = 8;
    
    if (current_pos + elem_len > capacity) {
        return CAPWAP_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(buffer + current_pos, temp_buffer, elem_len);
    current_pos += elem_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse WTP Event Response message
int capwap_parse_wtp_event_response(uint8_t *buffer, size_t len, capwap_wtp_event_resp_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            case CAPWAP_ELEM_RESULT_CODE:
                if (elem.length == 4) {
                    msg->result_code = capwap_ntohl(*(uint32_t*)elem.data);
                }
                break;
                
            // Add other message elements as needed
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Reset Request message
int capwap_build_reset_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_reset_req_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = 0;
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Reset Request message
int capwap_parse_reset_request(uint8_t *buffer, size_t len, capwap_reset_req_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            // Process reset request elements
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Reset Response message
int capwap_build_reset_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_reset_resp_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = 0;
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    // Add Result Code message element
    uint32_t network_result_code = capwap_htonl(msg->result_code);
    capwap_msg_element_t elem_result_code = {
        .type = capwap_htons(CAPWAP_ELEM_RESULT_CODE),
        .length = 4,
        .data = (uint8_t*)&network_result_code
    };
    
    uint8_t temp_buffer[8]; // Type(2) + Length(2) + Data(4)
    size_t elem_len = 8;
    int result = capwap_build_message_element(temp_buffer, &elem_len, &elem_result_code);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    // Update the temp buffer to use the actual network byte order value
    *(uint32_t*)(temp_buffer + 4) = capwap_htonl(msg->result_code);
    elem_len = 8;
    
    if (current_pos + elem_len > capacity) {
        return CAPWAP_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(buffer + current_pos, temp_buffer, elem_len);
    current_pos += elem_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Reset Response message
int capwap_parse_reset_response(uint8_t *buffer, size_t len, capwap_reset_resp_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            case CAPWAP_ELEM_RESULT_CODE:
                if (elem.length == 4) {
                    msg->result_code = capwap_ntohl(*(uint32_t*)elem.data);
                }
                break;
                
            // Add other message elements as needed
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Image Data Request message
int capwap_build_image_data_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_image_data_req_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = 0;
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Image Data Request message
int capwap_parse_image_data_request(uint8_t *buffer, size_t len, capwap_image_data_req_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            // Process image data request elements
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}

// Build Image Data Response message
int capwap_build_image_data_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_image_data_resp_t *msg) {
    if (!buffer || !len || !msg || capacity < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    size_t current_pos = 0;

    // Create and build the header
    capwap_header_t header = {0};
    header.version = CAPWAP_VERSION_1;
    header.type = 1; // Control message
    header.wireless_binding = CAPWAP_WIRELESS_BINDING_IEEE_80211;
    header.header_length = 2; // 2 words = 8 bytes
    header.flags = 0;
    header.fragment_id = 0;
    header.fragment_offset = 0;
    header.final_fragment = 0;
    header.reserved2 = 0;
    header.radio_id = 0;
    header.wtp_ac_id = 0;
    header.session_id = 0;

    size_t header_len = CAPWAP_HEADER_SIZE;
    int header_result = capwap_build_header(buffer, &header_len, &header);
    if (header_result != CAPWAP_SUCCESS) {
        return header_result;
    }
    
    current_pos += header_len;

    // Add Result Code message element
    uint32_t network_result_code = capwap_htonl(msg->result_code);
    capwap_msg_element_t elem_result_code = {
        .type = capwap_htons(CAPWAP_ELEM_RESULT_CODE),
        .length = 4,
        .data = (uint8_t*)&network_result_code
    };
    
    uint8_t temp_buffer[8]; // Type(2) + Length(2) + Data(4)
    size_t elem_len = 8;
    int result = capwap_build_message_element(temp_buffer, &elem_len, &elem_result_code);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }
    
    // Update the temp buffer to use the actual network byte order value
    *(uint32_t*)(temp_buffer + 4) = capwap_htonl(msg->result_code);
    elem_len = 8;
    
    if (current_pos + elem_len > capacity) {
        return CAPWAP_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(buffer + current_pos, temp_buffer, elem_len);
    current_pos += elem_len;

    *len = current_pos;
    return CAPWAP_SUCCESS;
}

// Parse Image Data Response message
int capwap_parse_image_data_response(uint8_t *buffer, size_t len, capwap_image_data_resp_t *msg) {
    if (!buffer || !msg || len < CAPWAP_HEADER_SIZE) {
        return CAPWAP_ERROR_INVALID_INPUT;
    }

    // Parse header first
    capwap_header_t header;
    int result = capwap_parse_header(buffer, len, &header);
    if (result != CAPWAP_SUCCESS) {
        return result;
    }

    size_t pos = CAPWAP_HEADER_SIZE;

    // Parse message elements until the end of the buffer
    while (pos < len) {
        capwap_msg_element_t elem;
        size_t bytes_consumed;
        
        result = capwap_parse_message_element(buffer + pos, len - pos, &elem, &bytes_consumed);
        if (result != CAPWAP_SUCCESS) {
            break;
        }

        // Process based on element type
        switch (elem.type) {
            case CAPWAP_ELEM_RESULT_CODE:
                if (elem.length == 4) {
                    msg->result_code = capwap_ntohl(*(uint32_t*)elem.data);
                }
                break;
                
            // Add other message elements as needed
            default:
                break;
        }
        
        pos += bytes_consumed;
    }

    return CAPWAP_SUCCESS;
}