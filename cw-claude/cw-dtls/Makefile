# CW-DTLS Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2 -fPIC -I./include
LDFLAGS = -lssl -lcrypto

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
LIB_DIR = lib

# Library
LIB_NAME = libcwdtls.a
LIB_SO_NAME = libcwdtls.so
LIB_STATIC = $(LIB_DIR)/$(LIB_NAME)
LIB_SHARED = $(LIB_DIR)/$(LIB_SO_NAME)

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

# Targets
.PHONY: all clean install static shared package

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

# Install library
install: all
	@echo "Installing cw-dtls library..."
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
	mkdir -p package/cw-dtls/lib
	mkdir -p package/cw-dtls/include
	cp $(LIB_STATIC) $(LIB_SHARED) package/cw-dtls/lib/
	cp $(INC_DIR)/*.h package/cw-dtls/include/
	cp README.md package/cw-dtls/
	tar -czf cw-dtls.tar.gz -C package cw-dtls
	rm -rf package
	@echo "Package created: cw-dtls.tar.gz"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(LIB_DIR)
	rm -f cw-dtls.tar.gz
	@echo "Clean complete"

# Help target
help:
	@echo "CW-DTLS Build System"
	@echo "===================="
	@echo "Targets:"
	@echo "  all      - Build both static and shared libraries (default)"
	@echo "  static   - Build static library"
	@echo "  shared   - Build shared library"
	@echo "  install  - Install library to system"
	@echo "  package  - Create distribution package"
	@echo "  clean    - Remove build artifacts"
	@echo "  help     - Show this help message"
