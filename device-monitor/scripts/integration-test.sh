#!/bin/bash
set -e

# Integration test script for monitor application

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "Running integration tests for monitor application..."
echo "Note: This requires Docker to be running"

cd "$PROJECT_ROOT/monitor"
mvn verify -DskipUnitTests=true

echo "All integration tests passed!"
