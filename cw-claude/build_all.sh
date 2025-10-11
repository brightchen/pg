#!/bin/bash

# CAPWAP Test Suite - Build All Script
# This script builds all modules in the correct order

set -e  # Exit on error

echo "========================================"
echo "  CAPWAP Test Suite - Build All"
echo "========================================"
echo ""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored status
print_status() {
    echo -e "${GREEN}[+]${NC} $1"
}

print_error() {
    echo -e "${RED}[!]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[*]${NC} $1"
}

# Check if we're in the right directory
if [ ! -d "cw-log" ] || [ ! -d "cw-msg-lib" ]; then
    print_error "Please run this script from the project root directory"
    exit 1
fi

# Build cw-log
print_status "Building cw-log library..."
cd cw-log
if make clean && make all; then
    print_status "cw-log built successfully"
else
    print_error "Failed to build cw-log"
    exit 1
fi
cd ..

# Build cw-msg-lib
print_status "Building cw-msg-lib library..."
cd cw-msg-lib
if make clean && make all; then
    print_status "cw-msg-lib built successfully"
else
    print_error "Failed to build cw-msg-lib"
    exit 1
fi
cd ..

# Build cw-dtls
print_status "Building cw-dtls library..."
cd cw-dtls
if make clean && make all; then
    print_status "cw-dtls built successfully"
else
    print_error "Failed to build cw-dtls"
    exit 1
fi
cd ..

# Build cw-client-tester
print_status "Building cw-client-tester application..."
cd cw-client-tester
if make clean && make all; then
    print_status "cw-client-tester built successfully"
else
    print_error "Failed to build cw-client-tester"
    exit 1
fi
cd ..

# Build cw-server-tester (if directory exists)
if [ -d "cw-server-tester" ]; then
    print_status "Building cw-server-tester application..."
    cd cw-server-tester
    if make clean && make all; then
        print_status "cw-server-tester built successfully"
    else
        print_warning "Failed to build cw-server-tester (optional)"
    fi
    cd ..
else
    print_warning "cw-server-tester not found, skipping..."
fi

echo ""
echo "========================================"
print_status "Build completed successfully!"
echo "========================================"
echo ""
echo "Built components:"
echo "  - cw-log library"
echo "  - cw-msg-lib library"
echo "  - cw-dtls library"
echo "  - cw-client-tester application"
if [ -d "cw-server-tester" ]; then
    echo "  - cw-server-tester application"
fi
echo ""
echo "To run tests:"
echo "  cd cw-log && make test"
echo "  cd cw-msg-lib && make test"
echo ""
echo "To run applications:"
echo "  cd cw-client-tester && make run"
if [ -d "cw-server-tester" ]; then
    echo "  cd cw-server-tester && make run"
fi
echo ""
