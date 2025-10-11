#!/bin/bash

# CAPWAP Test Suite - Test All Script
# This script runs all unit tests

set -e  # Exit on error

echo "========================================"
echo "  CAPWAP Test Suite - Run All Tests"
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

TESTS_PASSED=0
TESTS_FAILED=0

# Test cw-log
print_status "Testing cw-log library..."
cd cw-log
if make test; then
    print_status "cw-log tests PASSED"
    ((TESTS_PASSED++))
else
    print_error "cw-log tests FAILED"
    ((TESTS_FAILED++))
fi
cd ..
echo ""

# Test cw-msg-lib
print_status "Testing cw-msg-lib library..."
cd cw-msg-lib
if make test; then
    print_status "cw-msg-lib tests PASSED"
    ((TESTS_PASSED++))
else
    print_error "cw-msg-lib tests FAILED"
    ((TESTS_FAILED++))
fi
cd ..
echo ""

# Print summary
echo "========================================"
echo "  Test Summary"
echo "========================================"
echo "Tests Passed: $TESTS_PASSED"
echo "Tests Failed: $TESTS_FAILED"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    print_status "All tests passed!"
    exit 0
else
    print_error "Some tests failed!"
    exit 1
fi
