# CW-LOG Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2 -fPIC -I./include
LDFLAGS = -lpthread

# Directories
SRC_DIR = src
INC_DIR = include
TEST_DIR = tests
BUILD_DIR = build
LIB_DIR = lib

# Library
LIB_NAME = libcwlog.a
LIB_SO_NAME = libcwlog.so
LIB_STATIC = $(LIB_DIR)/$(LIB_NAME)
LIB_SHARED = $(LIB_DIR)/$(LIB_SO_NAME)

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

# Test files
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.c)
TEST_BINARIES = $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%,$(TEST_SOURCES))

# Targets
.PHONY: all clean test install static shared package

all: static shared

# Create directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build static library
static: $(LIB_STATIC)

$(LIB_STATIC): $(OBJECTS) | $(LIB_DIR)
	ar rcs $@ $(OBJECTS)
	@echo "Static library created: $@"

# Build shared library
shared: $(LIB_SHARED)

$(LIB_SHARED): $(OBJECTS) | $(LIB_DIR)
	$(CC) -shared -o $@ $(OBJECTS) $(LDFLAGS)
	@echo "Shared library created: $@"

# Build test binaries
$(BUILD_DIR)/test_%: $(TEST_DIR)/test_%.c $(LIB_STATIC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< $(LIB_STATIC) $(LDFLAGS) -o $@

# Run tests
test: $(TEST_BINARIES)
	@echo "Running tests..."
	@for test in $(TEST_BINARIES); do \
		echo "\nExecuting $$test..."; \
		LD_LIBRARY_PATH=$(LIB_DIR):$$LD_LIBRARY_PATH $$test || exit 1; \
	done
	@echo "\nAll tests completed successfully!"

# Install library
install: all
	@echo "Installing cw-log library..."
	install -d /usr/local/lib
	install -m 644 $(LIB_STATIC) /usr/local/lib/
	install -m 755 $(LIB_SHARED) /usr/local/lib/
	install -d /usr/local/include
	install -m 644 $(INC_DIR)/*.h /usr/local/include/
	ldconfig
	@echo "Installation complete"

# Package for distribution
package: all
	@echo "Creating package..."
	mkdir -p package/cw-log/lib
	mkdir -p package/cw-log/include
	cp $(LIB_STATIC) $(LIB_SHARED) package/cw-log/lib/
	cp $(INC_DIR)/*.h package/cw-log/include/
	cp README.md package/cw-log/
	tar -czf cw-log.tar.gz -C package cw-log
	rm -rf package
	@echo "Package created: cw-log.tar.gz"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(LIB_DIR)
	rm -f cw-log.tar.gz
	@echo "Clean complete"

# Help target
help:
	@echo "CW-LOG Build System"
	@echo "==================="
	@echo "Targets:"
	@echo "  all      - Build both static and shared libraries (default)"
	@echo "  static   - Build static library"
	@echo "  shared   - Build shared library"
	@echo "  test     - Build and run unit tests"
	@echo "  install  - Install library to system"
	@echo "  package  - Create distribution package"
	@echo "  clean    - Remove build artifacts"
	@echo "  help     - Show this help message"
