# CW-MSG-LIB - CAPWAP Message Library

A C library for generating and parsing CAPWAP (Control And Provisioning of Wireless Access Points) protocol messages according to RFC 5415.

## Features

- **Complete CAPWAP Message Support**: All control message types defined in RFC 5415
- **Message Element Handling**: Parse and generate standard CAPWAP message elements
- **Zero-Copy Parsing**: Efficient parsing that points to data without copying
- **Buffer Safety**: All functions check buffer bounds to prevent overflow
- **Thread-Safe**: No global state, safe for concurrent use
- **Comprehensive API**: High-level and low-level functions for flexibility

## Supported Message Types

### Control Messages
- Discovery Request/Response
- Join Request/Response
- Configuration Status Request/Response
- Configuration Update Request/Response
- WTP Event Request/Response
- Change State Event Request/Response
- Echo Request/Response
- Image Data Request/Response
- Reset Request/Response

### Message Elements
- AC Descriptor
- AC Name
- AC IPv4/IPv6 List
- Session ID
- Result Code
- WTP Descriptor
- WTP Name
- WTP Board Data
- Radio Administrative State
- Discovery Type
- Location Data
- And more...

## Building

### Build All Libraries
```bash
make
```

### Run Unit Tests
```bash
make test
```

### Install System-Wide
```bash
sudo make install
```

### Clean Build
```bash
make clean
```

## Usage

### Basic Message Generation

```c
#include "cw_msg.h"

// Create a Discovery Request
cw_control_msg_t msg;
cw_init_control_msg(&msg, CW_MSG_TYPE_DISCOVERY_REQUEST, 1);

// Add Discovery Type element
uint8_t discovery_type = CW_DISCOVERY_TYPE_DHCP;
cw_add_element(&msg, CW_ELEM_DISCOVERY_TYPE, &discovery_type, 1);

// Generate the message
uint8_t buffer[1024];
int len = cw_generate_control_msg(&msg, buffer, sizeof(buffer));

// Send buffer over network...
```

### Basic Message Parsing

```c
#include "cw_msg.h"

// Parse received message
cw_control_msg_t msg;
int len = cw_parse_control_msg(buffer, buffer_len, &msg);

if (len > 0) {
    printf("Received: %s\n", cw_msg_type_name(msg.ctrl_header.message_type));

    // Find specific element
    const cw_msg_element_t *elem = cw_find_element(&msg, CW_ELEM_AC_NAME);
    if (elem) {
        cw_ac_name_t ac_name;
        cw_parse_ac_name(elem, &ac_name);
        printf("AC Name: %.*s\n", ac_name.length, ac_name.name);
    }
}
```

### Generate Discovery Response

```c
cw_control_msg_t resp;
cw_init_control_msg(&resp, CW_MSG_TYPE_DISCOVERY_RESPONSE, seq_num);

// Add AC Descriptor
cw_ac_descriptor_t ac_desc = {
    .stations = 0,
    .limit = 1000,
    .active_wtps = 10,
    .max_wtps = 100,
    .security = 0x01,
    .rmac_field = 0x01,
    .dtls_policy = 0x01
};

uint8_t elem_buffer[128];
int elem_len = cw_generate_ac_descriptor(&ac_desc, elem_buffer, sizeof(elem_buffer));
cw_add_element(&resp, CW_ELEM_AC_DESCRIPTOR, &elem_buffer[4], elem_len - 4);

// Add AC Name
const char *ac_name = "MyController";
cw_add_element(&resp, CW_ELEM_AC_NAME, (uint8_t *)ac_name, strlen(ac_name));

// Generate message
uint8_t buffer[2048];
int len = cw_generate_control_msg(&resp, buffer, sizeof(buffer));
```

### Working with Elements

```c
// Generate Result Code element
uint8_t result_buffer[16];
int len = cw_generate_result_code(CW_RESULT_SUCCESS, result_buffer, sizeof(result_buffer));

// Generate Session ID element
cw_session_id_t session_id;
session_id.length = 16;
// ... fill session_id.session_id array ...
int len = cw_generate_session_id(&session_id, buffer, sizeof(buffer));

// Parse elements
const cw_msg_element_t *elem = cw_find_element(&msg, CW_ELEM_RESULT_CODE);
if (elem) {
    cw_result_code_t result;
    cw_parse_result_code(elem, &result);
    printf("Result: %u\n", result.result_code);
}
```

## API Overview

### Message Initialization
- `cw_init_control_msg()` - Initialize control message structure

### Message Generation/Parsing
- `cw_generate_control_msg()` - Generate complete control message
- `cw_parse_control_msg()` - Parse control message from buffer
- `cw_generate_header()` - Generate CAPWAP header
- `cw_parse_header()` - Parse CAPWAP header

### Element Management
- `cw_add_element()` - Add element to message
- `cw_find_element()` - Find element by type
- `cw_generate_*()` - Generate specific element types
- `cw_parse_*()` - Parse specific element types

### Utility Functions
- `cw_msg_type_name()` - Get message type name string
- `cw_elem_type_name()` - Get element type name string

## Error Codes

- `CW_MSG_SUCCESS` (0) - Success
- `CW_MSG_ERROR_INVALID_PARAM` (-1) - Invalid parameter
- `CW_MSG_ERROR_BUFFER_TOO_SMALL` (-2) - Buffer too small
- `CW_MSG_ERROR_PARSE_FAILED` (-3) - Parse failed
- `CW_MSG_ERROR_UNSUPPORTED_TYPE` (-4) - Unsupported type
- `CW_MSG_ERROR_INVALID_FORMAT` (-5) - Invalid format

## Zero-Copy Parsing

The library uses zero-copy parsing for efficiency. When parsing messages, element values point directly into the original buffer rather than copying data. This means:

1. **Fast**: No memory allocation or copying during parse
2. **Efficient**: Minimal memory overhead
3. **Lifetime**: Parsed structure is only valid while original buffer exists
4. **Read-Only**: Don't modify the original buffer after parsing

```c
// The buffer must remain valid
uint8_t buffer[1024];
receive_from_network(buffer, sizeof(buffer));

cw_control_msg_t msg;
cw_parse_control_msg(buffer, len, &msg);

// msg.elements[].value points into buffer
// Don't free or modify buffer while using msg
```

## Compilation

### Link with Static Library
```bash
gcc myapp.c -I./include -L./lib -lcwmsg -o myapp
```

### Link with Shared Library
```bash
gcc myapp.c -I./include -L./lib -lcwmsg -o myapp
```

## Thread Safety

The library is thread-safe as it maintains no global state. Different threads can safely use the library concurrently as long as they don't share message structures or buffers without proper synchronization.

## RFC 5415 Compliance

This library implements the CAPWAP protocol as specified in RFC 5415:
- CAPWAP header format (Section 4.3)
- Control message format (Section 6)
- Message element encoding (Section 4.6)
- All standard message types
- All standard message elements

## License

Part of the CAPWAP Test Suite project.
