#!/bin/bash
set -e

# Unit test script for monitor application

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "Running unit tests for monitor application..."
cd "$PROJECT_ROOT/monitor"
mvn test

echo "All tests passed!"
