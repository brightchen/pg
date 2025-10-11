# CW-DTLS - CAPWAP DTLS Library

A C library providing DTLS (Datagram Transport Layer Security) encryption and decryption for CAPWAP protocol using OpenSSL.

## Features

- **DTLS 1.0/1.2 Support**: Full DTLS protocol support via OpenSSL
- **Client and Server Roles**: Support for both DTLS client and server modes
- **Certificate Management**: Load certificates, private keys, and CA files
- **Flexible Verification**: Optional peer certificate verification
- **Non-blocking I/O**: Compatible with non-blocking socket operations
- **Error Handling**: Comprehensive error codes and error strings

## Dependencies

- OpenSSL (libssl, libcrypto)

### Install OpenSSL
```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev

# CentOS/RHEL
sudo yum install openssl-devel

# macOS
brew install openssl
```

## Building

### Build Libraries
```bash
make
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

### Initialize Library

```c
#include "cw_dtls.h"

int main() {
    // Initialize DTLS library (call once at program start)
    cw_dtls_library_init();

    // ... use DTLS ...

    // Cleanup (call once at program end)
    cw_dtls_library_cleanup();
    return 0;
}
```

### Create DTLS Client

```c
// Configure DTLS client
cw_dtls_config_t config = {
    .cert_file = "client-cert.pem",
    .key_file = "client-key.pem",
    .ca_file = "ca-cert.pem",
    .role = CW_DTLS_ROLE_CLIENT,
    .verify_peer = 1
};

// Create context
cw_dtls_ctx_t *ctx = cw_dtls_create_context(&config);
if (ctx == NULL) {
    printf("Failed to create DTLS context\n");
    return -1;
}

// Set socket
int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
cw_dtls_set_socket(ctx, sockfd);

// Perform handshake
while (!cw_dtls_is_handshake_complete(ctx)) {
    int ret = cw_dtls_handshake(ctx);
    if (ret == CW_DTLS_SUCCESS) {
        break;
    } else if (ret == CW_DTLS_ERROR_HANDSHAKE) {
        // Handshake in progress, continue
        continue;
    } else {
        printf("Handshake failed: %s\n", cw_dtls_get_error_string());
        break;
    }
}

// Send encrypted data
uint8_t data[] = "Hello, encrypted world!";
int sent = cw_dtls_write(ctx, data, sizeof(data));

// Receive encrypted data
uint8_t buffer[1024];
int received = cw_dtls_read(ctx, buffer, sizeof(buffer));

// Cleanup
cw_dtls_destroy_context(ctx);
```

### Create DTLS Server

```c
// Configure DTLS server
cw_dtls_config_t config = {
    .cert_file = "server-cert.pem",
    .key_file = "server-key.pem",
    .ca_file = "ca-cert.pem",
    .role = CW_DTLS_ROLE_SERVER,
    .verify_peer = 1
};

// Create context
cw_dtls_ctx_t *ctx = cw_dtls_create_context(&config);

// Set socket
int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
bind(sockfd, ...);  // Bind to address
cw_dtls_set_socket(ctx, sockfd);

// Accept connection and perform handshake
while (!cw_dtls_is_handshake_complete(ctx)) {
    int ret = cw_dtls_handshake(ctx);
    if (ret == CW_DTLS_SUCCESS) {
        break;
    }
    // Handle errors...
}

// Read/write encrypted data
uint8_t buffer[1024];
int received = cw_dtls_read(ctx, buffer, sizeof(buffer));

// Cleanup
cw_dtls_destroy_context(ctx);
```

### Generate Test Certificates

For testing purposes, you can generate self-signed certificates:

```bash
# Generate CA private key and certificate
openssl req -x509 -newkey rsa:2048 -days 365 -nodes \
    -keyout ca-key.pem -out ca-cert.pem \
    -subj "/C=US/ST=CA/L=San Francisco/O=Test/CN=Test CA"

# Generate server private key
openssl genrsa -out server-key.pem 2048

# Generate server certificate signing request
openssl req -new -key server-key.pem -out server-csr.pem \
    -subj "/C=US/ST=CA/L=San Francisco/O=Test/CN=Test Server"

# Sign server certificate with CA
openssl x509 -req -in server-csr.pem -CA ca-cert.pem \
    -CAkey ca-key.pem -CAcreateserial -out server-cert.pem -days 365

# Generate client private key and certificate (similar process)
openssl genrsa -out client-key.pem 2048
openssl req -new -key client-key.pem -out client-csr.pem \
    -subj "/C=US/ST=CA/L=San Francisco/O=Test/CN=Test Client"
openssl x509 -req -in client-csr.pem -CA ca-cert.pem \
    -CAkey ca-key.pem -CAcreateserial -out client-cert.pem -days 365
```

## API Reference

### Initialization
- `cw_dtls_library_init()` - Initialize DTLS library
- `cw_dtls_library_cleanup()` - Cleanup DTLS library

### Context Management
- `cw_dtls_create_context()` - Create DTLS context with configuration
- `cw_dtls_destroy_context()` - Destroy DTLS context
- `cw_dtls_set_socket()` - Set socket file descriptor

### DTLS Operations
- `cw_dtls_handshake()` - Perform DTLS handshake
- `cw_dtls_read()` - Read and decrypt data
- `cw_dtls_write()` - Encrypt and write data
- `cw_dtls_is_handshake_complete()` - Check handshake status

### Error Handling
- `cw_dtls_get_error_string()` - Get last error string

## Error Codes

- `CW_DTLS_SUCCESS` (0) - Success
- `CW_DTLS_ERROR_INVALID_PARAM` (-1) - Invalid parameter
- `CW_DTLS_ERROR_SSL_INIT` (-2) - SSL initialization error
- `CW_DTLS_ERROR_CERT_LOAD` (-3) - Certificate load error
- `CW_DTLS_ERROR_KEY_LOAD` (-4) - Private key load error
- `CW_DTLS_ERROR_CONNECTION` (-5) - Connection error
- `CW_DTLS_ERROR_HANDSHAKE` (-6) - Handshake error
- `CW_DTLS_ERROR_READ` (-7) - Read error
- `CW_DTLS_ERROR_WRITE` (-8) - Write error

## Thread Safety

Each DTLS context is independent and can be used by different threads. However, a single context should not be used concurrently by multiple threads without proper synchronization.

## Integration with CAPWAP

This library is designed to be used with the CAPWAP protocol:

```c
// Receive CAPWAP control packet
uint8_t encrypted_buffer[2048];
int recv_len = recvfrom(sockfd, encrypted_buffer, sizeof(encrypted_buffer), ...);

// Decrypt with DTLS
uint8_t decrypted_buffer[2048];
int decrypted_len = cw_dtls_read(dtls_ctx, decrypted_buffer, sizeof(decrypted_buffer));

// Parse CAPWAP message
cw_control_msg_t msg;
cw_parse_control_msg(decrypted_buffer, decrypted_len, &msg);

// Process CAPWAP message...

// Generate CAPWAP response
uint8_t response_buffer[2048];
int response_len = cw_generate_control_msg(&response_msg, response_buffer, sizeof(response_buffer));

// Encrypt with DTLS
cw_dtls_write(dtls_ctx, response_buffer, response_len);
```

## License

Part of the CAPWAP Test Suite project.
