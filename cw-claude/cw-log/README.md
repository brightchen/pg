# CW-LOG - CAPWAP Logging Library

A thread-safe, feature-rich logging library for C applications with support for multiple log levels, formats, and automatic log rotation.

## Features

- **Multiple Log Levels**: TRACE, DEBUG, INFO, WARN, ERROR
- **Multiple Output Formats**: Plain text, CSV, JSON
- **Automatic Log Rotation**: Based on configurable file size
- **Thread-Safe**: Uses mutexes for concurrent logging
- **Configurable**: Via API or configuration file
- **Detailed Context**: Includes timestamp, file, line, function name
- **Console Output**: Optional simultaneous console logging

## Building

### Build All (Static + Shared Libraries)
```bash
make
```

### Build Specific Library Type
```bash
make static  # Build static library only
make shared  # Build shared library only
```

### Run Unit Tests
```bash
make test
```

### Install System-Wide
```bash
sudo make install
```

### Create Distribution Package
```bash
make package
```

### Clean Build Artifacts
```bash
make clean
```

## Usage

### Basic Usage

```c
#include "cw_log.h"

int main() {
    // Initialize logging
    cw_log_config_t config = {
        .log_file_path = "/var/log/myapp.log",
        .log_level = CW_LOG_LEVEL_INFO,
        .log_format = CW_LOG_FORMAT_TEXT,
        .max_file_size = 10 * 1024 * 1024,  // 10MB
        .max_backup_files = 5,
        .console_output = 1  // Enable console output
    };

    cw_log_init(&config);

    // Log messages
    CW_LOG_INFO("Application started");
    CW_LOG_DEBUG("Debug message: value=%d", 42);
    CW_LOG_WARN("Warning message");
    CW_LOG_ERROR("Error occurred: %s", "example error");

    // Cleanup
    cw_log_cleanup();
    return 0;
}
```

### Initialize from Configuration File

```c
cw_log_init_from_file("/etc/myapp/logging.conf");
```

### Configuration File Format

```
log_file_path=/var/log/myapp.log
log_level=INFO
log_format=TEXT
max_file_size=10485760
max_backup_files=5
console_output=1
```

### Log Levels

- `CW_LOG_LEVEL_TRACE` - Most detailed, for tracing execution
- `CW_LOG_LEVEL_DEBUG` - Debugging information
- `CW_LOG_LEVEL_INFO` - General information
- `CW_LOG_LEVEL_WARN` - Warning messages
- `CW_LOG_LEVEL_ERROR` - Error messages

### Log Formats

#### TEXT (Default)
```
[2025-10-10 15:30:45] [INFO] [main.c:42:main] Application started
```

#### CSV
```
"2025-10-10 15:30:45","INFO","main.c",42,"main","Application started"
```

#### JSON
```json
{"timestamp":"2025-10-10 15:30:45","level":"INFO","file":"main.c","line":42,"function":"main","message":"Application started"}
```

## API Reference

### Initialization
- `int cw_log_init(const cw_log_config_t *config)` - Initialize with config structure
- `int cw_log_init_from_file(const char *config_file)` - Initialize from file

### Logging
- `CW_LOG_TRACE(fmt, ...)` - Log trace message
- `CW_LOG_DEBUG(fmt, ...)` - Log debug message
- `CW_LOG_INFO(fmt, ...)` - Log info message
- `CW_LOG_WARN(fmt, ...)` - Log warning message
- `CW_LOG_ERROR(fmt, ...)` - Log error message

### Control
- `int cw_log_set_level(int level)` - Change log level at runtime
- `int cw_log_get_level(void)` - Get current log level
- `void cw_log_flush(void)` - Flush log buffer to file
- `void cw_log_cleanup(void)` - Cleanup and close logging system

## Compilation

### Link with Static Library
```bash
gcc myapp.c -I./include -L./lib -lcwlog -lpthread -o myapp
```

### Link with Shared Library
```bash
gcc myapp.c -I./include -L./lib -lcwlog -lpthread -o myapp
```

## Error Codes

- `CW_LOG_SUCCESS` (0) - Operation successful
- `CW_LOG_ERROR_INVALID_PARAM` (-1) - Invalid parameter
- `CW_LOG_ERROR_FILE_OPEN` (-2) - Failed to open file
- `CW_LOG_ERROR_FILE_WRITE` (-3) - Failed to write to file
- `CW_LOG_ERROR_INIT` (-4) - Initialization error
- `CW_LOG_ERROR_NOT_INIT` (-5) - Not initialized

## License

Part of the CAPWAP Test Suite project.
