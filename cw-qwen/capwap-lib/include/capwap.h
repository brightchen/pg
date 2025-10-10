#ifndef CAPWAP_H
#define CAPWAP_H

#include <stdint.h>
#include <stddef.h>

// Error codes
#define CAPWAP_SUCCESS 0
#define CAPWAP_ERROR_INVALID_INPUT -1
#define CAPWAP_ERROR_BUFFER_OVERFLOW -2
#define CAPWAP_ERROR_PARSE_FAILED -3
#define CAPWAP_ERROR_BUILD_FAILED -4
#define CAPWAP_ERROR_UNSUPPORTED_VERSION -5
#define CAPWAP_ERROR_INVALID_MESSAGE_TYPE -6

// CAPWAP Protocol Constants
#define CAPWAP_VERSION_1 0
#define CAPWAP_HEADER_SIZE 8
#define CAPWAP_WIRELESS_BINDING_IEEE_80211 0
#define CAPWAP_PORT_CONTROL 5246
#define CAPWAP_PORT_DATA 5247

// Message Type Definitions
typedef enum {
    CAPWAP_DISCOVERY_REQUEST = 1,
    CAPWAP_DISCOVERY_RESPONSE = 2,
    CAPWAP_JOIN_REQUEST = 3,
    CAPWAP_JOIN_RESPONSE = 4,
    CAPWAP_CONFIG_STATUS_REQUEST = 5,
    CAPWAP_CONFIG_STATUS_RESPONSE = 6,
    CAPWAP_CONFIG_UPDATE_REQUEST = 7,
    CAPWAP_CONFIG_UPDATE_RESPONSE = 8,
    CAPWAP_CHANGE_STATE_EVENT_REQUEST = 9,
    CAPWAP_CHANGE_STATE_EVENT_RESPONSE = 10,
    CAPWAP_ECHO_REQUEST = 11,
    CAPWAP_ECHO_RESPONSE = 12,
    CAPWAP_IMAGE_DATA_REQUEST = 13,
    CAPWAP_IMAGE_DATA_RESPONSE = 14,
    CAPWAP_RESET_REQUEST = 15,
    CAPWAP_RESET_RESPONSE = 16,
    CAPWAP_PRIMARY_DISCOVERY_REQUEST = 17,
    CAPWAP_PRIMARY_DISCOVERY_RESPONSE = 18,
    CAPWAP_DATA_TRANSFER_REQUEST = 19,
    CAPWAP_DATA_TRANSFER_RESPONSE = 20,
    CAPWAP_CLEAR_CONFIG_REQUEST = 21,
    CAPWAP_CLEAR_CONFIG_RESPONSE = 22,
    CAPWAP_STATION_CONFIG_REQUEST = 23,
    CAPWAP_STATION_CONFIG_RESPONSE = 24,
    CAPWAP_WTP_EVENT_REQUEST = 25,
    CAPWAP_WTP_EVENT_RESPONSE = 26
} capwap_msg_type_t;

// Message Element Type Definitions
typedef enum {
    CAPWAP_ELEM_AC_DESCRIPTOR = 1,
    CAPWAP_ELEM_AC_IPV4_LIST = 2,
    CAPWAP_ELEM_AC_IPV6_LIST = 3,
    CAPWAP_ELEM_AC_NAME = 4,
    CAPWAP_ELEM_AC_TIMESTAMP = 6,
    CAPWAP_ELEM_CAPWAP_CONTROL_IPV4 = 7,
    CAPWAP_ELEM_CAPWAP_CONTROL_IPV6 = 8,
    CAPWAP_ELEM_CAPWAP_LOCAL_IPV4 = 9,
    CAPWAP_ELEM_CAPWAP_LOCAL_IPV6 = 10,
    CAPWAP_ELEM_CAPWAP_TIMERS = 11,
    CAPWAP_ELEM_DISCOVERY_TYPE = 21,
    CAPWAP_ELEM_RESULT_CODE = 35,
    CAPWAP_ELEM_SESSION_ID = 37,
    CAPWAP_ELEM_WTP_BOARD_DATA = 40,
    CAPWAP_ELEM_WTP_DESCRIPTOR = 41,
    CAPWAP_ELEM_WTP_NAME = 45
} capwap_elem_type_t;

// Discovery Type Values
typedef enum {
    CAPWAP_DISCOVERY_TYPE_STATIC = 1,
    CAPWAP_DISCOVERY_TYPE_DHCP = 2,
    CAPWAP_DISCOVERY_TYPE_DNS = 3,
    CAPWAP_DISCOVERY_TYPE_AC_REFERRAL = 4
} capwap_discovery_type_t;

// Result Code Values
typedef enum {
    CAPWAP_RESULT_SUCCESS = 0,
    CAPWAP_RESULT_FAILURE_AC_LIST = 1,
    CAPWAP_RESULT_FAILURE_UNRECOGNIZED_MESSAGE = 2,
    CAPWAP_RESULT_FAILURE_RESOURCE_DEPLETION = 3,
    CAPWAP_RESULT_FAILURE_UNSPECIFIED = 4
} capwap_result_code_t;

// CAPWAP Header Structure according to RFC 5415 Section 4.2
typedef struct {
    uint8_t version : 4;      // Version of CAPWAP protocol (4 bits) - should be 0 for version 1
    uint8_t type : 2;         // 0=data, 1=control (2 bits)
    uint8_t wireless_binding : 2; // Wireless technology binding ID (2 bits)
    uint8_t header_length : 5; // Length in 4-byte words (5 bits) (min 2 for basic header)
    uint8_t flags : 3;        // Header flags (3 bits)
    uint16_t fragment_id;     // Fragment identifier (network byte order)
    uint16_t fragment_offset : 13; // Fragment offset (network byte order) (13 bits)
    uint8_t final_fragment : 1; // Indicates final fragment (1 bit)
    uint8_t reserved2 : 2;    // Reserved bits (2 bits)
    uint8_t radio_id;         // WTP Radio ID
    uint16_t wtp_ac_id;      // WTP/AC ID (network byte order) - was wtp_ac_id in original
    uint32_t session_id;      // Session identifier (network byte order)
} capwap_header_t;

// CAPWAP Message Element Header
typedef struct {
    uint16_t type;            // Message element type (network byte order)
    uint16_t length;          // Length of data field (network byte order)
    uint8_t *data;            // Pointer to data (for parsing) or data buffer (for building)
} capwap_msg_element_t;

// CAPWAP Control Message Structure (for parsing)
typedef struct {
    capwap_header_t header;
    uint32_t *timestamp;      // Optional timestamp pointer (NULL if not present)
    capwap_msg_element_t *elements; // Array of message elements
    size_t num_elements;      // Number of elements
    capwap_msg_type_t msg_type; // The message type
} capwap_control_msg_t;

// CAPWAP Discovery Request Message Structure (for building/parsing)
typedef struct {
    capwap_discovery_type_t discovery_type;
    char *ac_name;            // Points to buffer data when parsing
    uint8_t *wtp_board_data;  // Points to buffer data when parsing
    size_t wtp_board_data_len;
    uint8_t *wtp_descriptor;  // Points to buffer data when parsing
    size_t wtp_descriptor_len;
} capwap_discovery_req_t;

// CAPWAP Discovery Response Message Structure (for building/parsing)
typedef struct {
    capwap_result_code_t result_code;
    uint8_t *ac_descriptor;   // Points to buffer data when parsing
    size_t ac_descriptor_len;
    char *ac_name;            // Points to buffer data when parsing
    uint32_t *ac_ipv4_list;   // Points to buffer data when parsing (network byte order)
    size_t num_ac_ipv4;
    uint8_t *ac_ipv6_list;    // Points to buffer data when parsing
    size_t num_ac_ipv6;
    uint8_t discovery_interval;  // Discovery timer value
    uint8_t echo_interval;       // Echo timer value
    uint8_t max_echo_interval;   // Max echo timer value
    uint8_t discovery_count;     // Discovery count
} capwap_discovery_resp_t;

// CAPWAP Join Request Message Structure (for building/parsing)
typedef struct {
    uint8_t *wtp_descriptor;  // Points to buffer data when parsing
    size_t wtp_descriptor_len;
    uint8_t *wtp_board_data;  // Points to buffer data when parsing
    size_t wtp_board_data_len;
    uint8_t *ac_control_addr; // Points to buffer data when parsing (IPv4 or IPv6)
    size_t ac_control_addr_len;
    char *wtp_name;           // Points to buffer data when parsing
    uint8_t *wtp_mac_address; // Points to buffer data when parsing (6 bytes)
} capwap_join_req_t;

// CAPWAP Join Response Message Structure (for building/parsing)
typedef struct {
    capwap_result_code_t result_code;
    uint8_t *ac_descriptor;   // Points to buffer data when parsing
    size_t ac_descriptor_len;
    uint8_t *session_id;      // Points to 16-byte session ID when parsing
    uint8_t wtp_mac_type;     // WTP MAC type when parsing
} capwap_join_resp_t;

// CAPWAP Echo Request Message Structure (for building/parsing)
typedef struct {
    // Echo request typically has no specific required elements
    // It may contain optional elements like AC Name, etc.
    char *ac_name;            // Points to buffer data when parsing
    uint32_t *statistics_timer; // Points to buffer data when parsing
} capwap_echo_req_t;

// CAPWAP Echo Response Message Structure (for building/parsing)
typedef struct {
    capwap_result_code_t result_code;
    char *ac_name;            // Points to buffer data when parsing
    uint32_t *statistics_timer; // Points to buffer data when parsing
} capwap_echo_resp_t;

// CAPWAP Configuration Status Request Message Structure (for building/parsing)
typedef struct {
    uint8_t *configuration_updates; // Points to buffer data when parsing
    size_t config_updates_len;
    uint8_t *encryption_key_update; // Points to buffer data when parsing
    size_t encryption_key_len;
} capwap_config_status_req_t;

// CAPWAP Configuration Status Response Message Structure (for building/parsing)
typedef struct {
    capwap_result_code_t result_code;
    uint8_t *configuration_ack; // Points to buffer data when parsing  
    size_t config_ack_len;
} capwap_config_status_resp_t;

// CAPWAP Configuration Update Request Message Structure (for building/parsing)
typedef struct {
    uint8_t *configuration_updates; // Points to buffer data when parsing
    size_t config_updates_len;
    uint8_t *encryption_key_update; // Points to buffer data when parsing
    size_t encryption_key_len;
} capwap_config_update_req_t;

// CAPWAP Configuration Update Response Message Structure (for building/parsing)
typedef struct {
    capwap_result_code_t result_code;
} capwap_config_update_resp_t;

// CAPWAP Change State Event Request Message Structure (for building/parsing)
typedef struct {
    uint8_t *wtp_event; // Points to buffer data when parsing
    size_t wtp_event_len;
    uint8_t *wtp_board_data; // Points to buffer data when parsing
    size_t wtp_board_data_len;
} capwap_change_state_event_req_t;

// CAPWAP Change State Event Response Message Structure (for building/parsing)
typedef struct {
    capwap_result_code_t result_code;
} capwap_change_state_event_resp_t;

// CAPWAP Primary Discovery Request Message Structure (for building/parsing)
typedef struct {
    capwap_discovery_type_t discovery_type;
    uint8_t *wtp_mac_address; // Points to buffer data when parsing (6 bytes)
} capwap_primary_discovery_req_t;

// CAPWAP Primary Discovery Response Message Structure (for building/parsing)
typedef struct {
    capwap_result_code_t result_code;
    uint8_t *primary_ac_ip_address; // Points to buffer data when parsing
    size_t primary_ac_ip_len;
} capwap_primary_discovery_resp_t;

// CAPWAP Data Transfer Request Message Structure (for building/parsing)
typedef struct {
    uint8_t *vendor_data; // Points to buffer data when parsing
    size_t vendor_data_len;
} capwap_data_transfer_req_t;

// CAPWAP Data Transfer Response Message Structure (for building/parsing)
typedef struct {
    capwap_result_code_t result_code;
} capwap_data_transfer_resp_t;

// CAPWAP Clear Configuration Request Message Structure (for building/parsing)
typedef struct {
    // No specific required elements
} capwap_clear_config_req_t;

// CAPWAP Clear Configuration Response Message Structure (for building/parsing)
typedef struct {
    capwap_result_code_t result_code;
} capwap_clear_config_resp_t;

// CAPWAP Station Configuration Request Message Structure (for building/parsing)
typedef struct {
    uint8_t *station_mac_address; // Points to buffer data when parsing (6 bytes)
    uint8_t *station_info; // Points to buffer data when parsing
    size_t station_info_len;
} capwap_station_config_req_t;

// CAPWAP Station Configuration Response Message Structure (for building/parsing)
typedef struct {
    capwap_result_code_t result_code;
} capwap_station_config_resp_t;

// CAPWAP WTP Event Request Message Structure (for building/parsing)
typedef struct {
    uint8_t *wtp_event; // Points to buffer data when parsing
    size_t wtp_event_len;
    uint8_t *wtp_board_data; // Points to buffer data when parsing
    size_t wtp_board_data_len;
    uint8_t *radio_statistics; // Points to buffer data when parsing
    size_t radio_stats_len;
} capwap_wtp_event_req_t;

// CAPWAP WTP Event Response Message Structure (for building/parsing)
typedef struct {
    capwap_result_code_t result_code;
} capwap_wtp_event_resp_t;

// CAPWAP Reset Request Message Structure (for building/parsing)
typedef struct {
    uint8_t reset_type; // 0 for cold reset, 1 for warm reset
} capwap_reset_req_t;

// CAPWAP Reset Response Message Structure (for building/parsing)
typedef struct {
    capwap_result_code_t result_code;
} capwap_reset_resp_t;

// CAPWAP Image Data Request Message Structure (for building/parsing)
typedef struct {
    uint32_t image_data_offset; // Offset in the image data
    uint8_t *image_data_chunk; // Points to buffer data when parsing
    size_t image_data_len;
    uint8_t *image_identifier; // Points to buffer data when parsing
    size_t image_id_len;
    uint8_t image_status; // Status of the image transfer
} capwap_image_data_req_t;

// CAPWAP Image Data Response Message Structure (for building/parsing)
typedef struct {
    capwap_result_code_t result_code;
    uint8_t *image_data_chunk; // Points to buffer data when parsing
    size_t image_data_len;
    uint32_t image_data_offset; // Offset in the image data
} capwap_image_data_resp_t;

#ifdef __cplusplus
extern "C" {
#endif

// Common header functions
int capwap_parse_header(uint8_t *buffer, size_t len, capwap_header_t *header);
int capwap_build_header(uint8_t *buffer, size_t *len, capwap_header_t *header);

// Message element functions
int capwap_parse_message_element(uint8_t *buffer, size_t len, capwap_msg_element_t *elem, size_t *bytes_consumed);
int capwap_build_message_element(uint8_t *buffer, size_t *len, capwap_msg_element_t *elem);

// Control message generation functions
int capwap_build_discovery_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_discovery_req_t *msg);
int capwap_build_discovery_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_discovery_resp_t *msg);
int capwap_build_join_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_join_req_t *msg);
int capwap_build_join_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_join_resp_t *msg);
int capwap_build_echo_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_echo_req_t *msg);
int capwap_build_echo_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_echo_resp_t *msg);
int capwap_build_config_status_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_config_status_req_t *msg);
int capwap_build_config_status_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_config_status_resp_t *msg);
int capwap_build_config_update_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_config_update_req_t *msg);
int capwap_build_config_update_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_config_update_resp_t *msg);
int capwap_build_change_state_event_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_change_state_event_req_t *msg);
int capwap_build_change_state_event_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_change_state_event_resp_t *msg);
int capwap_build_primary_discovery_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_primary_discovery_req_t *msg);
int capwap_build_primary_discovery_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_primary_discovery_resp_t *msg);
int capwap_build_data_transfer_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_data_transfer_req_t *msg);
int capwap_build_data_transfer_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_data_transfer_resp_t *msg);
int capwap_build_clear_config_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_clear_config_req_t *msg);
int capwap_build_clear_config_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_clear_config_resp_t *msg);
int capwap_build_station_config_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_station_config_req_t *msg);
int capwap_build_station_config_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_station_config_resp_t *msg);
int capwap_build_wtp_event_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_wtp_event_req_t *msg);
int capwap_build_wtp_event_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_wtp_event_resp_t *msg);
int capwap_build_reset_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_reset_req_t *msg);
int capwap_build_reset_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_reset_resp_t *msg);
int capwap_build_image_data_request(uint8_t *buffer, size_t *len, size_t capacity, capwap_image_data_req_t *msg);
int capwap_build_image_data_response(uint8_t *buffer, size_t *len, size_t capacity, capwap_image_data_resp_t *msg);

// Control message parsing functions
int capwap_parse_discovery_request(uint8_t *buffer, size_t len, capwap_discovery_req_t *msg);
int capwap_parse_discovery_response(uint8_t *buffer, size_t len, capwap_discovery_resp_t *msg);
int capwap_parse_join_request(uint8_t *buffer, size_t len, capwap_join_req_t *msg);
int capwap_parse_join_response(uint8_t *buffer, size_t len, capwap_join_resp_t *msg);
int capwap_parse_echo_request(uint8_t *buffer, size_t len, capwap_echo_req_t *msg);
int capwap_parse_echo_response(uint8_t *buffer, size_t len, capwap_echo_resp_t *msg);
int capwap_parse_config_status_request(uint8_t *buffer, size_t len, capwap_config_status_req_t *msg);
int capwap_parse_config_status_response(uint8_t *buffer, size_t len, capwap_config_status_resp_t *msg);
int capwap_parse_config_update_request(uint8_t *buffer, size_t len, capwap_config_update_req_t *msg);
int capwap_parse_config_update_response(uint8_t *buffer, size_t len, capwap_config_update_resp_t *msg);
int capwap_parse_change_state_event_request(uint8_t *buffer, size_t len, capwap_change_state_event_req_t *msg);
int capwap_parse_change_state_event_response(uint8_t *buffer, size_t len, capwap_change_state_event_resp_t *msg);
int capwap_parse_primary_discovery_request(uint8_t *buffer, size_t len, capwap_primary_discovery_req_t *msg);
int capwap_parse_primary_discovery_response(uint8_t *buffer, size_t len, capwap_primary_discovery_resp_t *msg);
int capwap_parse_data_transfer_request(uint8_t *buffer, size_t len, capwap_data_transfer_req_t *msg);
int capwap_parse_data_transfer_response(uint8_t *buffer, size_t len, capwap_data_transfer_resp_t *msg);
int capwap_parse_clear_config_request(uint8_t *buffer, size_t len, capwap_clear_config_req_t *msg);
int capwap_parse_clear_config_response(uint8_t *buffer, size_t len, capwap_clear_config_resp_t *msg);
int capwap_parse_station_config_request(uint8_t *buffer, size_t len, capwap_station_config_req_t *msg);
int capwap_parse_station_config_response(uint8_t *buffer, size_t len, capwap_station_config_resp_t *msg);
int capwap_parse_wtp_event_request(uint8_t *buffer, size_t len, capwap_wtp_event_req_t *msg);
int capwap_parse_wtp_event_response(uint8_t *buffer, size_t len, capwap_wtp_event_resp_t *msg);
int capwap_parse_reset_request(uint8_t *buffer, size_t len, capwap_reset_req_t *msg);
int capwap_parse_reset_response(uint8_t *buffer, size_t len, capwap_reset_resp_t *msg);
int capwap_parse_image_data_request(uint8_t *buffer, size_t len, capwap_image_data_req_t *msg);
int capwap_parse_image_data_response(uint8_t *buffer, size_t len, capwap_image_data_resp_t *msg);

// Utility functions
uint16_t capwap_htons(uint16_t host_short);
uint32_t capwap_htonl(uint32_t host_long);
uint16_t capwap_ntohs(uint16_t net_short);
uint32_t capwap_ntohl(uint32_t net_long);

#ifdef __cplusplus
}
#endif

#endif // CAPWAP_H