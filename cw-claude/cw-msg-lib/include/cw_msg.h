#ifndef CW_MSG_H
#define CW_MSG_H

#include "cw_msg_types.h"
#include <stdint.h>
#include <stddef.h>

/* Error Codes */
#define CW_MSG_SUCCESS               0
#define CW_MSG_ERROR_INVALID_PARAM   -1
#define CW_MSG_ERROR_BUFFER_TOO_SMALL -2
#define CW_MSG_ERROR_PARSE_FAILED    -3
#define CW_MSG_ERROR_UNSUPPORTED_TYPE -4
#define CW_MSG_ERROR_INVALID_FORMAT  -5

/* CAPWAP Header Structure (RFC 5415 Section 4.3) */
typedef struct {
    uint8_t preamble;        /* Version (4 bits) + Type (4 bits) */
    uint8_t hlen;            /* Header length in 4-byte words (5 bits) + WBID (3 bits) */
    uint8_t rid;             /* Radio ID */
    uint8_t wbid;            /* Wireless Binding ID */
    uint8_t flags;           /* T, F, L, W, M, K flags */
    uint16_t fragment_id;    /* Fragment ID */
    uint16_t fragment_offset; /* Fragment offset (13 bits) + Reserved (3 bits) */
    uint32_t reserved;       /* Reserved field */
} cw_header_t;

/* CAPWAP Control Header Structure */
typedef struct {
    uint32_t message_type;   /* Message type */
    uint8_t seq_num;         /* Sequence number */
    uint16_t msg_elem_len;   /* Total length of message elements */
    uint8_t flags;           /* Flags field */
} cw_control_header_t;

/* Message Element Header */
typedef struct {
    uint16_t type;           /* Element type */
    uint16_t length;         /* Element length */
    const uint8_t *value;    /* Pointer to element value (not owned) */
} cw_msg_element_t;

/* AC Descriptor Element */
typedef struct {
    uint16_t stations;       /* Number of stations */
    uint16_t limit;          /* Station limit */
    uint16_t active_wtps;    /* Active WTPs */
    uint16_t max_wtps;       /* Maximum WTPs */
    uint8_t security;        /* Security flags */
    uint8_t rmac_field;      /* R-MAC field */
    uint8_t reserved;        /* Reserved */
    uint8_t dtls_policy;     /* DTLS policy */
    /* Vendor-specific sub-elements follow */
} cw_ac_descriptor_t;

/* WTP Descriptor Element */
typedef struct {
    uint8_t max_radios;      /* Maximum radios */
    uint8_t radios_in_use;   /* Radios in use */
    /* Vendor-specific sub-elements follow */
} cw_wtp_descriptor_t;

/* WTP Board Data */
typedef struct {
    uint32_t vendor_id;
    const uint8_t *board_data;
    uint16_t board_data_len;
} cw_wtp_board_data_t;

/* Session ID Element */
typedef struct {
    uint8_t session_id[CW_MAX_SESSION_ID_LEN];
    uint8_t length;
} cw_session_id_t;

/* AC Name Element */
typedef struct {
    const char *name;
    uint16_t length;
} cw_ac_name_t;

/* Result Code Element */
typedef struct {
    uint32_t result_code;
} cw_result_code_t;

/* Radio Administrative State */
typedef struct {
    uint8_t radio_id;
    uint8_t admin_state;
} cw_radio_admin_state_t;

/* Discovery Type Element */
typedef struct {
    uint8_t discovery_type;
} cw_discovery_type_t;

/* Location Data Element */
typedef struct {
    const char *location;
    uint16_t length;
} cw_location_data_t;

/* WTP Name Element */
typedef struct {
    const char *name;
    uint16_t length;
} cw_wtp_name_t;

/* CAPWAP Control Message Structure */
typedef struct {
    cw_header_t header;
    cw_control_header_t ctrl_header;
    cw_msg_element_t elements[CW_MAX_ELEMENTS];
    int element_count;
} cw_control_msg_t;

/* ============= Header Functions ============= */

/**
 * Generate CAPWAP header
 * @param header Pointer to header structure
 * @param buffer Output buffer
 * @param capacity Buffer capacity
 * @return Number of bytes written, or negative error code
 */
int cw_generate_header(const cw_header_t *header, uint8_t *buffer, size_t capacity);

/**
 * Parse CAPWAP header
 * @param buffer Input buffer
 * @param length Buffer length
 * @param header Pointer to header structure to fill
 * @return Number of bytes parsed, or negative error code
 */
int cw_parse_header(const uint8_t *buffer, size_t length, cw_header_t *header);

/* ============= Control Message Functions ============= */

/**
 * Generate CAPWAP control message
 * @param msg Pointer to control message structure
 * @param buffer Output buffer
 * @param capacity Buffer capacity
 * @return Number of bytes written, or negative error code
 */
int cw_generate_control_msg(const cw_control_msg_t *msg, uint8_t *buffer, size_t capacity);

/**
 * Parse CAPWAP control message
 * @param buffer Input buffer
 * @param length Buffer length
 * @param msg Pointer to control message structure to fill
 * @return Number of bytes parsed, or negative error code
 */
int cw_parse_control_msg(const uint8_t *buffer, size_t length, cw_control_msg_t *msg);

/* ============= Message Element Functions ============= */

/**
 * Add message element to control message
 * @param msg Pointer to control message
 * @param type Element type
 * @param value Element value data
 * @param length Element value length
 * @return CW_MSG_SUCCESS on success, error code otherwise
 */
int cw_add_element(cw_control_msg_t *msg, uint16_t type, const uint8_t *value, uint16_t length);

/**
 * Find message element in control message
 * @param msg Pointer to control message
 * @param type Element type to find
 * @return Pointer to element, or NULL if not found
 */
const cw_msg_element_t* cw_find_element(const cw_control_msg_t *msg, uint16_t type);

/* ============= Specific Element Parsers ============= */

/**
 * Parse AC Descriptor element
 * @param elem Pointer to message element
 * @param desc Pointer to AC descriptor structure to fill
 * @return CW_MSG_SUCCESS on success, error code otherwise
 */
int cw_parse_ac_descriptor(const cw_msg_element_t *elem, cw_ac_descriptor_t *desc);

/**
 * Parse Result Code element
 * @param elem Pointer to message element
 * @param result Pointer to result code structure to fill
 * @return CW_MSG_SUCCESS on success, error code otherwise
 */
int cw_parse_result_code(const cw_msg_element_t *elem, cw_result_code_t *result);

/**
 * Parse Session ID element
 * @param elem Pointer to message element
 * @param session_id Pointer to session ID structure to fill
 * @return CW_MSG_SUCCESS on success, error code otherwise
 */
int cw_parse_session_id(const cw_msg_element_t *elem, cw_session_id_t *session_id);

/**
 * Parse AC Name element
 * @param elem Pointer to message element
 * @param name Pointer to AC name structure to fill
 * @return CW_MSG_SUCCESS on success, error code otherwise
 */
int cw_parse_ac_name(const cw_msg_element_t *elem, cw_ac_name_t *name);

/**
 * Parse WTP Name element
 * @param elem Pointer to message element
 * @param name Pointer to WTP name structure to fill
 * @return CW_MSG_SUCCESS on success, error code otherwise
 */
int cw_parse_wtp_name(const cw_msg_element_t *elem, cw_wtp_name_t *name);

/* ============= Specific Element Generators ============= */

/**
 * Generate AC Descriptor element
 * @param desc Pointer to AC descriptor structure
 * @param buffer Output buffer
 * @param capacity Buffer capacity
 * @return Number of bytes written, or negative error code
 */
int cw_generate_ac_descriptor(const cw_ac_descriptor_t *desc, uint8_t *buffer, size_t capacity);

/**
 * Generate Result Code element
 * @param result_code Result code value
 * @param buffer Output buffer
 * @param capacity Buffer capacity
 * @return Number of bytes written, or negative error code
 */
int cw_generate_result_code(uint32_t result_code, uint8_t *buffer, size_t capacity);

/**
 * Generate Session ID element
 * @param session_id Pointer to session ID structure
 * @param buffer Output buffer
 * @param capacity Buffer capacity
 * @return Number of bytes written, or negative error code
 */
int cw_generate_session_id(const cw_session_id_t *session_id, uint8_t *buffer, size_t capacity);

/**
 * Generate AC Name element
 * @param name AC name string
 * @param buffer Output buffer
 * @param capacity Buffer capacity
 * @return Number of bytes written, or negative error code
 */
int cw_generate_ac_name(const char *name, uint8_t *buffer, size_t capacity);

/* ============= Utility Functions ============= */

/**
 * Get message type name
 * @param msg_type Message type value
 * @return String representation of message type
 */
const char* cw_msg_type_name(uint32_t msg_type);

/**
 * Get element type name
 * @param elem_type Element type value
 * @return String representation of element type
 */
const char* cw_elem_type_name(uint16_t elem_type);

/**
 * Get error code string
 * @param error_code Error code value
 * @return String representation of error code
 */
const char* cw_msg_error_string(int error_code);

/**
 * Initialize control message structure
 * @param msg Pointer to control message
 * @param msg_type Message type
 * @param seq_num Sequence number
 */
void cw_init_control_msg(cw_control_msg_t *msg, uint32_t msg_type, uint8_t seq_num);

#endif /* CW_MSG_H */
