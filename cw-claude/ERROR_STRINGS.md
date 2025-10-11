# Error Code String Functions

This document describes the error code to string conversion functions added to all modules.

## Overview

All modules now provide error code to string conversion functions that:
1. Convert numeric error codes to human-readable strings
2. Are used throughout the codebase for better error logging
3. Make debugging easier by providing descriptive error messages

## Modules

### 1. cw-log

**Function**: `const char* cw_log_error_string(int error_code)`

**Error Codes**:
- `CW_LOG_SUCCESS` → "Success"
- `CW_LOG_ERROR_INVALID_PARAM` → "Invalid parameter"
- `CW_LOG_ERROR_FILE_OPEN` → "Failed to open file"
- `CW_LOG_ERROR_FILE_WRITE` → "Failed to write to file"
- `CW_LOG_ERROR_INIT` → "Initialization error"
- `CW_LOG_ERROR_NOT_INIT` → "Not initialized"

**Usage**:
```c
int ret = cw_log_init(&config);
if (ret != CW_LOG_SUCCESS) {
    fprintf(stderr, "Logging init failed: %s (%d)\n",
            cw_log_error_string(ret), ret);
}
```

---

### 2. cw-msg-lib

**Function**: `const char* cw_msg_error_string(int error_code)`

**Error Codes**:
- `CW_MSG_SUCCESS` → "Success"
- `CW_MSG_ERROR_INVALID_PARAM` → "Invalid parameter"
- `CW_MSG_ERROR_BUFFER_TOO_SMALL` → "Buffer too small"
- `CW_MSG_ERROR_PARSE_FAILED` → "Parse failed"
- `CW_MSG_ERROR_UNSUPPORTED_TYPE` → "Unsupported type"
- `CW_MSG_ERROR_INVALID_FORMAT` → "Invalid format"

**Usage**:
```c
int len = cw_generate_control_msg(&msg, buffer, sizeof(buffer));
if (len < 0) {
    CW_LOG_ERROR("Failed to generate message: %s (%d)",
                 cw_msg_error_string(len), len);
}
```

---

### 3. cw-dtls

**Function**: `const char* cw_dtls_error_code_string(int error_code)`

**Error Codes**:
- `CW_DTLS_SUCCESS` → "Success"
- `CW_DTLS_ERROR_INVALID_PARAM` → "Invalid parameter"
- `CW_DTLS_ERROR_SSL_INIT` → "SSL initialization error"
- `CW_DTLS_ERROR_CERT_LOAD` → "Certificate load error"
- `CW_DTLS_ERROR_KEY_LOAD` → "Private key load error"
- `CW_DTLS_ERROR_CONNECTION` → "Connection error"
- `CW_DTLS_ERROR_HANDSHAKE` → "Handshake error"
- `CW_DTLS_ERROR_READ` → "Read error"
- `CW_DTLS_ERROR_WRITE` → "Write error"
- `CW_DTLS_ERROR_MEMORY` → "Memory allocation error"

**Usage**:
```c
int ret = cw_dtls_handshake(ctx);
if (ret != CW_DTLS_SUCCESS) {
    CW_LOG_ERROR("DTLS handshake failed: %s (%d)",
                 cw_dtls_error_code_string(ret), ret);
}
```

---

## Error Logging Best Practices

### 1. Always Include Error Code and String

```c
// Good - includes both string and numeric code
CW_LOG_ERROR("Operation failed: %s (%d)", cw_msg_error_string(ret), ret);

// Bad - only shows numeric code
CW_LOG_ERROR("Operation failed: %d", ret);
```

### 2. Include Context Information

```c
// Good - includes what operation failed
CW_LOG_ERROR("Failed to parse Discovery Request: %s (%d)",
             cw_msg_error_string(ret), ret);

// Bad - generic message
CW_LOG_ERROR("Parse failed: %s (%d)", cw_msg_error_string(ret), ret);
```

### 3. Use errno for System Call Errors

```c
// Good - uses strerror for system calls
if (sent < 0) {
    CW_LOG_ERROR("Failed to send message: %s (errno=%d)",
                 strerror(errno), errno);
}
```

### 4. Log at Appropriate Levels

```c
// Configuration errors - WARNING
if (ret != CW_LOG_SUCCESS) {
    CW_LOG_WARN("Using default config: %s (%d)",
                cw_log_error_string(ret), ret);
}

// Critical errors - ERROR
if (ret < 0) {
    CW_LOG_ERROR("Critical failure: %s (%d)",
                 cw_msg_error_string(ret), ret);
    return -1;
}
```

---

## Updated Code Examples

### cw-client-tester/src/main.c

**Before**:
```c
int parse_len = cw_parse_control_msg(buffer, len, &msg);
if (parse_len < 0) {
    CW_LOG_ERROR("Failed to parse CAPWAP message");
    return -1;
}
```

**After**:
```c
int parse_len = cw_parse_control_msg(buffer, len, &msg);
if (parse_len < 0) {
    CW_LOG_ERROR("Failed to parse CAPWAP message: %s (%d)",
                 cw_msg_error_string(parse_len), parse_len);
    return -1;
}
```

---

**Before**:
```c
int msg_len = cw_generate_control_msg(&resp, buffer, sizeof(buffer));
if (msg_len < 0) {
    CW_LOG_ERROR("Failed to generate Discovery Response");
    return -1;
}
```

**After**:
```c
int msg_len = cw_generate_control_msg(&resp, buffer, sizeof(buffer));
if (msg_len < 0) {
    CW_LOG_ERROR("Failed to generate Discovery Response: %s (%d)",
                 cw_msg_error_string(msg_len), msg_len);
    return -1;
}
```

---

**Before**:
```c
if (sent < 0) {
    CW_LOG_ERROR("Failed to send Join Response: %s", strerror(errno));
    return -1;
}
```

**After**:
```c
if (sent < 0) {
    CW_LOG_ERROR("Failed to send Join Response: %s (errno=%d)",
                 strerror(errno), errno);
    return -1;
}
```

---

## Testing

All error string functions are tested in unit tests:

- **cw-log**: 7 new tests in `test_log_error_string()`
- **cw-msg-lib**: 7 new tests in `test_msg_error_strings()`

**Test Results**:
- cw-log: 34/34 tests passed (was 27, added 7)
- cw-msg-lib: 63/63 tests passed (was 56, added 7)

### Internal Error Logging Coverage

All functions in cw-msg-lib now have comprehensive error logging:

**Parse Functions**:
- `cw_parse_header()` - logs invalid parameters, insufficient data
- `cw_parse_control_msg()` - logs header parse errors, element length overflows
- `cw_parse_ac_descriptor()` - logs invalid parameters, insufficient length
- `cw_parse_result_code()` - logs invalid parameters, short data
- `cw_parse_session_id()` - logs invalid parameters, excessive length
- `cw_parse_ac_name()` - logs invalid parameters
- `cw_parse_wtp_name()` - logs invalid parameters

**Generate Functions**:
- `cw_generate_header()` - logs invalid parameters, buffer too small
- `cw_generate_control_msg()` - logs header generation errors
- `cw_generate_ac_descriptor()` - logs invalid parameters, buffer too small
- `cw_generate_result_code()` - logs invalid parameters, buffer too small
- `cw_generate_session_id()` - logs invalid parameters, buffer too small
- `cw_generate_ac_name()` - logs invalid parameters, buffer too small

**Element Management**:
- `cw_add_element()` - logs invalid parameters, maximum elements reached

**Integration Tests**:
- `test_logging_integration.c` - tests basic error scenarios
- `test_comprehensive_logging.c` - tests all parse/generate functions with various error conditions

---

## Benefits

1. **Better Debugging**: Error messages now include descriptive strings
2. **Consistent Format**: All modules use the same pattern
3. **Complete Information**: Both error string and numeric code are logged
4. **Maintainability**: Centralized error string definitions
5. **User-Friendly**: Clear error messages for users/developers

---

## Future Enhancements

1. Add internationalization support for error strings
2. Include error recovery suggestions in error messages
3. Add error statistics/tracking
4. Create error code documentation generator
