# CAPWAP Message Library Makefile

# Compiler
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
AR = ar
ARFLAGS = rcs

# Directories
SRCDIR = src
INCDIR = include
TESTDIR = tests
BUILDDIR = build

# Library name
LIBNAME = libcapwap
LIBRARY = $(BUILDDIR)/$(LIBNAME).a

# Source and header files
SOURCES = $(wildcard $(SRCDIR)/*.c)
HEADERS = $(wildcard $(INCDIR)/*.h)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)
TEST_SRC = $(TESTDIR)/test_capwap.c
TEST_OBJ = $(TESTDIR)/test_capwap.o
TEST_BIN = $(BUILDDIR)/test_capwap
EXAMPLE_SRC = example.c
EXAMPLE_BIN = $(BUILDDIR)/example

# Default target
.PHONY: all
all: $(LIBRARY) test

# Build the library
$(LIBRARY): $(OBJECTS)
	@mkdir -p $(BUILDDIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Library built: $@"

# Compile source files
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# Build test binary
test: $(LIBRARY) $(TEST_SRC)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $(TEST_SRC) -L$(BUILDDIR) -lcapwap -o $(TEST_BIN)
	@echo "Test binary built: $(TEST_BIN)"

# Build example binary
example: $(LIBRARY) $(EXAMPLE_SRC)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $(EXAMPLE_SRC) -L$(BUILDDIR) -lcapwap -o $(EXAMPLE_BIN)
	@echo "Example binary built: $(EXAMPLE_BIN)"

# Run tests
.PHONY: run-test
run-test: test
	@echo "Running CAPWAP library tests..."
	./$(TEST_BIN)

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILDDIR)/*
	@echo "Build artifacts cleaned"

# Install the library
.PHONY: install
install: $(LIBRARY)
	@echo "Installing CAPWAP library..."
	@sudo cp $(LIBRARY) /usr/local/lib/
	@sudo cp $(INCDIR)/capwap.h /usr/local/include/
	@sudo ldconfig
	@echo "CAPWAP library installed"

# Uninstall the library
.PHONY: uninstall
uninstall:
	@echo "Uninstalling CAPWAP library..."
	@sudo rm -f /usr/local/lib/$(LIBNAME).a
	@sudo rm -f /usr/local/include/capwap.h
	@sudo ldconfig
	@echo "CAPWAP library uninstalled"

# Check if all necessary files exist
.PHONY: check
check:
	@if [ ! -f "$(INCDIR)/capwap.h" ]; then echo "Missing header file"; exit 1; fi
	@if [ ! -f "$(SRCDIR)/capwap.c" ]; then echo "Missing source file"; exit 1; fi
	@if [ ! -f "$(TEST_SRC)" ]; then echo "Missing test file"; exit 1; fi
	@echo "All necessary files present"

.PHONY: help
help:
	@echo "CAPWAP Library Makefile targets:"
	@echo "  all        - Build library and tests (default)"
	@echo "  test       - Build test binary"
	@echo "  example    - Build example binary"
	@echo "  run-test   - Build and run tests"
	@echo "  clean      - Clean build artifacts"
	@echo "  install    - Install library to system"
	@echo "  uninstall  - Uninstall library from system"
	@echo "  check      - Check if all necessary files exist"
	@echo "  help       - Show this help"