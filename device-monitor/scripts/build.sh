#!/bin/bash
set -e

# Build script for monitor application

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "Building monitor application..."
cd "$PROJECT_ROOT/monitor"
mvn clean compile

echo "Building event generator..."
cd "$PROJECT_ROOT/event-generator"
mvn clean compile

echo "Build completed successfully!"
