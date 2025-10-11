#include "cw_msg.h"
#include "cw_log.h"
#include <string.h>
#include <arpa/inet.h>

/* Helper macros for buffer bounds checking */
#define CHECK_BUFFER_SIZE(needed, capacity) \
    do { \
        if ((needed) > (capacity)) { \
            CW_LOG_ERROR("Buffer too small: needed=%zu, capacity=%zu", (size_t)(needed), (size_t)(capacity)); \
            return CW_MSG_ERROR_BUFFER_TOO_SMALL; \
        } \
    } while(0)

#define CHECK_LENGTH(needed, available) \
    do { \
        if ((needed) > (available)) { \
            CW_LOG_ERROR("Insufficient data length: needed=%zu, available=%zu", (size_t)(needed), (size_t)(available)); \
            return CW_MSG_ERROR_PARSE_FAILED; \
        } \
    } while(0)

/* ============= Utility Functions ============= */

const char* cw_msg_type_name(uint32_t msg_type) {
    switch (msg_type) {
        case CW_MSG_TYPE_DISCOVERY_REQUEST: return "Discovery Request";
        case CW_MSG_TYPE_DISCOVERY_RESPONSE: return "Discovery Response";
        case CW_MSG_TYPE_JOIN_REQUEST: return "Join Request";
        case CW_MSG_TYPE_JOIN_RESPONSE: return "Join Response";
        case CW_MSG_TYPE_CONFIGURATION_STATUS_REQUEST: return "Configuration Status Request";
        case CW_MSG_TYPE_CONFIGURATION_STATUS_RESPONSE: return "Configuration Status Response";
        case CW_MSG_TYPE_CONFIGURATION_UPDATE_REQUEST: return "Configuration Update Request";
        case CW_MSG_TYPE_CONFIGURATION_UPDATE_RESPONSE: return "Configuration Update Response";
        case CW_MSG_TYPE_WTP_EVENT_REQUEST: return "WTP Event Request";
        case CW_MSG_TYPE_WTP_EVENT_RESPONSE: return "WTP Event Response";
        case CW_MSG_TYPE_CHANGE_STATE_EVENT_REQUEST: return "Change State Event Request";
        case CW_MSG_TYPE_CHANGE_STATE_EVENT_RESPONSE: return "Change State Event Response";
        case CW_MSG_TYPE_ECHO_REQUEST: return "Echo Request";
        case CW_MSG_TYPE_ECHO_RESPONSE: return "Echo Response";
        case CW_MSG_TYPE_IMAGE_DATA_REQUEST: return "Image Data Request";
        case CW_MSG_TYPE_IMAGE_DATA_RESPONSE: return "Image Data Response";
        case CW_MSG_TYPE_RESET_REQUEST: return "Reset Request";
        case CW_MSG_TYPE_RESET_RESPONSE: return "Reset Response";
        default: return "Unknown";
    }
}

const char* cw_elem_type_name(uint16_t elem_type) {
    switch (elem_type) {
        case CW_ELEM_AC_DESCRIPTOR: return "AC Descriptor";
        case CW_ELEM_AC_IPV4_LIST: return "AC IPv4 List";
        case CW_ELEM_AC_IPV6_LIST: return "AC IPv6 List";
        case CW_ELEM_AC_NAME: return "AC Name";
        case CW_ELEM_SESSION_ID: return "Session ID";
        case CW_ELEM_RESULT_CODE: return "Result Code";
        case CW_ELEM_WTP_DESCRIPTOR: return "WTP Descriptor";
        case CW_ELEM_WTP_NAME: return "WTP Name";
        case CW_ELEM_WTP_BOARD_DATA: return "WTP Board Data";
        case CW_ELEM_RADIO_ADMINISTRATIVE_STATE: return "Radio Administrative State";
        case CW_ELEM_DISCOVERY_TYPE: return "Discovery Type";
        case CW_ELEM_LOCATION_DATA: return "Location Data";
        default: return "Unknown";
    }
}

const char* cw_msg_error_string(int error_code) {
    switch (error_code) {
        case CW_MSG_SUCCESS: return "Success";
        case CW_MSG_ERROR_INVALID_PARAM: return "Invalid parameter";
        case CW_MSG_ERROR_BUFFER_TOO_SMALL: return "Buffer too small";
        case CW_MSG_ERROR_PARSE_FAILED: return "Parse failed";
        case CW_MSG_ERROR_UNSUPPORTED_TYPE: return "Unsupported type";
        case CW_MSG_ERROR_INVALID_FORMAT: return "Invalid format";
        default: return "Unknown error";
    }
}

void cw_init_control_msg(cw_control_msg_t *msg, uint32_t msg_type, uint8_t seq_num) {
    if (msg == NULL) return;

    memset(msg, 0, sizeof(cw_control_msg_t));

    /* Initialize CAPWAP header */
    msg->header.preamble = CW_PREAMBLE_VERSION_0;
    msg->header.hlen = 2; /* Minimum header length (8 bytes = 2 * 4) */
    msg->header.flags = CW_FLAG_HDR_T; /* Control message */

    /* Initialize control header */
    msg->ctrl_header.message_type = msg_type;
    msg->ctrl_header.seq_num = seq_num;
    msg->ctrl_header.msg_elem_len = 0;
    msg->ctrl_header.flags = 0;

    msg->element_count = 0;
}

/* ============= Header Functions ============= */

int cw_generate_header(const cw_header_t *header, uint8_t *buffer, size_t capacity) {
    if (header == NULL || buffer == NULL) {
        CW_LOG_ERROR("Invalid parameter: header=%p, buffer=%p", (void*)header, (void*)buffer);
        return CW_MSG_ERROR_INVALID_PARAM;
    }

    size_t header_len = 8; /* Minimum CAPWAP header size */
    CHECK_BUFFER_SIZE(header_len, capacity);

    /* Byte 0: Preamble (version and type) */
    buffer[0] = header->preamble;

    /* Byte 1: HLEN (5 bits) + WBID (3 bits) */
    buffer[1] = ((header->hlen & 0x1F) << 3) | (header->wbid & 0x07);

    /* Byte 2: RID */
    buffer[2] = header->rid;

    /* Byte 3: WBID */
    buffer[3] = header->wbid;

    /* Byte 4: Flags */
    buffer[4] = header->flags;

    /* Bytes 5-6: Fragment ID */
    uint16_t frag_id = htons(header->fragment_id);
    memcpy(&buffer[5], &frag_id, 2);

    /* Bytes 7: Fragment Offset (13 bits) + Reserved (3 bits) */
    buffer[7] = (header->fragment_offset >> 8) & 0x1F;

    return (int)header_len;
}

int cw_parse_header(const uint8_t *buffer, size_t length, cw_header_t *header) {
    if (buffer == NULL || header == NULL) {
        CW_LOG_ERROR("Invalid parameter: buffer=%p, header=%p", (void*)buffer, (void*)header);
        return CW_MSG_ERROR_INVALID_PARAM;
    }

    CHECK_LENGTH(8, length); /* Minimum header size */

    /* Byte 0: Preamble */
    header->preamble = buffer[0];

    /* Byte 1: HLEN + WBID */
    header->hlen = (buffer[1] >> 3) & 0x1F;
    header->wbid = buffer[1] & 0x07;

    /* Byte 2: RID */
    header->rid = buffer[2];

    /* Byte 3: WBID (full) */
    header->wbid = buffer[3];

    /* Byte 4: Flags */
    header->flags = buffer[4];

    /* Bytes 5-6: Fragment ID */
    uint16_t frag_id;
    memcpy(&frag_id, &buffer[5], 2);
    header->fragment_id = ntohs(frag_id);

    /* Byte 7: Fragment Offset */
    header->fragment_offset = (buffer[7] & 0x1F) << 8;

    return 8;
}

/* ============= Control Message Functions ============= */

int cw_generate_control_msg(const cw_control_msg_t *msg, uint8_t *buffer, size_t capacity) {
    if (msg == NULL || buffer == NULL) {
        CW_LOG_ERROR("Invalid parameter: msg=%p, buffer=%p", (void*)msg, (void*)buffer);
        return CW_MSG_ERROR_INVALID_PARAM;
    }

    size_t offset = 0;

    /* Generate CAPWAP header */
    int header_len = cw_generate_header(&msg->header, buffer, capacity);
    if (header_len < 0) {
        CW_LOG_ERROR("Failed to generate CAPWAP header: %s (%d)",
                     cw_msg_error_string(header_len), header_len);
        return header_len;
    }
    offset += header_len;

    /* Generate Control Header */
    CHECK_BUFFER_SIZE(offset + 8, capacity); /* Control header is 8 bytes */

    /* Message Type (4 bytes) */
    uint32_t msg_type = htonl(msg->ctrl_header.message_type);
    memcpy(&buffer[offset], &msg_type, 4);
    offset += 4;

    /* Sequence Number (1 byte) */
    buffer[offset++] = msg->ctrl_header.seq_num;

    /* Message Element Length (2 bytes) - will be calculated */
    uint16_t elem_len = 0;
    size_t elem_len_offset = offset;
    offset += 2;

    /* Flags (1 byte) */
    buffer[offset++] = msg->ctrl_header.flags;

    /* Generate Message Elements */
    for (int i = 0; i < msg->element_count; i++) {
        const cw_msg_element_t *elem = &msg->elements[i];
        size_t elem_total_len = 4 + elem->length; /* Type(2) + Length(2) + Value */

        CHECK_BUFFER_SIZE(offset + elem_total_len, capacity);

        /* Element Type (2 bytes) */
        uint16_t type = htons(elem->type);
        memcpy(&buffer[offset], &type, 2);
        offset += 2;

        /* Element Length (2 bytes) */
        uint16_t len = htons(elem->length);
        memcpy(&buffer[offset], &len, 2);
        offset += 2;

        /* Element Value */
        if (elem->length > 0 && elem->value != NULL) {
            memcpy(&buffer[offset], elem->value, elem->length);
            offset += elem->length;
        }

        elem_len += elem_total_len;
    }

    /* Update Message Element Length in control header */
    uint16_t elem_len_net = htons(elem_len);
    memcpy(&buffer[elem_len_offset], &elem_len_net, 2);

    return (int)offset;
}

int cw_parse_control_msg(const uint8_t *buffer, size_t length, cw_control_msg_t *msg) {
    if (buffer == NULL || msg == NULL) {
        CW_LOG_ERROR("Invalid parameter: buffer=%p, msg=%p", (void*)buffer, (void*)msg);
        return CW_MSG_ERROR_INVALID_PARAM;
    }

    memset(msg, 0, sizeof(cw_control_msg_t));

    /* Parse CAPWAP header */
    int header_len = cw_parse_header(buffer, length, &msg->header);
    if (header_len < 0) {
        CW_LOG_ERROR("Failed to parse CAPWAP header: %s (%d)",
                     cw_msg_error_string(header_len), header_len);
        return header_len;
    }

    size_t offset = header_len;

    /* Parse Control Header */
    CHECK_LENGTH(offset + 8, length);

    /* Message Type (4 bytes) */
    uint32_t msg_type;
    memcpy(&msg_type, &buffer[offset], 4);
    msg->ctrl_header.message_type = ntohl(msg_type);
    offset += 4;

    /* Sequence Number (1 byte) */
    msg->ctrl_header.seq_num = buffer[offset++];

    /* Message Element Length (2 bytes) */
    uint16_t elem_len;
    memcpy(&elem_len, &buffer[offset], 2);
    msg->ctrl_header.msg_elem_len = ntohs(elem_len);
    offset += 2;

    /* Flags (1 byte) */
    msg->ctrl_header.flags = buffer[offset++];

    /* Parse Message Elements */
    //offset point Flags, while 
    //Message Element Length: indicates the number of bytes following the Sequence Number field.
    size_t elements_end = offset - 3 + msg->ctrl_header.msg_elem_len;
    if (elements_end > length) {
        CW_LOG_ERROR("Message element length exceeds buffer: elements_end=%zu, length=%zu; offset=%zu,msg_elem_len=%zu",
                     elements_end, length, offset, msg->ctrl_header.msg_elem_len);
        return CW_MSG_ERROR_PARSE_FAILED;
    }

    msg->element_count = 0;
    while (offset < elements_end && msg->element_count < CW_MAX_ELEMENTS) {
        CHECK_LENGTH(offset + 4, length); /* Type + Length */

        cw_msg_element_t *elem = &msg->elements[msg->element_count];

        /* Element Type (2 bytes) */
        uint16_t type;
        memcpy(&type, &buffer[offset], 2);
        elem->type = ntohs(type);
        offset += 2;

        /* Element Length (2 bytes) */
        uint16_t len;
        memcpy(&len, &buffer[offset], 2);
        elem->length = ntohs(len);
        offset += 2;

        /* Element Value (pointer to buffer) */
        if (elem->length > 0) {
            CHECK_LENGTH(offset + elem->length, length);
            elem->value = &buffer[offset];
            offset += elem->length;
        } else {
            elem->value = NULL;
        }

        msg->element_count++;
    }

    return (int)offset;
}

/* ============= Message Element Functions ============= */

int cw_add_element(cw_control_msg_t *msg, uint16_t type, const uint8_t *value, uint16_t length) {
    if (msg == NULL) {
        CW_LOG_ERROR("Invalid parameter: msg=%p", (void*)msg);
        return CW_MSG_ERROR_INVALID_PARAM;
    }

    if (msg->element_count >= CW_MAX_ELEMENTS) {
        CW_LOG_ERROR("Maximum elements reached: element_count=%d, max=%d",
                     msg->element_count, CW_MAX_ELEMENTS);
        return CW_MSG_ERROR_BUFFER_TOO_SMALL;
    }

    cw_msg_element_t *elem = &msg->elements[msg->element_count];
    elem->type = type;
    elem->length = length;
    elem->value = value;

    msg->element_count++;
    return CW_MSG_SUCCESS;
}

const cw_msg_element_t* cw_find_element(const cw_control_msg_t *msg, uint16_t type) {
    if (msg == NULL) {
        return NULL;
    }

    for (int i = 0; i < msg->element_count; i++) {
        if (msg->elements[i].type == type) {
            return &msg->elements[i];
        }
    }

    return NULL;
}

/* ============= Specific Element Parsers ============= */

int cw_parse_ac_descriptor(const cw_msg_element_t *elem, cw_ac_descriptor_t *desc) {
    if (elem == NULL || desc == NULL || elem->value == NULL) {
        CW_LOG_ERROR("Invalid parameter: elem=%p, desc=%p, value=%p",
                     (void*)elem, (void*)desc, elem ? (void*)elem->value : NULL);
        return CW_MSG_ERROR_INVALID_PARAM;
    }

    if (elem->length < 12) {
        CW_LOG_ERROR("AC Descriptor too short: length=%u, expected>=12", elem->length);
        return CW_MSG_ERROR_PARSE_FAILED;
    }

    const uint8_t *data = elem->value;

    /* Stations (2 bytes) */
    uint16_t stations;
    memcpy(&stations, &data[0], 2);
    desc->stations = ntohs(stations);

    /* Limit (2 bytes) */
    uint16_t limit;
    memcpy(&limit, &data[2], 2);
    desc->limit = ntohs(limit);

    /* Active WTPs (2 bytes) */
    uint16_t active_wtps;
    memcpy(&active_wtps, &data[4], 2);
    desc->active_wtps = ntohs(active_wtps);

    /* Max WTPs (2 bytes) */
    uint16_t max_wtps;
    memcpy(&max_wtps, &data[6], 2);
    desc->max_wtps = ntohs(max_wtps);

    /* Security (1 byte) */
    desc->security = data[8];

    /* R-MAC Field (1 byte) */
    desc->rmac_field = data[9];

    /* Reserved (1 byte) */
    desc->reserved = data[10];

    /* DTLS Policy (1 byte) */
    desc->dtls_policy = data[11];

    return CW_MSG_SUCCESS;
}

int cw_parse_result_code(const cw_msg_element_t *elem, cw_result_code_t *result) {
    if (elem == NULL || result == NULL || elem->value == NULL) {
        CW_LOG_ERROR("Invalid parameter: elem=%p, result=%p, value=%p",
                     (void*)elem, (void*)result, elem ? (void*)elem->value : NULL);
        return CW_MSG_ERROR_INVALID_PARAM;
    }

    if (elem->length < 4) {
        CW_LOG_ERROR("Result Code element too short: length=%u, expected=4", elem->length);
        return CW_MSG_ERROR_PARSE_FAILED;
    }

    uint32_t code;
    memcpy(&code, elem->value, 4);
    result->result_code = ntohl(code);

    return CW_MSG_SUCCESS;
}

int cw_parse_session_id(const cw_msg_element_t *elem, cw_session_id_t *session_id) {
    if (elem == NULL || session_id == NULL || elem->value == NULL) {
        CW_LOG_ERROR("Invalid parameter: elem=%p, session_id=%p, value=%p",
                     (void*)elem, (void*)session_id, elem ? (void*)elem->value : NULL);
        return CW_MSG_ERROR_INVALID_PARAM;
    }

    if (elem->length > CW_MAX_SESSION_ID_LEN) {
        CW_LOG_ERROR("Session ID too long: length=%u, max=%d", elem->length, CW_MAX_SESSION_ID_LEN);
        return CW_MSG_ERROR_PARSE_FAILED;
    }

    session_id->length = elem->length;
    memcpy(session_id->session_id, elem->value, elem->length);

    return CW_MSG_SUCCESS;
}

int cw_parse_ac_name(const cw_msg_element_t *elem, cw_ac_name_t *name) {
    if (elem == NULL || name == NULL || elem->value == NULL) {
        CW_LOG_ERROR("Invalid parameter: elem=%p, name=%p, value=%p",
                     (void*)elem, (void*)name, elem ? (void*)elem->value : NULL);
        return CW_MSG_ERROR_INVALID_PARAM;
    }

    name->name = (const char *)elem->value;
    name->length = elem->length;

    return CW_MSG_SUCCESS;
}

int cw_parse_wtp_name(const cw_msg_element_t *elem, cw_wtp_name_t *name) {
    if (elem == NULL || name == NULL || elem->value == NULL) {
        CW_LOG_ERROR("Invalid parameter: elem=%p, name=%p, value=%p",
                     (void*)elem, (void*)name, elem ? (void*)elem->value : NULL);
        return CW_MSG_ERROR_INVALID_PARAM;
    }

    name->name = (const char *)elem->value;
    name->length = elem->length;

    return CW_MSG_SUCCESS;
}

/* ============= Specific Element Generators ============= */

int cw_generate_ac_descriptor(const cw_ac_descriptor_t *desc, uint8_t *buffer, size_t capacity) {
    if (desc == NULL || buffer == NULL) {
        CW_LOG_ERROR("Invalid parameter: desc=%p, buffer=%p", (void*)desc, (void*)buffer);
        return CW_MSG_ERROR_INVALID_PARAM;
    }

    size_t required = 16; /* Type(2) + Length(2) + Data(12) */
    CHECK_BUFFER_SIZE(required, capacity);

    size_t offset = 0;

    /* Element Type */
    uint16_t type = htons(CW_ELEM_AC_DESCRIPTOR);
    memcpy(&buffer[offset], &type, 2);
    offset += 2;

    /* Element Length */
    uint16_t length = htons(12);
    memcpy(&buffer[offset], &length, 2);
    offset += 2;

    /* Stations */
    uint16_t stations = htons(desc->stations);
    memcpy(&buffer[offset], &stations, 2);
    offset += 2;

    /* Limit */
    uint16_t limit = htons(desc->limit);
    memcpy(&buffer[offset], &limit, 2);
    offset += 2;

    /* Active WTPs */
    uint16_t active = htons(desc->active_wtps);
    memcpy(&buffer[offset], &active, 2);
    offset += 2;

    /* Max WTPs */
    uint16_t max = htons(desc->max_wtps);
    memcpy(&buffer[offset], &max, 2);
    offset += 2;

    /* Security, R-MAC, Reserved, DTLS Policy */
    buffer[offset++] = desc->security;
    buffer[offset++] = desc->rmac_field;
    buffer[offset++] = desc->reserved;
    buffer[offset++] = desc->dtls_policy;

    return (int)offset;
}

int cw_generate_result_code(uint32_t result_code, uint8_t *buffer, size_t capacity) {
    if (buffer == NULL) {
        CW_LOG_ERROR("Invalid parameter: buffer=%p", (void*)buffer);
        return CW_MSG_ERROR_INVALID_PARAM;
    }

    size_t required = 8; /* Type(2) + Length(2) + Data(4) */
    CHECK_BUFFER_SIZE(required, capacity);

    size_t offset = 0;

    /* Element Type */
    uint16_t type = htons(CW_ELEM_RESULT_CODE);
    memcpy(&buffer[offset], &type, 2);
    offset += 2;

    /* Element Length */
    uint16_t length = htons(4);
    memcpy(&buffer[offset], &length, 2);
    offset += 2;

    /* Result Code */
    uint32_t code = htonl(result_code);
    memcpy(&buffer[offset], &code, 4);
    offset += 4;

    return (int)offset;
}

int cw_generate_session_id(const cw_session_id_t *session_id, uint8_t *buffer, size_t capacity) {
    if (session_id == NULL || buffer == NULL) {
        CW_LOG_ERROR("Invalid parameter: session_id=%p, buffer=%p", (void*)session_id, (void*)buffer);
        return CW_MSG_ERROR_INVALID_PARAM;
    }

    size_t required = 4 + session_id->length; /* Type(2) + Length(2) + Data */
    CHECK_BUFFER_SIZE(required, capacity);

    size_t offset = 0;

    /* Element Type */
    uint16_t type = htons(CW_ELEM_SESSION_ID);
    memcpy(&buffer[offset], &type, 2);
    offset += 2;

    /* Element Length */
    uint16_t length = htons(session_id->length);
    memcpy(&buffer[offset], &length, 2);
    offset += 2;

    /* Session ID */
    memcpy(&buffer[offset], session_id->session_id, session_id->length);
    offset += session_id->length;

    return (int)offset;
}

int cw_generate_ac_name(const char *name, uint8_t *buffer, size_t capacity) {
    if (name == NULL || buffer == NULL) {
        CW_LOG_ERROR("Invalid parameter: name=%p, buffer=%p", (void*)name, (void*)buffer);
        return CW_MSG_ERROR_INVALID_PARAM;
    }

    uint16_t name_len = strlen(name);
    size_t required = 4 + name_len; /* Type(2) + Length(2) + Data */
    CHECK_BUFFER_SIZE(required, capacity);

    size_t offset = 0;

    /* Element Type */
    uint16_t type = htons(CW_ELEM_AC_NAME);
    memcpy(&buffer[offset], &type, 2);
    offset += 2;

    /* Element Length */
    uint16_t length = htons(name_len);
    memcpy(&buffer[offset], &length, 2);
    offset += 2;

    /* AC Name */
    memcpy(&buffer[offset], name, name_len);
    offset += name_len;

    return (int)offset;
}
