#!/bin/bash

# CAPWAP Test Suite - Clean All Script
# This script cleans build artifacts from all modules

echo "========================================"
echo "  CAPWAP Test Suite - Clean All"
echo "========================================"
echo ""

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored status
print_status() {
    echo -e "${GREEN}[+]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[*]${NC} $1"
}

# Check if we're in the right directory
if [ ! -d "cw-log" ] || [ ! -d "cw-msg-lib" ]; then
    print_warning "Please run this script from the project root directory"
    exit 1
fi

# Clean cw-log
if [ -d "cw-log" ]; then
    print_status "Cleaning cw-log..."
    cd cw-log && make clean && cd ..
fi

# Clean cw-msg-lib
if [ -d "cw-msg-lib" ]; then
    print_status "Cleaning cw-msg-lib..."
    cd cw-msg-lib && make clean && cd ..
fi

# Clean cw-dtls
if [ -d "cw-dtls" ]; then
    print_status "Cleaning cw-dtls..."
    cd cw-dtls && make clean && cd ..
fi

# Clean cw-client-tester
if [ -d "cw-client-tester" ]; then
    print_status "Cleaning cw-client-tester..."
    cd cw-client-tester && make clean && cd ..
fi

# Clean cw-server-tester
if [ -d "cw-server-tester" ]; then
    print_status "Cleaning cw-server-tester..."
    cd cw-server-tester && make clean && cd ..
fi

# Clean log files
print_status "Cleaning log files..."
rm -f /tmp/cw_*.log 2>/dev/null || true
rm -f /tmp/test_cw_*.log 2>/dev/null || true

echo ""
print_status "Clean completed!"
echo ""
