CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O2
LDFLAGS = -lpthread

# Directories
CAPWAP_LIB_DIR = ../capwap-lib
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

# Include directories
INCLUDES = -I$(INC_DIR) -I$(CAPWAP_LIB_DIR)/include

# Libraries
LIBS = -L$(CAPWAP_LIB_DIR)/build -lcapwap

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Target
TARGET = cw-client-tester

.PHONY: all clean test

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) $(LDFLAGS) -o $(BUILD_DIR)/$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

test: $(TARGET)
	./$(BUILD_DIR)/$(TARGET)

# Ensure capwap library is built before building this application
$(OBJECTS): | $(CAPWAP_LIB_DIR)/build/libcapwap.a

$(CAPWAP_LIB_DIR)/build/libcapwap.a:
	$(MAKE) -C $(CAPWAP_LIB_DIR)