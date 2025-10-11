#ifndef CW_MSG_TYPES_H
#define CW_MSG_TYPES_H

#include <stdint.h>

/* CAPWAP Message Type Constants (RFC 5415 Section 6) */
#define CW_MSG_TYPE_DISCOVERY_REQUEST          1
#define CW_MSG_TYPE_DISCOVERY_RESPONSE         2
#define CW_MSG_TYPE_JOIN_REQUEST               3
#define CW_MSG_TYPE_JOIN_RESPONSE              4
#define CW_MSG_TYPE_CONFIGURATION_STATUS_REQUEST   5
#define CW_MSG_TYPE_CONFIGURATION_STATUS_RESPONSE  6
#define CW_MSG_TYPE_CONFIGURATION_UPDATE_REQUEST   7
#define CW_MSG_TYPE_CONFIGURATION_UPDATE_RESPONSE  8
#define CW_MSG_TYPE_WTP_EVENT_REQUEST          9
#define CW_MSG_TYPE_WTP_EVENT_RESPONSE         10
#define CW_MSG_TYPE_CHANGE_STATE_EVENT_REQUEST 11
#define CW_MSG_TYPE_CHANGE_STATE_EVENT_RESPONSE 12
#define CW_MSG_TYPE_ECHO_REQUEST               13
#define CW_MSG_TYPE_ECHO_RESPONSE              14
#define CW_MSG_TYPE_IMAGE_DATA_REQUEST         15
#define CW_MSG_TYPE_IMAGE_DATA_RESPONSE        16
#define CW_MSG_TYPE_RESET_REQUEST              17
#define CW_MSG_TYPE_RESET_RESPONSE             18
#define CW_MSG_TYPE_PRIMARY_DISCOVERY_REQUEST  19
#define CW_MSG_TYPE_PRIMARY_DISCOVERY_RESPONSE 20
#define CW_MSG_TYPE_STATION_CONFIG_REQUEST     25
#define CW_MSG_TYPE_STATION_CONFIG_RESPONSE    26

/* CAPWAP Message Element Type Constants (IANA Registry) */
#define CW_ELEM_AC_DESCRIPTOR                  1
#define CW_ELEM_AC_IPV4_LIST                   2
#define CW_ELEM_AC_IPV6_LIST                   3
#define CW_ELEM_AC_NAME                        4
#define CW_ELEM_AC_NAME_WITH_PRIORITY          5
#define CW_ELEM_AC_TIMESTAMP                   6
#define CW_ELEM_ADD_MAC_ACL_ENTRY              7
#define CW_ELEM_ADD_STATION                    8
#define CW_ELEM_CAPWAP_CONTROL_IPV4_ADDRESS    10
#define CW_ELEM_CAPWAP_CONTROL_IPV6_ADDRESS    11
#define CW_ELEM_CAPWAP_TIMERS                  12
#define CW_ELEM_DATA_TRANSFER_DATA             13
#define CW_ELEM_DATA_TRANSFER_MODE             14
#define CW_ELEM_DECRYPTION_ERROR_REPORT        15
#define CW_ELEM_DECRYPTION_ERROR_REPORT_PERIOD 16
#define CW_ELEM_DELETE_MAC_ACL_ENTRY           17
#define CW_ELEM_DELETE_STATION                 18
#define CW_ELEM_DISCOVERY_TYPE                 20
#define CW_ELEM_DUPLICATE_IPV4_ADDRESS         21
#define CW_ELEM_DUPLICATE_IPV6_ADDRESS         22
#define CW_ELEM_IDLE_TIMEOUT                   23
#define CW_ELEM_IMAGE_DATA                     24
#define CW_ELEM_IMAGE_IDENTIFIER               25
#define CW_ELEM_IMAGE_INFORMATION              26
#define CW_ELEM_INITIATE_DOWNLOAD              27
#define CW_ELEM_LOCATION_DATA                  28
#define CW_ELEM_MAXIMUM_MESSAGE_LENGTH         29
#define CW_ELEM_CAPWAP_LOCAL_IPV4_ADDRESS      30
#define CW_ELEM_RADIO_ADMINISTRATIVE_STATE     31
#define CW_ELEM_RADIO_OPERATIONAL_STATE        32
#define CW_ELEM_RESULT_CODE                    33
#define CW_ELEM_RETURNED_MESSAGE_ELEMENT       34
#define CW_ELEM_SESSION_ID                     35
#define CW_ELEM_STATISTICS_TIMER               36
#define CW_ELEM_VENDOR_SPECIFIC_PAYLOAD        37
#define CW_ELEM_WTP_BOARD_DATA                 38
#define CW_ELEM_WTP_DESCRIPTOR                 39
#define CW_ELEM_WTP_FALLBACK                   40
#define CW_ELEM_WTP_FRAME_TUNNEL_MODE          41
#define CW_ELEM_WTP_MAC_TYPE                   44
#define CW_ELEM_WTP_NAME                       45
#define CW_ELEM_WTP_RADIO_STATISTICS           47
#define CW_ELEM_WTP_REBOOT_STATISTICS          48
#define CW_ELEM_WTP_STATIC_IP_ADDRESS          49
#define CW_ELEM_CAPWAP_LOCAL_IPV6_ADDRESS      50
#define CW_ELEM_CAPWAP_TRANSPORT_PROTOCOL      51

/* CAPWAP Header Flags */
#define CW_FLAG_HDR_T  0x01  /* Type flag (0=Data, 1=Control) */
#define CW_FLAG_HDR_F  0x02  /* Fragment flag */
#define CW_FLAG_HDR_L  0x04  /* Last fragment flag */
#define CW_FLAG_HDR_W  0x08  /* Wireless flag */
#define CW_FLAG_HDR_M  0x10  /* Radio MAC flag */
#define CW_FLAG_HDR_K  0x20  /* Keep-Alive flag */

/* CAPWAP Preamble Constants */
#define CW_PREAMBLE_VERSION_0  0x00

/* Result Code Values */
#define CW_RESULT_SUCCESS                      0
#define CW_RESULT_FAILURE_AC_LIST              1
#define CW_RESULT_FAILURE_JOIN_RESOURCE        2
#define CW_RESULT_FAILURE_JOIN_NOT_AUTHORIZED  3
#define CW_RESULT_FAILURE_UNKNOWN              255

/* Discovery Type Values */
#define CW_DISCOVERY_TYPE_UNKNOWN    0
#define CW_DISCOVERY_TYPE_STATIC     1
#define CW_DISCOVERY_TYPE_DHCP       2
#define CW_DISCOVERY_TYPE_DNS        3
#define CW_DISCOVERY_TYPE_AC_REFERRAL 4

/* Radio Administrative State Values */
#define CW_RADIO_ADMIN_STATE_RESERVED  0
#define CW_RADIO_ADMIN_STATE_ENABLED   1
#define CW_RADIO_ADMIN_STATE_DISABLED  2

/* Maximum Sizes */
#define CW_MAX_VENDOR_ID_LEN      128
#define CW_MAX_MODEL_NUMBER_LEN   128
#define CW_MAX_SERIAL_NUMBER_LEN  128
#define CW_MAX_NAME_LEN           512
#define CW_MAX_LOCATION_LEN       1024
#define CW_MAX_SESSION_ID_LEN     16
#define CW_MAX_ELEMENTS           256

#endif /* CW_MSG_TYPES_H */
