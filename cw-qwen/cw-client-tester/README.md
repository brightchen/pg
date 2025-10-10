# CAPWAP Client Tester

CAPWAP Client Tester is a standalone application that acts as a CAPWAP server to test CAPWAP client implementations. The application handles client discovery messages and responds appropriately.

## Features

- Acts as a CAPWAP server to test client implementations
- Handles CAPWAP client discovery requests and responds with discovery responses
- Supports graceful and forced shutdown
- Configurable leveled logging (TRACE, DEBUG, INFO, WARN, ERROR)
- Multiple log output formats (text, CSV, JSON)
- Log rotation based on file size
- Configurable via configuration file
- Test case framework with state/event-driven workflow

## Building

To build the application:

```bash
cd cw-client-tester
make
```

This will build the application and link against the capwap-lib library.

## Running

To run the application:

```bash
./build/cw-client-tester
```

The application will start a CAPWAP server on port 5246 (control) and listen for discovery requests from CAPWAP clients.

## Configuration

The application can be configured using the configuration file located at `config/cw-client-tester.conf`. The configuration file supports:

- Logging level and format
- Log file location and rotation settings
- Server binding settings
- Test case parameters

## Shutdown

The application can be gracefully shut down using `Ctrl+C` or sending a SIGTERM signal. For forced shutdown, send a SIGKILL signal.

## Directory Structure

- `src/` - Source code files
- `include/` - Header files
- `config/` - Configuration files
- `logs/` - Log files (when using file logging)
- `tests/` - Test files
- `docs/` - Documentation
- `build/` - Build artifacts