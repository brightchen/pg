# CW-CLIENT-TESTER Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./include -I../cw-log/include -I../cw-msg-lib/include -I../cw-dtls/include
LDFLAGS = -L../cw-log/lib -L../cw-msg-lib/lib -L../cw-dtls/lib -lcwlog -lcwmsg -lcwdtls -lssl -lcrypto -lpthread

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Target binary
TARGET = $(BIN_DIR)/cw-client-tester

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

# Targets
.PHONY: all clean run test

all: $(TARGET)

# Create directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build target binary
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo "Binary created: $@"

# Run the application
run: $(TARGET)
	@echo "Starting CAPWAP Client Tester..."
	LD_LIBRARY_PATH=../cw-log/lib:../cw-msg-lib/lib:../cw-dtls/lib:$$LD_LIBRARY_PATH \
		$(TARGET) -c config/client_tester.conf

# Test the application
test: $(TARGET)
	@echo "Running tests..."
	LD_LIBRARY_PATH=../cw-log/lib:../cw-msg-lib/lib:../cw-dtls/lib:$$LD_LIBRARY_PATH \
		$(TARGET) -c config/test.conf

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Clean complete"

# Help target
help:
	@echo "CW-CLIENT-TESTER Build System"
	@echo "=============================="
	@echo "Targets:"
	@echo "  all      - Build the application (default)"
	@echo "  run      - Build and run the application"
	@echo "  test     - Build and run tests"
	@echo "  clean    - Remove build artifacts"
	@echo "  help     - Show this help message"
