# CAPWAP Message Library

A C library for generating and parsing CAPWAP (Control And Provisioning of Wireless Access Points) messages.

## Overview

This library provides functions to build and parse all CAPWAP control messages as defined in RFC 5415. It includes support for:

- All 26 CAPWAP control message types
- Message element parsing and generation
- Header manipulation
- Network byte order conversion
- Buffer overflow protection 
- Error handling and validation

## Features

- **Complete CAPWAP Support**: All control messages types (Discovery, Join, Configuration, etc.)
- **Buffer Safety**: Checks for buffer overflow conditions
- **Zero-copy Parsing**: During parsing, structure fields point to buffer data without cloning
- **Network Byte Order**: Proper handling of endianness for network protocols
- **Comprehensive Error Handling**: Detailed error codes for debugging

## Installation

```bash
make all
sudo make install
```

## Building

To build the library and run tests:

```bash
# Build library and tests
make

# Run unit tests
make run-test

# Build example program
make example

# Run example program
./build/example

# Clean build artifacts
make clean
```

## Usage

### Including the Library

```c
#include "capwap.h"
```

### Message Generation

```c
uint8_t buffer[1024];
size_t length;
capwap_discovery_req_t msg = {0};
msg.discovery_type = CAPWAP_DISCOVERY_TYPE_DHCP;

int result = capwap_build_discovery_request(buffer, &length, sizeof(buffer), &msg);
if (result == CAPWAP_SUCCESS) {
    // Message generated successfully
    // length contains the actual message length
}
```

### Message Parsing

```c
capwap_discovery_req_t msg = {0};
int result = capwap_parse_discovery_request(buffer, length, &msg);
if (result == CAPWAP_SUCCESS) {
    // Message parsed successfully
    // msg.discovery_type contains the parsed value
    // Note: Structure fields point to data within the buffer (zero-copy)
}
```

### Complete Example

A complete example demonstrating library usage is available in `example.c`:

```c
#include "capwap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // Example: Building a Discovery Request message
    uint8_t buffer[1024];
    size_t length;
    
    capwap_discovery_req_t discovery_req = {0};
    discovery_req.discovery_type = CAPWAP_DISCOVERY_TYPE_DHCP;
    
    int result = capwap_build_discovery_request(buffer, &length, sizeof(buffer), &discovery_req);
    if (result == CAPWAP_SUCCESS) {
        printf("Discovery Request built successfully, length: %zu bytes\n", length);
    }
    
    // Example: Parsing the Discovery Request message
    capwap_discovery_req_t parsed_discovery_req = {0};
    result = capwap_parse_discovery_request(buffer, length, &parsed_discovery_req);
    if (result == CAPWAP_SUCCESS) {
        printf("Discovery Request parsed successfully\n");
        printf("Discovery Type: %d\n", parsed_discovery_req.discovery_type);
    }
    
    return 0;
}
```

To compile the example:
```bash
gcc -I. example.c -Lbuild -lcapwap -o example
```

## API Functions

### Header Functions
- `capwap_build_header()`: Build CAPWAP header
- `capwap_parse_header()`: Parse CAPWAP header

### Message Element Functions
- `capwap_build_message_element()`: Build message element
- `capwap_parse_message_element()`: Parse message element

### Control Message Functions
Each message type has both build and parse functions:

- Discovery messages: `capwap_build_discovery_request()`, `capwap_parse_discovery_request()`, etc.
- Join messages: `capwap_build_join_request()`, `capwap_parse_join_request()`, etc.
- Echo messages: `capwap_build_echo_request()`, `capwap_parse_echo_request()`, etc.
- Configuration messages: `capwap_build_config_status_request()`, `capwap_parse_config_status_request()`, etc.
- And all other CAPWAP message types

### Error Codes

- `CAPWAP_SUCCESS`: Operation successful (0)
- `CAPWAP_ERROR_INVALID_INPUT`: Invalid input parameters (-1)
- `CAPWAP_ERROR_BUFFER_OVERFLOW`: Buffer too small (-2)
- `CAPWAP_ERROR_PARSE_FAILED`: Parse operation failed (-3)
- `CAPWAP_ERROR_BUILD_FAILED`: Build operation failed (-4)
- `CAPWAP_ERROR_UNSUPPORTED_VERSION`: Unsupported protocol version (-5)
- `CAPWAP_ERROR_INVALID_MESSAGE_TYPE`: Invalid message type (-6)

## Message Types Supported

The library supports all standard CAPWAP message types:

1. Discovery Request/Response
2. Join Request/Response
3. Configuration Status Request/Response
4. Configuration Update Request/Response
5. Change State Event Request/Response
6. Echo Request/Response
7. Image Data Request/Response
8. Reset Request/Response
9. Primary Discovery Request/Response
10. Data Transfer Request/Response
11. Clear Configuration Request/Response
12. Station Configuration Request/Response
13. WTP Event Request/Response

## Message Elements Supported

- AC Descriptor
- AC IPv4/IPv6 List
- AC Name
- AC Timestamp
- CAPWAP Control IPv4/IPv6
- CAPWAP Local IPv4/IPv6
- CAPWAP Timers
- Discovery Type
- Result Code
- Session ID
- WTP Descriptor
- WTP Name
- And more

## Testing

Run the unit tests with:

```bash
make run-test
```

The tests cover all message types and error conditions.

## License

[MIT License](LICENSE)